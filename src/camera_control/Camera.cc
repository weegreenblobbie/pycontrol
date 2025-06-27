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
    _info{.connected = true, .serial = serial, .port = port, },
    _camera{camera}
{
    gphoto2cpp::read_config(_camera);

    std::string make;
    std::string model;

    gphoto2cpp::read_property(_camera, "manufacturer", make);
    gphoto2cpp::read_property(_camera, "cameramodel", model);

    _info.desc = make + " " + model;
}


void
Camera::reconnect(gphoto2cpp::camera_ptr & camera, const std::string & port)
{
    gphoto2cpp::reset_cache(camera);
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
    _info.quality = "__ERROR__";
    _info.num_photos = "-1";
}


std::vector<std::string>
Camera::
read_choices(const std::string & property) const
{
    std::vector<std::string> out;
    for (auto & choice : gphoto2cpp::read_choices(_camera, property))
    {
        out.emplace_back(choice);
    }
    return out;
}

result
Camera::read_config()
{
    if (not _info.connected) return result::success;

    if (not gphoto2cpp::read_config(_camera))
    {
        disconnect();
        return result::success;
    }

    // Battery can be read event if camera isn't turned on.
    ABORT_IF_NOT(
        gphoto2cpp::read_property(_camera, "batterylevel", _info.battery_level),
        "reasing batterylevel failed",
        result::failure
    );

    if (not gphoto2cpp::read_property(_camera, "shutterspeed2", _info.shutter))
    {
        disconnect();
        return result::success;
    }

    ABORT_IF_NOT(
        gphoto2cpp::read_property(_camera, "expprogram", _info.mode),
        "reading mode failed",
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::read_property(_camera, "f-number", _info.fstop),
        "reading fstop failed",
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::read_property(_camera, "iso", _info.iso),
        "reading iso failed",
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::read_property(_camera, "imagequality", _info.quality),
        "reading quality failed",
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::read_property(_camera, "availableshots", _info.num_photos),
        "reading avialble shots failed",
        result::failure
    );

    return result::success;
}

result
Camera::write_config()
{
    ABORT_IF_NOT(
        gphoto2cpp::write_property(_camera, "shutterspeed2", _info.shutter),
        "failed to write 'shutterspeed2': " << _info.shutter,
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::write_property(_camera, "f-number", _info.fstop),
        "failed to write 'f-number': " << _info.fstop,
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::write_property(_camera, "iso", _info.iso),
        "failed to write 'iso': " << _info.iso,
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::write_property(_camera, "imagequality", _info.quality),
        "failed to write 'imagequality': " << _info.quality,
        result::failure
    );
    ABORT_IF_NOT(
        gphoto2cpp::write_config(_camera),
        "failed to flush settings",
        result::failure
    );

    return result::success;
}


void
Camera::set_shutter(const std::string & speed)
{
    _info.shutter = speed;
}


void
Camera::set_fstop(const std::string & fstop)
{
    _info.fstop = fstop;
}


void
Camera::set_iso(const std::string & iso)
{
    _info.iso = iso;
}

void
Camera::set_quality(const std::string & quality)
{
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
            return trigger();
        }
        case Channel::null:
        {
            ERROR_LOG << "Got Channel::null!" << std::endl;
            return result::failure;
            break;
        }
    }

    return result::success;
}


result
Camera::trigger()
{
    ABORT_IF_NOT(
        gphoto2cpp::trigger(_camera),
        "failed to trigger camera",
        result::failure
    );

    return result::success;
}


} /* namespace */
