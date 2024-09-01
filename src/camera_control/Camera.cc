#include <common/io.h>
#include <camera_control/Camera.h>

namespace pycontrol
{

Camera::
Camera(
    gphoto2cpp::camera_ptr & camera,
    const std::string & port,
    const std::string & serial,
    const std::string & config_file)
:
    _stale_shutter{true},
    _stale_fstop{true},
    _stale_iso{true},
    _info{.connected = true, .serial = serial, .port = port, },
    _camera{camera}
{
    _info.desc = gphoto2cpp::read_property(_camera, "manufacturer") + " "
               + gphoto2cpp::read_property(_camera, "cameramodel");
}


void
Camera::reconnect(gphoto2cpp::camera_ptr & camera, const std::string & port)
{
    _camera = camera;
    _info.port = port;
}

void
Camera::fetch_settings()
{
    _info.shutter = gphoto2cpp::read_property(_camera, "shutterspeed2");
    _info.battery_level = gphoto2cpp::read_property(_camera, "batterylevel");
    _info.mode = gphoto2cpp::read_property(_camera, "expprogram");
    _info.fstop = gphoto2cpp::read_property(_camera, "f-number");
    _info.iso = gphoto2cpp::read_property(_camera, "iso");
    _info.quality = gphoto2cpp::read_property(_camera, "imagequality");
    _info.num_photos = gphoto2cpp::read_property(_camera, "availableshots");
}


}
