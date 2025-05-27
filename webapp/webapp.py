import flask

from gps_reader import GpsReader
from cam_info_reader import CameraInfoReader

app = flask.Flask(__name__)
app.config['DEBUG'] = True

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

if __name__ == '__main__':
    _gps_reader = GpsReader()
    _gps_reader.start()
    _cam_reader = CameraInfoReader("../data/camera_descriptions.yaml")
    _cam_reader.start()
    app.run(host="0.0.0.0")
