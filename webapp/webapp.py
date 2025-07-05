import copy
import dataclasses
import datetime
import flask
import glob
import logging
import os.path
import threading
import time

# These imports can remain at the top level
import date_utils as du
from camera_control_io import CameraControlIo
from event_solver import EventSolver
from gps_reader import GpsReader
import utils

# These dataclasses are also fine at the top level as they define data structures
@dataclasses.dataclass(kw_only=True)
class GpsPos:
    latitude: float = 0.0
    longitude: float = 0.0
    altitude: float = 0.0

@dataclasses.dataclass(kw_only=True)
class RunSim:
    is_running: bool = False
    gps_pos: GpsPos = dataclasses.field(default_factory=GpsPos)
    gps_offset: GpsPos = dataclasses.field(default_factory=GpsPos)
    event_id: str = ""
    event_time_offset: float = 0.0
    sim_time_offset: datetime.timedelta = datetime.timedelta(seconds=0.0)


def make_response(status_or_obj, message=None, return_code=200):
    if isinstance(status_or_obj, dict):
        success = status_or_obj.get('success')
        message = status_or_obj.get('message')
        return_code = 200 if success else 400
        status = "Success" if success else "Failure"
        data = status_or_obj.get('data')
    else:
        status = status_or_obj
        data = None
    app.logger.info(f"status: {status} return_code: {return_code} message: {message} data: {data}")
    if data:
        return (flask.jsonify(data), return_code)
    return (flask.jsonify(status=status, message=message), return_code)


class PyControlApp:
    def __init__(self, root_dir):
        """
        Initializes all long-running objects and starts their threads.
        """
        print("Initializing PyControlApp...")
        self._gps_reader = GpsReader()
        self._cam_io = CameraControlIo(os.path.join(root_dir, "config", "camera_control.config"))
        self._event_solver = EventSolver(self._cam_io)
        self._run_sim = RunSim()
        self._lock = threading.Lock()
        self._gps_reader.start()
        self._cam_io.start()
        self._event_solver.start()
        self._event_file = None
        self._seq_file = None
        self._defaults_loaded = False

        # Read in gui config to load previous event and sequence files.
        self._gui_config_filename = os.path.join(root_dir, "config", "gui.config")
        print("All reader/solver threads started.")

    def load_defaults(self):
        if self._defaults_loaded:
            return
        cfg = utils.read_kv_config(self._gui_config_filename)
        self.load_event_file(cfg["event_file"])
        self.load_sequence(cfg["sequence_file"])
        self._defaults_loaded = True

    def get_dashboard_data(self):
        """Gathers all data needed for the main dashboard update."""
        telem = self._cam_io.read()
        detected_cameras = telem.pop("detected_cameras", [])
        events = self._read_events(self._event_solver.read())
        camera_control = dict(
            state = telem.pop("state", "unknown"),
            sequence = telem.pop("sequence", ""),
            sequence_state = telem.pop("sequence_state", []),
        )
        return {
            "gps": self._gps_reader.read(),
            "detected_cameras": detected_cameras,
            "event_filename": self._event_file,
            "sequence_filename": self._seq_file,
            "events": events,
            "camera_control": camera_control,
        }

    def set_camera_id(self, serial, cam_id):
        """Public method to update a camera's description."""
        return make_response(self._cam_io.set_camera_id(serial, cam_id))

    def load_event_file(self, filename):
        """Loads an event file and triggers an update."""
        full_path = os.path.join("../events", filename)
        with open(full_path, "r") as fin:
            all_lines = fin.readlines()

        date_ = None
        data = dict(event_map=dict())
        event_ids = None
        for line in all_lines:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            tokens = line.split()
            key, values = tokens[0], tokens[1:]
            if key == "type":
                data["type"] = values[0]
            elif key == "date":
                date_ = du.make_datetime(values[0])
            elif key == "event_ids":
                event_ids = values
                data["event_ids"] = values
            elif event_ids and key in event_ids:
                data["event_map"][key] = du.make_datetime(values[0])

        self._update_and_trigger(
            type = data["type"],
            datetime = date_,
            event_ids = data["event_ids"],
            event_map = data.get("event_map", {}),
        )

        self._event_file = filename
        utils.write_kv_config(
            self._gui_config_filename,
            dict(
                event_file = self._event_file,
                sequence_file = self._seq_file,
            )
        )

        return data

    def load_sequence(self, filename):
        """Public method to load a camera sequence and re-trigger the solver."""
        full_path = os.path.abspath(os.path.join("..", "sequences", filename))
        if not os.path.isfile(full_path):
            return make_response("Failure", f"Sequence file not found: {filename}", 500)

        response = self._cam_io.load_sequence(full_path)
        if not response.get("success"):
            return make_response(response)

        # Re-trigger the solver with existing params after loading a new sequence
        params = self._event_solver.params()
        self._update_and_trigger(**params)

        self._seq_file = filename
        utils.write_kv_config(
            self._gui_config_filename,
            dict(
                event_file = self._event_file,
                sequence_file = self._seq_file,
            )
        )
        return make_response("Success", f"Sequence loaded: {filename}", 200)

    def start_simulation(self, gps_latitude, gps_longitude, gps_altitude, event_id, event_time_offset):
        """Starts a simulation. Returns (status_str, message_str)."""
        with self._lock:
            if self._run_sim.is_running:
                return make_response("Failure", "Simulation is already running.", 400)

        if event_id and event_id not in self._event_solver.event_ids():
            return "Failure", f"Bad event_id '{event_id}'"

        solution = self._event_solver.simulate_trigger(gps_latitude, gps_longitude, gps_altitude)

        sim_time_offset = datetime.timedelta(seconds=0)

        if event_id:
            event_time = solution.get(event_id)
            if event_time is None:
                return make_response("Failure", f"No event time for {event_id}", 400)
            if event_time_offset is None:
                return make_response("Failure", "Event time offset cannot be None", 400)

            event_time_offset_delta = datetime.timedelta(seconds=event_time_offset)
            sim_time_offset = du.now() - event_time - event_time_offset_delta

        snapshot = self._gps_reader.read()
        gps_offset = GpsPos(latitude=snapshot["lat"], longitude=snapshot["long"], altitude=snapshot["altitude"])
        gps_pos = GpsPos(latitude=gps_latitude, longitude=gps_longitude, altitude=gps_altitude)

        with self._lock:
            self._run_sim = RunSim(
                is_running=True,
                gps_pos=gps_pos,
                gps_offset=gps_offset,
                event_id=event_id,
                event_time_offset=event_time_offset,
                sim_time_offset=sim_time_offset,
            )

        params = self._event_solver.params()
        self._update_and_trigger(**params)
        return make_response("Success", "Simulation started", 200)

    def read_choices(self, serial, prop):
        return make_response(self._cam_io.read_choices(serial, prop))

    def set_choice(self, serial, prop, value):
        return make_response(self._cam_io.set_choice(serial, prop, value))

    def stop_simulation(self):
        """Stops the current simulation."""
        self._cam_io.set_events({})
        self._cam_io.reset_sequence()
        with self._lock:
            self._run_sim = RunSim()
        params = self._event_solver.params()
        self._update_and_trigger(**params)
        return make_response("Success", "Simulation stopped.", 200)

    def trigger(self, serial):
        return make_response(self._cam_io.trigger(serial))

    def _read_events(self, event_map):
        """Internal method to get and format event data from the solver."""
        event_ids = self._event_solver.event_ids()
        out = []
        for event_id in event_ids:
            event_time = event_map.get(event_id, EventSolver.COMPUTING)
            if event_time == EventSolver.COMPUTING:
                out.append( (event_id, event_time, "") )
            elif event_time is None:
                out.append( (event_id, "Event not visible!", "N/A") )
            elif isinstance(event_time, datetime.datetime):
                eta = du.eta(source=du.now(), destination=event_time)
                out.append( (event_id, du.normalize(event_time), eta) )
            else:
                raise ValueError(f"Unknown event_time type: {event_time}")
        return out

    def _update_and_trigger(self, **kwargs):
        """Internal method to update the event solver with new data."""
        gps = self._gps_reader.read()
        latitude = gps["lat"]
        longitude = gps["long"]
        altitude = gps["altitude"]

        with self._lock:
            sim_is_running = self._run_sim.is_running
            sim_pos = copy.deepcopy(self._run_sim.gps_pos)
            sim_offset = copy.deepcopy(self._run_sim.gps_offset)
            sim_time_offset = self._run_sim.sim_time_offset

        if sim_is_running:
            latitude = sim_pos.latitude + (sim_offset.latitude - latitude)
            longitude = sim_pos.longitude + (sim_offset.longitude - longitude)
            altitude = sim_pos.altitude + (sim_offset.altitude - altitude)

        self._event_solver.update_and_trigger(
            type = kwargs["type"],
            datetime = kwargs["datetime"],
            lat = latitude,
            long = longitude,
            altitude = altitude,
            event_ids = kwargs["event_ids"],
            event_map = kwargs["event_map"],
            sim_time_offset = sim_time_offset,
        )

#------------------------------------------------------------------------------
# Global Flask app and PyControlApp instance
#
ROOT_DIR = "../"
app = flask.Flask(__name__)
app.config['DEBUG'] = False
logging.getLogger('werkzeug').setLevel(logging.ERROR)

pycontrol_app = PyControlApp(ROOT_DIR)

#------------------------------------------------------------------------------
# Flask Routes
#
@app.route('/')
def hello():
    pycontrol_app.load_defaults()
    return flask.render_template('index.html')


@app.route("/api/dashboard_update")
def api_dashboard_update():
    return flask.jsonify(pycontrol_app.get_dashboard_data()), 200


@app.route('/api/camera/update_description', methods=['POST'])
def api_camera_update_description():
    if not flask.request.is_json:
        return make_response("error", "Request must be JSON", 400)
    data = flask.request.get_json()
    serial = data.get('serial')
    cam_id = data.get('description')
    if not serial or cam_id is None:
        return make_response("Failure", "Missing serial or description", 400)
    return pycontrol_app.set_camera_id(serial, cam_id)


@app.route('/api/event_list')
def api_event_list():
    filelist = glob.glob("../events/*.event")
    filelist = sorted([os.path.basename(x) for x in filelist])
    return flask.jsonify(filelist), 200


@app.route('/api/event_load', methods=['POST'])
def api_event_load():
    filename = flask.request.get_json().get("filename")
    if not filename:
        return make_response("error", "Filename not provided", 400)

    try:
        data = pycontrol_app.load_event_file(filename)
        app.logger.info(f"loaded event {filename}")
        return flask.jsonify(data), 200
    except Exception as e:
        return make_response("Failure", f"Failed to load event file: {e}", 500)


@app.route('/api/run_sim/defaults')
def api_run_sim_defaults():
    try:
        with open("../config/run_sim.config", "r") as fin:
            all_lines = fin.readlines()
        data = {}
        for line in all_lines:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            tokens = line.split(maxsplit=1)
            if len(tokens) == 2:
                data[tokens[0]] = tokens[1]
            else:
                data[tokens[0]] = ""
        return flask.jsonify(data), 200
    except FileNotFoundError:
        return make_response("Failure", "run_sim.config not found", 404)


@app.route('/api/run_sim')
def api_run_sim():
    gps_latitude = flask.request.args.get('gps_latitude', type=float)
    gps_longitude = flask.request.args.get('gps_longitude', type=float)
    gps_altitude = flask.request.args.get('gps_altitude', type=float)
    event_id = flask.request.args.get('event_id', default="", type=str).lower()
    event_time_offset = flask.request.args.get('event_time_offset', default=None, type=float)

    if any(p is None for p in [gps_latitude, gps_longitude, gps_altitude]):
        return make_response("Failure", "Missing or invalid GPS simulation parameters.", 400)

    return pycontrol_app.start_simulation(
        gps_latitude, gps_longitude, gps_altitude, event_id, event_time_offset
    )



@app.route('/api/run_sim/stop')
def api_run_sim_stop():
    return pycontrol_app.stop_simulation()


@app.route('/api/camera_sequence_list')
def api_camera_sequence_list():
    filelist = glob.glob("../sequences/*.seq")
    filelist = sorted([os.path.basename(x) for x in filelist])
    return flask.jsonify(filelist), 200


@app.route('/api/camera_sequence_load', methods=["POST"])
def api_camera_sequence_load():
    filename = flask.request.get_json().get("filename")
    if not filename:
        return make_response("error", "Filename not provided", 400)

    try:
        # Correctly call the public method
        return pycontrol_app.load_sequence(filename)
    except FileNotFoundError as e:
        return make_response("error", str(e), 404)
    except Exception as e:
        app.logger.error(f"Failed to load camera sequence {filename}: {e}")
        return make_response("error", "Failed to load camera sequence", 500)


JS_TO_PROP = {
    "quality": "imagequality",
    "mode": "expprogram",
    "fstop": "f-number",
    "shutter": "shutterspeed2",
}


@app.route('/api/camera/read_choices', methods=["POST"])
def api_camera_read_choices():
    data = flask.request.get_json()
    serial = data.get('serial')
    prop = data.get('property')

    if prop in JS_TO_PROP:
        prop = JS_TO_PROP[prop]

    return pycontrol_app.read_choices(serial, prop)


@app.route('/api/camera/set_choice', methods=["POST"])
def api_camera_set_choice():
    data = flask.request.get_json()
    serial = data.get('serial')
    prop = data.get('property')
    value = data.get('value')

    if prop in JS_TO_PROP:
        prop = JS_TO_PROP[prop]

    return pycontrol_app.set_choice(serial, prop, value)


@app.route('/api/camera/trigger', methods=["POST"])
def api_camera_trigger():
    data = flask.request.get_json()
    serial = data.get('serial')

    return pycontrol_app.trigger(serial)


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000)
