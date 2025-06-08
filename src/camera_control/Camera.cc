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
    _camera.reset();
    _camera = camera;
    _info.port = port;
    _info.connected = true;
}


void
Camera::
disconnect()
{
    _info.connected = false;
    _info.battery_level =
    _info.shutter =
    _info.mode =
    _info.fstop =
    _info.iso =
    _info.quality =
    _info.num_photos = "__ERROR__";
}


void
Camera::fetch_settings()
{
    if (not _info.connected) return;

    _info.battery_level = gphoto2cpp::read_property(_camera, "batterylevel");
    _info.shutter = gphoto2cpp::read_property(_camera, "shutterspeed2");
    if (_info.shutter == "__ERROR__")
    {
        disconnect();
        return;
    }

    _info.mode = gphoto2cpp::read_property(_camera, "expprogram");
    _info.fstop = gphoto2cpp::read_property(_camera, "f-number");
    _info.iso = gphoto2cpp::read_property(_camera, "iso");
    _info.quality = gphoto2cpp::read_property(_camera, "imagequality");
    _info.num_photos = gphoto2cpp::read_property(_camera, "availableshots");
}

result
Camera::write_settings()
{
    if (_stale_shutter)
    {
        bool success = gphoto2cpp::write_property(
            _camera,
            "shutterspeed2",
            _info.shutter
        );

        ABORT_IF_NOT(
            success,
            "failed to write shutterspeed2: " << _info.shutter,
            result::failure
        );

        _stale_shutter = false;
    }

    if (_stale_fstop)
    {
        bool success = gphoto2cpp::write_property(
            _camera,
            "f-nmber",
            _info.fstop
        );

        ABORT_IF_NOT(
            success,
            "failed to write f-number: " << _info.fstop,
            result::failure
        );

        _stale_fstop = false;
    }

    if (_stale_iso)
    {
        bool success = gphoto2cpp::write_property(
            _camera,
            "iso",
            _info.iso
        );

        ABORT_IF_NOT(
            success,
            "failed to write iso: " << _info.iso,
            result::failure
        );

        _stale_iso = false;
    }

    if (_stale_quality)
    {
        bool success = gphoto2cpp::write_property(
            _camera,
            "imagequality",
            _info.quality
        );

        ABORT_IF_NOT(
            success,
            "failed to write imagequality: " << _info.quality,
            result::failure
        );

        _stale_quality = false;
    }

    return result::success;
}


result
Camera::trigger()
{
    ABORT_IF(
        _stale_shutter or _stale_fstop or _stale_iso or _stale_quality,
        "Stale camera settings, please call write_settings() first",
        result::failure
    );

    ABORT_IF_NOT(
        gphoto2cpp::trigger(_camera),
        "failed to trigger camera",
        result::failure
    );

    return result::success;
}



void
Camera::set_shutter(const std::string & speed)
{
    _stale_shutter = _stale_shutter or _info.shutter != speed;
    _info.shutter = speed;
}


void
Camera::set_fstop(const std::string & fstop)
{
    _stale_fstop = _stale_fstop or _info.fstop != fstop;
    _info.fstop = fstop;
}


void
Camera::set_iso(const std::string & iso)
{
    _stale_iso = _stale_iso or _info.iso != iso;
    _info.iso = iso;
}

void
Camera::set_quality(const std::string & quality)
{
    _stale_quality = _stale_quality or _info.quality != quality;
    _info.quality = quality;
}


result
Camera::handle(const Event & event)
{
    switch(event.channel)
    {
        case Channel::fps:
        {
            ABORT_IF(true, "fps not implemented yet", result::failure);
            break;
        }
        case Channel::fstop:
        {
            set_fstop(event.channel_value);
            break;
        }
        case Channel::iso:
        {
            set_iso(event.channel_value);
            break;
        }
        case Channel::quality:
        {
            set_quality(event.channel_value);
            break;
        }
        case Channel::shutter_speed:
        {
            set_shutter(event.channel_value);
            break;
        }
        case Channel::trigger:
        {
            ABORT_ON_FAILURE(
                 gphoto2cpp::trigger(_camera),
                 "failed to trgger camera",
                 result::failure
             );
            break;
        }
    }

    return result::success;
}


}
