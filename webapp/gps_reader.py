import gps
import time
import threading
from datetime import datetime, timedelta
from pyproj import Transformer

def format_timedelta_to_hms(td):
    """Formats a timedelta object into a HH:MM:SS string."""
    if not isinstance(td, timedelta):
        return "00:00:00"
    total_seconds = int(td.total_seconds())
    hours, remainder = divmod(total_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{hours:02}:{minutes:02}:{seconds:02}"

class GpsReader:
    """
    A class to read GPS data from gpsd in a separate thread.
    """
    DATA_TIMEOUT_SECONDS = 2
    FLOAT_TOLERANCE = 1e-7
    MODES = ("NOT SEEN", "NO FIX", "2D FIX", "3D FIX")

    def __init__(self):
        self._thread = None
        self._running = False
        self._lock = threading.Lock()

        # Initialize all data fields to default values.
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
        self._mode_change_time = datetime.now()

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
                sats_used=self._satellites_used,
                sats_seen=self._satellites_seen,
            )
        return data

    def _run(self):
        """The main loop for the background thread."""
        session = None
        last_report_timestamp = datetime.now()

        while self._running:
            try:
                if session is None:
                    session = gps.gps(mode=gps.WATCH_ENABLE | gps.WATCH_NEWSTYLE)

                # Check for stale data first.
                if datetime.now() - last_report_timestamp > timedelta(seconds=self.DATA_TIMEOUT_SECONDS):
                    # If data is stale, acquire lock and update the status.
                    with self._lock:
                        self._connected = False
                        if self._last_known_mode != 0:
                            self._last_known_mode = 0
                            self._mode_change_time = datetime.now()
                            self._mode = self.MODES[0]
                            self._mode_time = "00:00:00"

                # Use a very short timeout to poll for data without blocking
                # for long.
                if session.waiting(timeout=0.1):
                    report = session.next()
                    # Reset timer on successful read
                    last_report_timestamp = datetime.now()

                    updates_to_commit = {'connected': True, 'error': False}
                    if report['class'] == 'TPV':
                        updates_to_commit.update(self._process_tpv_report(report))
                    elif report['class'] == 'SKY':
                        updates_to_commit.update(self._process_sky_report(report))

                    # --- Acquire lock only for the final, quick update ---
                    with self._lock:
                        for key, value in updates_to_commit.items():
                            setattr(self, f"_{key}", value)

            except Exception as e:
                print(f"GpsReader thread error: {e}")
                with self._lock:
                    self._error = True
                    self._connected = False
                if session:
                    session.close()
                session = None

            # Sleep at the end of every loop iteration for responsiveness.
            time.sleep(0.25)

    def _process_tpv_report(self, report):
        """Processes a TPV report and returns a dict of updates."""
        updates = {}
        current_mode = getattr(report, 'mode', 0)

        if current_mode != self._last_known_mode:
            # Note: This check uses a stale self value, but it's only for a print statement.
            # The core state update will be atomic.
            # This state is only used by this thread.
            self._last_known_mode = current_mode
            self._mode_change_time = datetime.now()

        time_in_mode = datetime.now() - self._mode_change_time
        updates['mode'] = self.MODES[current_mode]
        updates['mode_time'] = format_timedelta_to_hms(time_in_mode)
        updates['time'] = getattr(report, 'time', None)

        has_fix = current_mode >= 2
        lat_from_report = getattr(report, 'lat', 0.0)
        is_lat_lon_invalid = abs(lat_from_report) < self.FLOAT_TOLERANCE
        ecefx = getattr(report, 'ecefx', 0.0)
        has_valid_ecef = abs(ecefx) < self.FLOAT_TOLERANCE

        if has_fix and is_lat_lon_invalid and has_valid_ecef:
            x, y, z = report.ecefx, report.ecefy, report.ecefz
            lon, lat, alt = self._transformer.transform(x, y, z)
            updates['lat'], updates['long'], updates['altitude'] = lat, lon, alt
        elif has_fix:
            updates['lat'] = lat_from_report
            updates['long'] = getattr(report, 'lon', 0.0)
            updates['altitude'] = getattr(report, 'alt', 0.0)
        else:
            updates['lat'], updates['long'], updates['altitude'] = 0.0, 0.0, 0.0

        return updates

    def _process_sky_report(self, report):
        """Processes a SKY report and returns a dict of updates."""
        updates = {}
        if hasattr(report, 'satellites'):
            updates['satellites_seen'] = len(report.satellites)
            updates['satellites_used'] = sum(1 for sat in report.satellites if sat.used)
        else:
            updates['satellites_seen'] = 0
            updates['satellites_used'] = 0
        return updates
