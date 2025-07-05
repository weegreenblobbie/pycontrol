import socket
import json
import time
import threading
import datetime

from pyproj import Transformer


def format_timedelta_to_hms(td):
    """Formats a timedelta object into a HH:MM:SS string."""
    # Updated to use datetime.timedelta
    if not isinstance(td, datetime.timedelta):
        return "00:00:00"
    total_seconds = int(td.total_seconds())
    hours, remainder = divmod(total_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{hours:02}:{minutes:02}:{seconds:02}"


class GpsReader:
    """
    A class to read GPS data directly from the gpsd socket in a separate thread.
    This version has NO dependency on the 'python3-gps' library.
    """
    DATA_TIMEOUT_SECONDS = 1.25
    FLOAT_TOLERANCE = 1e-7
    MODES = ("NOT SEEN", "NO FIX", "2D FIX", "3D FIX")
    NO_FIX = 1

    def __init__(self, host="127.0.0.1", port="2947"):
        self._host = host
        self._port = port
        self._thread = None
        self._running = False
        self._lock = threading.Lock()

        self._connected = False
        self._error = False
        self._mode = self.MODES[0]
        self._mode_time = "00:00:00"
        self._time = None
        self._lat = 0.0
        self._long = 0.0
        self._altitude = 0.0
        self._time_err = None
        self._device = None
        self._path = None
        self._satellites_used = 0
        self._satellites_seen = 0

        self._last_known_mode = 0
        # Updated to use datetime.datetime
        self._mode_change_time = datetime.datetime.now()

        ecef_crs = "EPSG:4978"
        lla_crs = "EPSG:4326"
        self._transformer = Transformer.from_crs(ecef_crs, lla_crs, always_xy=True)

    def start(self):
        if self._running:
            return
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self):
        if not self._running:
            return
        self._running = False
        if self._thread and self._thread.is_alive():
            self._thread.join()

    def read(self):
        """Returns a dictionary containing the latest GPS data. Thread-safe."""
        with self._lock:
            data = dict(
                connected=self._connected and not self._error,
                mode=self._mode,
                mode_time=self._mode_time,
                time=self._time,
                lat=self._lat,
                long=self._long,
                altitude=self._altitude,
                time_err=self._time_err,
                device=self._device,
                path=self._path,
                sats_used=self._satellites_used,
                sats_seen=self._satellites_seen,
            )
        return data

    def _run(self):
        """The main loop for the background thread."""
        sock = None
        last_data_timestamp = datetime.datetime.now()

        # Pre-compute the timedelta object before the loop starts
        timeout_delta = datetime.timedelta(seconds=self.DATA_TIMEOUT_SECONDS)

        while self._running:
            try:
                if sock is None:
                    print(f"Connecting to gpsd socket at {self._host}:{self._port}")
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(self.DATA_TIMEOUT_SECONDS)
                    sock.connect((self._host, int(self._port)))
                    sock.sendall(b'?WATCH={"enable":true,"json":true}\n')
                    sock_file = sock.makefile('r')

                line = sock_file.readline()
                if not line:
                    raise ConnectionError("gpsd disconnected")

                report = json.loads(line)
                last_data_timestamp = datetime.datetime.now()

                updates_to_commit = {}
                if report.get('class') == 'TPV':
                    updates_to_commit.update(self._process_tpv_report(report))
                elif report.get('class') == 'SKY':
                    updates_to_commit.update(self._process_sky_report(report))

                if updates_to_commit:
                    with self._lock:
                        for key, value in updates_to_commit.items():
                            setattr(self, f"_{key}", value)

            except Exception as err:
                print(f"gps_reader.py error: {err}")
                sock.close()
                sock = None

                with self._lock:
                    self._error = True
                    self._connected = False
                    if self._last_known_mode != 0:
                        self._last_known_mode = 0
                        self._mode_change_time = datetime.datetime.now()
                        self._mode = self.MODES[0]
                        self._mode_time = "00:00:00"
                    self._time = "N/A"
                    self._lat = "N/A"
                    self._long = "N/A"
                    self._altitude = "N/A"
                    self._satellites_used = 0
                    self._satellites_seen = 0


    def _process_tpv_report(self, report):
        """Processes a TPV report and returns a dict of updates."""
        updates = {'connected': True, 'error': False}

        current_mode = report.get('mode', 0)

        has_fix = current_mode >= 2
        lat_from_report = report.get('lat', 0.0)
        has_valid_lat_lon = abs(lat_from_report) > self.FLOAT_TOLERANCE
        ecefx = report.get('ecefx', 0.0)
        has_valid_ecef = abs(ecefx) > 10000.0

        # My cheap u-blox GPS receiver sometimes produces bogus lat, long, alt,
        # but valid ECEF.  So if we have ECEF, use that instead of the lat, long,
        # alt.
        if has_fix and has_valid_ecef:
            x, y, z = report['ecefx'], report['ecefy'], report['ecefz']
            lon, lat, alt = self._transformer.transform(x, y, z)
            updates['lat'], updates['long'], updates['altitude'] = lat, lon, alt

        # Sometimes we don't have ECEF, so fall back on lat, long, alt.
        elif has_fix  and has_valid_lat_lon:
            updates['lat'] = lat_from_report
            updates['long'] = report.get('lon', 0.0)
            updates['altitude'] = report.get('alt', 0.0)
        else:
            # Oh no!  We don't have either!  Consider the GPS disconnected to
            # bring attention to the user.
            current_mode = self.NO_FIX
            updates['lat'], updates['long'], updates['altitude'] = 0.0, 0.0, 0.0

        if current_mode != self._last_known_mode:
            self._last_known_mode = current_mode
            self._mode_change_time = datetime.datetime.now()

        time_in_mode = datetime.datetime.now() - self._mode_change_time
        updates['mode'] = self.MODES[current_mode]
        updates['mode_time'] = format_timedelta_to_hms(time_in_mode)
        updates['time'] = report.get('time')

        return updates

    def _process_sky_report(self, report):
        """Processes a SKY report and returns a dict of updates."""
        updates = {'connected': True, 'error': False}
        satellites = report.get('satellites', [])
        updates['satellites_seen'] = len(satellites)
        updates['satellites_used'] = sum(1 for sat in satellites if sat.get('used', False))
        return updates

# ==============================================================================
# Example Usage Block
# ==============================================================================
if __name__ == "__main__":
    gps_reader = None
    try:
        print("Initializing GpsReader...")
        gps_reader = GpsReader()
        gps_reader.start()

        print("GpsReader started. Reading data every second for 60 seconds.")
        for i in range(30):
            time.sleep(2)
            latest_data = gps_reader.read()

            print("\n--- GPS Data Snapshot ---")
            if not latest_data.get('connected'):
                print("  Status: DISCONNECTED")
            else:
                for key, value in latest_data.items():
                    print(f"  {key:<15}: {value}")

    except KeyboardInterrupt:
        print("\nProgram interrupted by user.")
    finally:
        if gps_reader:
            gps_reader.stop()
