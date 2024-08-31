import flask

from gps_reader import GpsReader
from cam_info_reader import CameraInfoReader

app = flask.Flask(__name__)


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


if __name__ == '__main__':
    _gps_reader = GpsReader()
    _gps_reader.start()
    _cam_reader = CameraInfoReader()
    _cam_reader.start()
    app.run(debug=True, host="0.0.0.0")
