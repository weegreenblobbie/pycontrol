#! /usr/bin/env python3
"""
A client that reads from the gpsd service continuously in a thread while
allowing other threads to safely read the latest state.
"""
import copy
import datetime
import threading
import time

import gps # the gpsd interface module

MODES = ("INVALID", "NO FIX", "2D FIX", "3D FIX")
STATUS = ("UNKNOWN", "FIX", "DGPS_FIX")

now = datetime.datetime.now

NO_ERROR = 0

def _get_hms(current, mode_time_start):
    if not mode_time_start:
        return None
    seconds = (current - mode_time_start).total_seconds()
    num_hours = int(seconds / 3600.0)
    seconds -= num_hours * 3600.0
    num_minutes = int(seconds / 60.0)
    seconds -= num_minutes * 60.0
    seconds = int(seconds)
    return f"{num_hours:2d}:{num_minutes:02d}:{seconds:02d}"


class GpsReader:

    def __init__(self):
        self._altitude = None
        self._path = None
        self._mode = "Disconnected"
        self._mode_time = now()
        self._device = None
        self._connected = False
        self._error = None
        self._time = None
        self._time_err = None
        self._lat = None
        self._long = None
        self._satellites_seen = None
        self._satellites_used = None
        self._session = None
        self._lock = threading.Lock()
        self._thread = None

    def start(self):
        self._thread = threading.Thread(target=self._run_in_thread)
        self._thread.daemon = True
        self._thread.start()

    def read(self):
        assert self._thread, "Please call start() first!"
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

        data["mode_time"] = _get_hms(now(), data["mode_time"])

        # Discard Nones.
        keys = list(data.keys())
        for key in keys:
            v = data[key]
            if v is None:
                del data[key]

        return data

    def _run_in_thread(self):
        session = gps.gps(mode=gps.WATCH_ENABLE)
        self._session = session

        while True:
            response = session.read()
            if response != NO_ERROR:
                time.sleep(1)
                continue

            # The valid bits when it's connected and a FIX vs no GPS device:
            #         24 23    16 15     8 7      0
            #     0b0010 00000101 00110101 01111110    # Working with GPS FIX
            #
            # The bit fields are defined here:
            #
            #     https://gitlab.com/gpsd/gpsd/-/blob/f7a8c18f6e676f921cc683198d3b6df439d3d047/gps/gps.py.in#L69
            #
            # Happy data have these bits set:
            #
            #     bit 1 is ONLINE_SET - not sure what this means
            #     bit 2 is TIME_SET
            #     bit 3 is TIMERR_SET
            #     bit 4 is LATLON_SET
            #     bit 5 is ALTITUDE_SET
            #     bit 6 is SPEED_SET
            #     bit 7 is TRACK_SET
            #     bit 8 is CLIMB_SET
            #     bit 9 is STATUS_SET
            #     bit 10 is MODE_SET
            #     bit 11 is DOP_SET
            #     bit 12 is HERR_SET
            #     bit 13 is VERR_SET
            #     bit 14 is ATTITUDE_SET
            #     bit 15 is SATELLITE_SET
            #     bit 16 is SPEEDERR_SET
            #     bit 17 is TRACKERR_SET
            #     bit 18 is CLIMBERR_SET
            #     bit 25 is PACKET_SET
            #
            # When I unplug the USB GPS device after it's been happy:
            #
            #         24 23    16 15     8 7      0
            #     0b0010 00001000 00000000 00000010
            #
            #     bit 1 is ONLINE_SET
            #     bit 19 is DEVICE_SET
            #     bit 25 is PACKET_SET
            #
            # When a GPS device has never been plugged in:
            #
            #         24 23    16 15     8 7      0
            #     0b0010 00000000 00000000 00000000
            #
            #     bit 25 is PACKET_SET

            have_error = gps.ERROR_SET & session.valid != 0

            connected = gps.ONLINE_SET & session.valid != 0

            device_error = gps.DEVICE_SET & session.valid != 0

            with self._lock:
                self._connected = connected
                if session.satellites:
                    self._satellites_used = session.satellites_used
                    self._satellites_seen = len(session.satellites)
                self._path = session.path if session.path else None
                self._device = session.device
                self._error = have_error or device_error
                if self._error:
                    current_mode = "Disconnected"
                    if current_mode != self._mode:
                        self._mode = current_mode
                        self._mode_time = now()
                    self._time = None
                    self._time_err = None
                    self._lat = None
                    self._long = None
                    self._altitude = None
                    self._satellites_used = None
                    self._satellites_seen = None

            if not (gps.MODE_SET & session.valid) or session.fix.mode == 0:
                # not useful, probably not a TPV message
                continue

            with self._lock:

                if self._mode_time is None:
                    self._mode_time = now()

                # Mode.
                current_mode = MODES[session.fix.mode]
                if current_mode != self._mode:
                    self._mode = current_mode
                    self._mode_time = now()

                # Time.
                if gps.TIME_SET & session.valid:
                    self._time = session.fix.time
                else:
                    self._time = None

                # Time error.
                if gps.TIMERR_SET & session.valid:
                    self._time_err = session.fix.ept
                else:
                    self._time_err = None

                # Location.
                if (
                    gps.isfinite(session.fix.latitude) and
                    gps.isfinite(session.fix.longitude)
                ):
                    self._lat = session.fix.latitude
                    self._long = session.fix.longitude
                else:
                    self._lat = None
                    self._long = None

                # Altitude above mean sea level.
                if gps.isfinite(session.fix.altMSL):
                    self._altitude = session.fix.altMSL
                else:
                    self._altitude = None

                # if lat long or alt is None, reset mode.
                if any([self._lat is None, self._long is None, self._altitude is None]):
                    self._mode = "INVALID"
                    self._mode_time = now()


        print("thread exited")


if __name__ == "__main__":
    gr = GpsReader()
    gr.start()

    while True:
        print(gr.read())
        time.sleep(3)

