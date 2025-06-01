import glob
import flask
import logging
import os.path

import datetime


import date_utils as du
from cam_info_reader import CameraInfoReader
from event_solver import EventSolver
from gps_reader import GpsReader

app = flask.Flask(__name__)
app.config['DEBUG'] = True
logging.getLogger('werkzeug').setLevel(logging.ERROR)


@app.route('/')
def hello():
    return flask.render_template(
        'index.html',
        gps_status=_gps_reader.read(),
        cam_status=_cam_reader.read(),
    )


@app.route('/api/gps')
def gps():
    return flask.jsonify(_gps_reader.read())


@app.route('/api/cameras')
def cameras():
    return flask.jsonify(_cam_reader.read())


@app.route('/api/camera/update_description', methods=['POST'])
def camera_update_description():
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
def get_event_list():
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


@app.route('/api/event_load/<filename>')
def get_event(filename):
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

    # Trigger the event solver.
    gps = _gps_reader.read()

    _event_solver.update_and_trigger(
        type=type_,
        datetime=date_,
        lat=gps["lat"],
        long=gps["long"],
        altitude=gps["altitude"],
        events=data["events"],
        event=data["event"],
    )

    data.pop("event")
    return (
        flask.jsonify(data),
        200
    )


@app.route('/api/events')
def get_events():
    """
    Asks the event solver for the events, and then we return a map:

    event_id: (abs time, relative time)
    """
    events = _event_solver.read()
    out = dict()
    for event_id in events:
        event_time = events[event_id]

        if event_time is None:
            out[event_id] = ("Event not visible!", "N/A")

        elif event_time == EventSolver.COMPUTING:
            out[event_id] = (event_time, "")

        elif isinstance(event_time, datetime.datetime):
            eta = du.eta(du.now(), event_time)
            out[event_id] = (du.normalize(event_time), eta)

        else:
            print(f"event_time: {event_time}")
            raise ValueError(f"event_time: {event_time}")

    return (
        flask.jsonify(out),
        200
    )


if __name__ == '__main__':
    _gps_reader = GpsReader()
    _gps_reader.start()
    _cam_reader = CameraInfoReader("../config/camera_descriptions.config")
    _cam_reader.start()
    _event_solver = EventSolver()
    _event_solver.start()
    app.run(host="0.0.0.0")
