#include <camera_control/Camera.h>
#include <camera_control/Event.h>

#include <common/io.h>
#include <common/str_utils.h>

#include <gphoto2cpp/gphoto2cpp.h> // For Event type.

namespace pycontrol
{

Camera::
Camera(
    interface::GPhoto2Cpp & gp2cpp,
    gphoto2cpp::camera_ptr & camera,
    const std::string & port,
    const std::string & serial,
    const std::string & config_file)
:
    _gp2cpp(gp2cpp),
    _camera{camera},
    _info{.connected = true, .serial = serial, .port = port}
{
    // Expensive read, reads all the camera's configuration.
    _gp2cpp.read_config(_camera);

    std::string make;
    std::string model;

    _gp2cpp.read_property(_camera, "manufacturer", make);
    _gp2cpp.read_property(_camera, "cameramodel", model);
    _info.desc = make + " " + model;

    _query_props();
}

void
Camera::_query_props()
{
    INFO_LOG << "Checking for some specific camera properties..." << std::endl;
    std::string value;
    _have_shutterspeed2 = _gp2cpp.read_property(_camera, "shutterspeed2", value);
    _have_num_avail = _gp2cpp.read_property(_camera, "availableshots", value);
    _have_burst_number = _gp2cpp.read_property(_camera, "burstnumber", value);
    _have_capture_mode = _gp2cpp.read_property(_camera, "capturemode", value);
    _have_shooting_speed = _gp2cpp.read_property(_camera, "shootingspeed", value);

    std::cout
        << "    avialableshots: " << _have_num_avail << "\n"
        << "       burstnumber: " << _have_burst_number << "\n"
        << "       capturemode: " << _have_capture_mode << "\n"
        << "     shootingspeed: " << _have_shooting_speed << "\n"
        << "     shutterspeed2: " << _have_shutterspeed2 << std::endl;
}


void
Camera::reconnect(gphoto2cpp::camera_ptr & camera, const std::string & port)
{
    _camera = camera;
    _info.port = port;
    _info.connected = true;
    _query_props();
}


void
Camera::
disconnect()
{
    auto serial = _info.serial;
    auto port = _info.port;
    _info = Info();
    _info.serial = serial;
    _info.port = port;
}


std::vector<std::string>
Camera::
read_choices(const std::string & property) const
{
    std::vector<std::string> out;
    for (auto & choice : _gp2cpp.read_choices(_camera, property))
    {
        out.emplace_back(choice);
    }
    return out;
}

result
Camera::
write_property(const std::string & property, const std::string & value)
{
    if (property == "burstnumber")
    {
        set_burst_number(value);
    }
    else if (property == "capturemode")
    {
        set_capture_mode(value);
    }
    else if (property == "expprogram")
    {
        set_mode(value);
    }
    else if (property == "f-number")
    {
        set_fstop(value);
    }
    else if (property == "iso")
    {
        set_iso(value);
    }
    else if (property == "imagequality")
    {
        set_quality(value);
    }
    else if (property == "shootingspeed")
    {
        set_shooting_speed(value);
    }
    else if (property == "shutterspeed" or property == "shutterspeed2")
    {
        set_shutter(value);
    }
    else
    {
        ERROR_LOG << "Can't set '" << property << "'" << std::endl;
        return result::failure;
    }
    return result::success;
}

result
Camera::read_config()
{
    if (not _info.connected) return result::success;

    // Big, expesive camera state fetch.
    if (not _gp2cpp.read_config(_camera))
    {
        disconnect();
        return result::success;
    }

    // Battery can be read event if camera isn't turned on.
    ABORT_IF_NOT(
        _gp2cpp.read_property(_camera, "batterylevel", _info.battery_level),
        "reasing batterylevel failed",
        result::failure
    );

    std::string shutterspeed = "shutterspeed";
    if (_have_shutterspeed2)
    {
        shutterspeed += "2";
    }
    if (not _gp2cpp.read_property(_camera, shutterspeed, _info.shutter))
    {
        disconnect();
        return result::success;
    }

    ABORT_IF_NOT(
        _gp2cpp.read_property(_camera, "expprogram", _info.mode),
        "reading mode failed",
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.read_property(_camera, "f-number", _info.fstop),
        "reading fstop failed",
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.read_property(_camera, "iso", _info.iso),
        "reading iso failed",
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.read_property(_camera, "imagequality", _info.quality),
        "reading quality failed",
        result::failure
    );

    if (_have_num_avail)
    {
        std::string num_avail;
        ABORT_IF_NOT(
            _gp2cpp.read_property(_camera, "availableshots", num_avail),
            "reading avialbleshots failed",
            result::failure
        );
        ABORT_ON_FAILURE(
            as_type<int>(num_avail, _info.num_avail),
            "failure",
            result::failure
        );
    }

    if (_have_burst_number)
    {
        std::string burst_number;
        ABORT_IF_NOT(
            _gp2cpp.read_property(_camera, "burstnumber", burst_number),
            "reading burstnumber failed",
            result::failure
        );
        ABORT_ON_FAILURE(
            as_type<int>(burst_number, _info.burst_number),
            "failure",
            result::failure
        );
    }

    if (_have_capture_mode)
    {
        ABORT_IF_NOT(
            _gp2cpp.read_property(_camera, "capturemode", _info.capture_mode),
            "reading capturemode failed",
            result::failure
        );
    }

    if (_have_shooting_speed)
    {
        ABORT_IF_NOT(
            _gp2cpp.read_property(_camera, "shootingspeed", _info.shooting_speed),
            "reading shootingspeed failed",
            result::failure
        );
    }

    ABORT_ON_FAILURE(
        drain_events(),
        "failure",
        result::failure
    );

    return result::success;
}


result
Camera::drain_events()
{
    // Drain the camera event queue, in order to count photos taken.
    bool have_events = true;
    while (have_events)
    {
        gphoto2cpp::Event event;
        int timeout_ms = 1;

        ABORT_IF_NOT(
            _gp2cpp.wait_for_event(_camera, timeout_ms, event),
            "failure",
            result::failure
        );

        switch (event.type)
        {
            case GP2::GP_EVENT_FILE_ADDED:
            {
                ++_info.num_photos;
                break;
            }
            case GP2::GP_EVENT_TIMEOUT:
            {
                have_events = false;
                break;
            }
            default:
            {
                // Ignore all other events.
                break;
            }
        }
    }

    return result::success;
}

result
Camera::write_config()
{
    std::string shutterspeed = "shutterspeed";
    if (_have_shutterspeed2)
    {
        shutterspeed += "2";
    }
    ABORT_IF_NOT(
        _gp2cpp.write_property(_camera, shutterspeed, _info.shutter),
        "failed to write '" << shutterspeed << "': " << _info.shutter,
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.write_property(_camera, "expprogram", _info.mode),
        "failed to write 'expprogram': " << _info.mode,
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.write_property(_camera, "f-number", _info.fstop),
        "failed to write 'f-number': " << _info.fstop,
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.write_property(_camera, "iso", _info.iso),
        "failed to write 'iso': " << _info.iso,
        result::failure
    );
    ABORT_IF_NOT(
        _gp2cpp.write_property(_camera, "imagequality", _info.quality),
        "failed to write 'imagequality': " << _info.quality,
        result::failure
    );
    if (_have_burst_number)
    {
        ABORT_IF_NOT(
            _gp2cpp.write_property(_camera, "burstnumber", std::to_string(_info.burst_number)),
            "failed to write 'burstnumber': " << _info.quality,
            result::failure
        );
    }
    ABORT_IF_NOT(
        _gp2cpp.write_config(_camera),
        "failed to flush settings",
        result::failure
    );

    ABORT_ON_FAILURE(drain_events(), "failure", result::failure);

    return result::success;
}


void
Camera::set_shutter(const std::string & speed)
{
    _info.shutter = speed;
}


void
Camera::set_mode(const std::string & mode)
{
    _info.mode = mode;
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

void
Camera::set_burst_number(const std::string & burst_number)
{
    as_type<int>(burst_number, _info.burst_number);
}

void
Camera::set_capture_mode(const std::string & capture_mode)
{
    _info.capture_mode = capture_mode;
}

void
Camera::set_shooting_speed(const std::string & shooting_speed)
{
    _info.shooting_speed = shooting_speed;
}



result
Camera::handle(const Event & event)
{
    switch(event.channel)
    {
        case Channel::burst_number:
        {
            set_burst_number(event.channel_value);
            break;
        }
        case Channel::capture_mode:
        {
            set_capture_mode(event.channel_value);
            break;
        }
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
        case Channel::mode:
        {
            set_mode(event.channel_value);
            break;
        }
        case Channel::quality:
        {
            set_quality(event.channel_value);
            break;
        }
        case Channel::shooting_speed:
        {
            set_shooting_speed(event.channel_value);
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
        _gp2cpp.trigger(_camera),
        "failed to trigger camera",
        result::failure
    );

    return result::success;
}


} /* namespace pycontrol */
