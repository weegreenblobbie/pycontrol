#include <camera_control/Camera.h>
#include <camera_control/Event.h>

#include <common/io.h>
#include <common/str_utils.h>

// For gphoto2cpp::Event and gphoto2cpp::FileCapture types.
#include <gphoto2cpp/gphoto2cpp.h>

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

Camera::~Camera(){}

void
Camera::_query_props()
{
    INFO_LOG << "Checking for some specific camera properties..." << std::endl;
    std::string value;
    _have_burst_number = _gp2cpp.read_property(_camera, "burstnumber", value);
    _have_capture_mode = _gp2cpp.read_property(_camera, "capturemode", value);
    _have_capturetarget = _gp2cpp.read_property(_camera, "capturetarget", value);
    _have_num_avail = _gp2cpp.read_property(_camera, "availableshots", value);
    _have_shooting_speed = _gp2cpp.read_property(_camera, "shootingspeed", value);

    std::cout
        << "    availableshots: " << _have_num_avail << "\n"
        << "       burstnumber: " << _have_burst_number << "\n"
        << "       capturemode: " << _have_capture_mode << "\n"
        << "     capturetarget: " << _have_capturetarget << "\n"
        << "     shootingspeed: " << _have_shooting_speed << "\n";
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
    else if (property == "shutterspeed")
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

    if (not _gp2cpp.read_property(_camera, "shutterspeed", _info.shutter))
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

    if (_have_capturetarget)
    {
        ABORT_IF_NOT(
            _gp2cpp.read_property(_camera, "capturetarget", _info.capture_target),
            "reading capturetarget failed",
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
    ABORT_IF_NOT(
        _gp2cpp.write_property(_camera, "shutterspeed", _info.shutter),
        "failed to write 'shutterspeed': " << _info.shutter,
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
            "failed to write 'burstnumber': " << _info.burst_number,
            result::failure
        );
    }
    if (_have_capturetarget)
    {
        ABORT_IF_NOT(
            _gp2cpp.write_property(_camera, "capturetarget", _info.capture_target),
            "failed to write 'capturetarget': " << _info.capture_target,
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
Camera::set_capture_target(const std::string & capture_target)
{
    _info.capture_target = capture_target;
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


struct ScopedSettings
{
    explicit ScopedSettings(Camera::Info & info) : _read_write(info), _saved(info) {}
    ~ScopedSettings() {_read_write = _saved;}
private:
    Camera::Info &     _read_write;
    const Camera::Info _saved;
};


result
Camera::capture_histogram()
{
    INFO_LOG << "Starting capture" << std::endl;

    auto before = ScopedSettings(_info);

    // Set camera quality to JPEG.
    //set_quality("JPEG Basic");
    //set_capture_target("sdram");

    ABORT_ON_FAILURE(
        write_config(),
        "failure",
        result::failure
    );

    if (not _hist_capture)
    {
        // Allocate _hist_capture for the firt time.
        _hist_capture = std::move(_gp2cpp.make_file_capture(_camera));

        ABORT_IF(
            _hist_capture == nullptr,
            "GPhoto2Cpp::make_file_capture() failed",
            result::failure
        );
    }

    INFO_LOG << "Triggering JPEG capture" << std::endl;

    ABORT_IF_NOT(
        _hist_capture->capture(),
        "gphoto2cpp::FileCapture::capture() failed",
        result::failure
    );

    unsigned int num_channels = 0;

    INFO_LOG << "Decompressing JPEG capture" << std::endl;

    ABORT_IF_NOT(
        _hist_capture->decompress_jpeg(_pixels, num_channels),
        "gphoto2cpp::FileCapture::decompress_jpeg() failed",
        result::failure
    );

    INFO_LOG << "JPEG capture complete!\n"
             << "    " << _pixels.size() << " pixels\n"
             << "    " << num_channels << " channels" << std::endl;

    ABORT_IF_NOT(
        _hist_capture->delete_last_capture(),
        "gphoto2cpp::FileCapture::delete_last_capture() failed",
        result::failure
    );

    // Compute histogram here!

    INFO_LOG << "Computing histogram" << std::endl;

    // Reset with zeros.
    _hist.assign(256, 0);

    // Black and white capture?
    if (num_channels == 1)
    {
        for (const auto pix : _pixels)
        {
            ++_hist[pix];
        }
    }

    // 3 channel color.
    else if (num_channels == 3)
    {
        for (std::size_t i = 0; i < _pixels.size(); i += 3)
        {
            /*
            These coefficients are defined by the International Electrotechnical
            Commission (IEC) standard, specifically IEC 61966-2-1:1999 for the
            sRGB color space. The formula is used to convert linear RGB values
            into a single luminance value.

            auto r = _pixels[i];
            auto g = _pixels[i + 1];
            auto b = _pixels[i + 2];
            auto luminance = static_cast<unsigned long>(
                0.2126f * r + 0.7152f * g + 0.0722f * b);
            */

            // Using fixed-point arithmetic for the above to avoid expensive
            // float operations by multipling the coefficients by 65536 (2**16),
            // then sumimng up and shifting the result down by 16 bits to
            // normalize.
            std::uint32_t r = _pixels[i];
            std::uint32_t g = _pixels[i + 1];
            std::uint32_t b = _pixels[i + 2];
            std::uint32_t luminance = (13933 * r + 46871 * g + 4732 * b) >> 16;

            ++_hist[luminance];
        }
    }

    return result::success;
}

float
Camera::shutter_speed(const std::string & value) const
{
    const auto ss = value.empty() ? _info.shutter : value;

    if (ss.empty() or ss == "bulb" or ss == "time" or ss == "x 200")
    {
        return 0.0;
    }

    auto slash_pos = ss.find('/');

    try
    {
        // numerator / demonimator representation.
        if (slash_pos != std::string::npos)
        {
            float numerator = std::stof(ss.substr(0, slash_pos));
            float denominator = std::stof(ss.substr(slash_pos + 1));

            if (denominator == 0.0f)
            {
                INFO_LOG << "Failed to parse shutter speed: " << ss << std::endl;
                return 0.0f;
            }

            return numerator / denominator;
        }
        // A while number, i.e. 10
        else
        {
            return std::stof(ss);
        }
    }
    catch (const std::exception& e)
    {
        INFO_LOG << "Excpetion thrown during parsing of shutter speed: " << ss << std::endl;
    }

    return 0.0f;
}

unsigned int
Camera::iso(const std::string & value) const
{
    const auto iso_ = value.empty() ? _info.iso : value;

    std::stringstream ss;
    ss << iso_;

    unsigned int out;
    ss >> out;

    return out;
}

void
Camera::step_shutter_speed(int steps)
{
    _step_camera_property("shutterspeed2", steps);
}

void
Camera::step_iso(int steps)
{
    _step_camera_property("iso", steps);
}

void
Camera::_step_camera_property(const std::string & property, int step)
{
    if (step == 0)
    {
        return;
    }
    std::string tmp;

    if (property == "shutterspeed2")
    {
        tmp = _info.shutter;
    }
    else if (property == "iso")
    {
        tmp = _info.iso;
    }
    else
    {
        ERROR_LOG << "Unhandeled property: " << property << std::endl;
        return;
    }

    const std::string current = tmp;
    const auto all_choices = read_choices(property);

    if (all_choices.empty())
    {
        ERROR_LOG << "read_choices(\"" << property << "\") is empty!" << std::endl;
        return;
    }

    int idx = 0;
    for (const auto & choice : all_choices)
    {
        if (choice == current)
        {
            break;
        }
        ++idx;
    }

    if (static_cast<std::size_t>(idx) >= all_choices.size())
    {
        ERROR_LOG << "Could not find index for '" << current << "' in read_choices(\"" << property << "\")" << std::endl;
        return;
    }

    auto new_index = idx + step;

    if (new_index < 0) new_index = 0;
    if (static_cast<std::size_t>(new_index) >= all_choices.size()) new_index = all_choices.size() - 1;

    if (property == "shutterspeed" or property == "shutterspeed2")
    {
        set_shutter(all_choices[new_index]);
    }
    else if (property == "iso")
    {
        set_iso(all_choices[new_index]);
    }
}

} /* namespace pycontrol */
