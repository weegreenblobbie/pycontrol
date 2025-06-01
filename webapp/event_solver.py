import copy
import datetime
import threading

import date_utils as du

class EventSolver:

    COMPUTING = "Computing ..."

    def __init__(self):
        self._params = None
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
            self._event.wait(timeout=60.0)

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

        type_ = params["type"]
        assert type_ in {"solar", "lunar", "custom"}

        if type_ == "solar":
            solution = self._solve_solar(params)

        elif type_ == "lunar":
            solution = self._solve_lunar(params)

        elif type_ == "custom":
            solution = self._solve_custom(params)

        print(f"solution = {solution}")

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
        assert "events" in new_params
        assert "event" in new_params

        new_params = copy.deepcopy(new_params)
        solution = dict()

        with self._lock:
            self._params = new_params
            self._solution = solution
            for event_id in new_params["events"]:
                self._solution[event_id] = self.COMPUTING

            # Immediately wake up the worker thread.
            self._event.set()

    def _solve_custom(self, params):
        """
        The "custom" format is just a list of a priori event names mapped to
        datetimes.

        events:
            - EVENT_ID: DATETIME
            - EVENT_ID: DATETIME

        """
        solution = dict()
        for event_id in params["events"]:
            solution[event_id] = params["event"][event_id]

        return solution

