import copy
import dataclasses
import datetime
import flask
import glob
import logging
import os.path
import threading
import time

import date_utils as du
from cam_info_reader import CameraInfoReader
from event_solver import EventSolver
from gps_reader import GpsReader

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


app = flask.Flask(__name__)
app.config['DEBUG'] = True
logging.getLogger('werkzeug').setLevel(logging.ERROR)

_gps_reader = GpsReader()
_gps_reader.start()
_cam_reader = CameraInfoReader("../config/camera_descriptions.config")
_cam_reader.start()
_event_solver = EventSolver()
_event_solver.start()
_run_sim = RunSim()
_run_sim_lock = threading.Lock()


@app.route('/')
def hello():
    return flask.render_template(
        'index.html',
        gps_status=_gps_reader.read(),
        cam_status=_cam_reader.read(),
    )


@app.route('/api/gps')
def api_gps():
    return flask.jsonify(_gps_reader.read())


@app.route('/api/cameras')
def api_cameras():
    return flask.jsonify(_cam_reader.read())


@app.route('/api/camera/update_description', methods=['POST'])
def api_camera_update_description():
    if not flask.request.is_json:
        return ('{"status":"error","message":"Request must be JSON"}', 400)

    data = flask.request.get_json()

    serial = data.get('serial')
    new_description = data.get('description')

    if not serial or not new_description:
        return (
            '{"status":"error","message":"Missing serial or description"}',
            400
        )

    _cam_reader.update_description(serial, new_description)
    return (
        '{"status":"success","message":"Description updated"}',
        200
    )

@app.route('/api/event_list')
def api_event_list():
    """
    Returns a filelist of saved event files in the raspberry pi.
    """
    filelist = glob.glob("../events/*.event")
    app.logger.info(f"{filelist}")
    filelist = [os.path.basename(x) for x in filelist]
    app.logger.info(f"{filelist}")
    return (
        flask.jsonify(filelist),
        200
    )


def update_and_trigger(type_, date_, event_ids, event_map):

    global _gps_reader
    global _run_sim_lock
    global _run_sim
    global _event_solver

    # Read the current GPS location.
    gps = _gps_reader.read()
    latitude = gps["lat"]
    longitude = gps["long"]
    altitude = gps["altitude"]

    # Check if a simulation is running and replace the GPS location with the
    # simulated location.
    with _run_sim_lock:
        sim_is_running = _run_sim.is_running
        sim_pos = copy.deepcopy(_run_sim.gps_pos)
        sim_offset = copy.deepcopy(_run_sim.gps_offset)

    if sim_is_running:
        """
        Compute the GPS delta since the simulation started and add it to the
        simulated GPS location for realistc behavior in the field where the GPS
        receiver continutally updates its position.
        """
        latitude = sim_pos.latitude + (sim_offset.latitude - latitude)
        longitude = sim_pos.longitude + (sim_offset.longitude - longitude)
        altitude = sim_pos.altitude + (sim_offset.altitude - altitude)

    _event_solver.update_and_trigger(
        type=type_,
        datetime=date_,
        lat=latitude,
        long=longitude,
        altitude=altitude,
        event_ids=event_ids,
        event=event_map,
    )


@app.route('/api/event_load/<filename>')
def api_event_load(filename):
    """
    Loads an event file and configures the contact solver.
    """
    with open(os.path.join("../events", filename), "r") as fin:
        all_lines = fin.readlines()

    date_ = None
    data = dict(event=dict())
    event_ids = None
    for line in all_lines:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        tokens = line.split()
        key = tokens[0]
        values = tokens[1:]

        if key == "type":
            type_ = values[0]
            assert type_ in {"solar", "lunar", "custom"}
            data["type"] = type_

        elif key == "date":
            date_ = du.make_datetime(values[0])
            continue

        elif key == "events":
            event_ids = values
            data["events"] = values

        elif event_ids and key in event_ids:
            if "event" not in data:
                data["event"] = dict()
            data["event"][key] = du.make_datetime(values[0])

    update_and_trigger(data["type"], date_, data["events"], data.get("event", dict()))

    data.pop("event")
    return (
        flask.jsonify(data),
        200
    )


@app.route('/api/events')
def api_events():
    """
    Asks the event solver for the events, and then we return a map:

    event_id: (abs time, relative time)
    """
    global _event_solver
    global _run_sim_lock
    global _run_sim

    events = _event_solver.read()
    all_events = _event_solver.event_ids()

    with _run_sim_lock:
        sim_time_offset = copy.deepcopy(_run_sim.sim_time_offset)

    out = dict()
    for event_id in all_events:
        event_time = events.get(event_id, EventSolver.COMPUTING)

        if event_time == EventSolver.COMPUTING:
            out[event_id] = (event_time, "")

        elif event_time is None:
            out[event_id] = ("Event not visible!", "N/A")

        elif isinstance(event_time, datetime.datetime):
            event_time += sim_time_offset
            eta = du.eta(du.now(), event_time)
            out[event_id] = (du.normalize(event_time), eta)

        else:
            print(f"event_time: {event_time}")
            raise ValueError(f"event_time: {event_time}")

    return (
        flask.jsonify(out),
        200
    )


@app.route('/api/run_sim/defaults')
def api_run_sim_defaults():
    with open("../config/run_sim.config", "r") as fin:
        all_lines = fin.readlines()
    data = dict()
    for line in all_lines:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        key, value = line.split()
        data[key] = value

    return (flask.jsonify(data), 200)


@app.route('/api/run_sim')
def api_run_sim():

    # Extract parameters from query string.
    gps_latitude = flask.request.args.get('gps_latitude', type=float)
    gps_longitude = flask.request.args.get('gps_longitude', type=float)
    gps_altitude = flask.request.args.get('gps_altitude', type=float)
    event_id = flask.request.args.get('event_id', type=str)
    event_time_offset = datetime.timedelta(seconds=flask.request.args.get('event_time_offset', type=float))

    # Basic validation.
    if any(p is None for p in [gps_latitude, gps_longitude, gps_altitude, event_id, event_time_offset]):
        return (
            flask.jsonify({"status": "error", "message": "Missing or invalid simulation parameters."}),
            400
        )

    global _event_solver
    global _gps_reader
    global _run_sim
    global _run_sim_lock

    with _run_sim_lock:
        is_running = _run_sim.is_running

    if is_running:
        return (
            flask.jsonify({"status": "error", "message": "Simulation is already running."}),
            409
        )

    all_event_ids = _event_solver.event_ids()
    if event_id not in all_event_ids:
        return (
            flask.jsonify({"status": "error", "message": f"Bad event_id {event_id}"}),
            400
        )

    # We have a bit of a checking and egg problem here.  Most likly, you are not
    # currently in the path of totality, so there will not be any contact times
    # avialable to know how to shift the clock, so we reset the event solver
    # with the simulated gps position to compute conact times if avialable.
    solution = _event_solver.simulate_trigger(gps_latitude, gps_longitude, gps_altitude)
    app.logger.info(f"run_sim: solution = {solution}")

    event_time = solution.get(event_id)
    if event_time is None:
        return (
            flask.jsonify({"status": "error", "message": f"No event time for {event_id}"}),
            409
        )

    # Compute the sim time offset, when applied, all the computed event times
    # shift to the current system time, which in turn, makes the camera control
    # believe the events are upcoming without changing system time.
    #
    # (event_time + sim_time_offset) + event_time_offset == current_time
    #
    # Now solve for sim_time_offset:
    #
    # sim_time_offset = current_time - event_time - event_time_offset
    #
    sim_time_offset = du.now() - event_time - event_time_offset

    # Grab current gps position for computing gps residual deltas.
    snapshot = _gps_reader.read()
    gps_offset = GpsPos(
        latitude = snapshot["lat"],
        longitude = snapshot["long"],
        altitude = snapshot["altitude"],
    )

    # Construct the simulated gps position.
    gps_pos = GpsPos(
        latitude = gps_latitude,
        longitude = gps_longitude,
        altitude = gps_altitude,
    )

    # Construct a new RunSim object.
    with _run_sim_lock:
        _run_sim = RunSim(
            is_running = True,
            gps_pos = gps_pos,
            gps_offset = gps_offset,
            event_id = event_id,
            event_time_offset = event_time_offset,
            sim_time_offset = sim_time_offset,
        )

    params = _event_solver.params()

    update_and_trigger(params["type"], params["datetime"], params["event_ids"], params["event"])

    with open("../config/run_sim.config", "w") as fout:
        """
        gps_latitude            40.918959
        gps_longitude           -1.289364
        gps_altitude           950.0
        event_id                C2
        event_time_offset      -68.0
        """
        fout.write(f"gps_latitude {gps_latitude}\n")
        fout.write(f"gps_longitude {gps_longitude}\n")
        fout.write(f"gps_altitude {gps_altitude}\n")
        fout.write(f"event_id {event_id}\n")
        fout.write(f"event_time_offset {flask.request.args.get('event_time_offset', type=float)}\n")

    return (
        flask.jsonify({"status": "success", "message": "Simulation started."}),
        200
    )


@app.route('/api/run_sim/stop')
def api_run_sim_stop():
    global _run_sim
    with _run_sim_lock:
        _run_sim = RunSim()

    return (
        flask.jsonify({"status": "success", "message": "Simulation stopped."}),
        200
    )


if __name__ == '__main__':
    app.run(host="0.0.0.0")
