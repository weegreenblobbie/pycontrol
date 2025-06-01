import copy
import datetime
import threading

import astropy.time

import date_utils as du
from solar_eclipse_contact_times import find_contact_times as solar_contact_times

class EventSolver:

    COMPUTING = "Computing ..."

    def __init__(self):
        self._params = dict()
        self._solution = dict()
        self._lock = threading.Lock()
        self._event = threading.Event()
        self._thread = None
        self._solution = dict()

    def start(self):
        assert self._thread is None
        self._event.clear()
        self._thread = threading.Thread(target=self._run, name="EventSolverThread", daemon=True)
        self._thread.start()

    def _run(self):
        while True:
            # Wait for either a trigger (event.set()) or 60 seconds (timeout).
            # If event.set() is called, event.wait() returns immediately.
            # Otherwise, event.wait() returns after 60 seconds.
            self._event.wait(timeout=10.0)

            # Clear the event immediately after waking up, whether by trigger or timeout
            # This prepares it for the next trigger or timeout cycle.
            self._event.clear()

            # Dispatch the calculation.
            self._calculate_and_store()

    def _calculate_and_store(self):
        """
        Performs the calculation and stores the result safely.
        This method is called by the worker thread.
        """
        with self._lock:
            params = copy.deepcopy(self._params)

        if not params:
            return

        solution = self._solve(params)

        with self._lock:
            self._solution = solution

    def read(self):
        """
        Provides access to the latest calculation result.
        Safe for Flask app to call.
        """
        with self._lock:
            out = copy.deepcopy(self._solution)
        return out

    def event_ids(self):
        return self.params().get("event_ids", [])

    def params(self):
        """
        Returns the configured event ids.
        """
        out = None
        with self._lock:
            out = copy.deepcopy(self._params)
        return out

    def simulate_trigger(self, lat, long, alt):
        """
        When we kick off a simulation, need to reset the solution for the
        specifc gps location in order to compute the sim_time_offset based on an
        event_id, for example, C2 - 60.0.
        """
        with self._lock:
            params = copy.deepcopy(self._params)
        params["lat"] = lat
        params["long"] = long
        params["altitude"] = alt
        return self._solve(params)

    def update_and_trigger(self, **new_params):
        """
        Updates an input parameter and triggers an immediate recalculation.
        Safe for Flask app to call.
        """
        assert "type" in new_params
        assert "lat" in new_params
        assert "long" in new_params
        assert "altitude" in new_params
        assert "datetime" in new_params
        assert "event_ids" in new_params
        assert "event" in new_params

        new_params = copy.deepcopy(new_params)

        with self._lock:
            self._params = new_params
            self._solution = dict()
            # Immediately wake up the worker thread.
            self._event.set()

    def _solve(self, params):
        """
        Solves contact times.
        """

        type_ = params["type"]
        assert type_ in {"solar", "lunar", "custom"}

        solution = dict()

        if type_ == "custom":
            solution = self._solve_custom(params)

        elif type_ == "solar":
            solution = self._solve_solar(params)

        elif type_ == "lunar":
            solution = self._solve_lunar(params)

        return solution


    def _solve_custom(self, params):
        """
        The "custom" format is just a list of a priori event names mapped to
        datetimes.

        events:
            - EVENT_ID: DATETIME
            - EVENT_ID: DATETIME

        """
        solution = dict()
        for event_id in params["event_ids"]:
            solution[event_id] = params["event"][event_id]

        return solution


    def _solve_solar(self, params):
        """
        Computes the contact time for a total solar eclipse.

        C1, C2, MID, C3, C4.
        """
        solution = solar_contact_times(
            start = astropy.time.Time(params["datetime"], scale="utc"),
            lat = params["lat"],
            lon = params["long"],
            alt = params["altitude"],
        )
        solution.pop("type")
        for key, value in solution.items():
            if isinstance(value, astropy.time.Time):
                solution[key] = value.to_datetime().astimezone(datetime.timezone.utc)
        return solution

    def _solve_lunar(self, params):
        """
        TODO: implement.
        """
        return dict(P1=None, U1=None, U2=None, MID=None, U3=None, U4=None, P4=None)


