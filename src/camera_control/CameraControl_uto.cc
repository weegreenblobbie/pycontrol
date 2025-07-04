#include <camera_control/CameraControl_uto.h>

void
UtoSocket::reset()
{
    _from_send = {};
}

result
UtoSocket::recv(std::string & out)
{
    out = _to_recv;
    return result::success;
}

result
UtoSocket::send(const std::string & out)
{
    // CameraControl is using a std::stringstream with a reserved capacity
    // of something like 4048 chars.  std::stringstream.str() will return
    // a string with the same capacity.  But our real UdpSocket::send()
    // implementation uses strlen(char *), so we construct a std::string
    // from a char * to "read" only the first, null-terminated string.
    _from_send.emplace_back(out.c_str());
    return result::success;
}

void
UtoSocket::to_recv(const std::string & message)
{
    _to_recv = message;
}

str_vec &
UtoSocket::from_send()
{
    return _from_send;
}

test_camera_ptr
make_test_camera(
    const std::string & model,
    const std::string & port,
    const std::string & serial)
{
    auto tc = std::make_shared<TestCamera>();

    tc->maker = "Nikon Corporation";
    tc->model = model;
    tc->connected = true;
    tc->serial = serial;
    tc->port = port;
    tc->desc = tc->maker + " " + tc->model;
    tc->mode = "M";
    tc->shutter = "1/1000";
    tc->fstop = "F/8";
    tc->iso = "64";
    tc->quality = "NEF (Raw)";
    tc->batt_level = "100%";
    tc->num_photos = "850";

    tc->choice_map = {
        {"expprogram", {"M", "A", "S", "P"}},
        {"f-number", {"f/8", "f/5.6", "f/4"}},
        {"imagequality", {"NEF (Raw)", "JPEG Basic", "JPEG Fine"}},
        {"iso", {"64", "100", "200", "500"}},
        {"shutterspeed2", {"1/1000", "1/500", "1/250"}},
    };

    return tc;
}


void
UtoGp2Cpp::add_camera(test_camera_ptr cam)
{
    if (not cam)
    {
        throw std::runtime_error("cam is null");
    }
    _test_cams.insert(cam);
}

str_vec
UtoGp2Cpp::auto_detect()
{
    str_vec out;
    for (auto & cam : _test_cams)
    {
        if (cam->connected)
        {
            out.push_back(cam->port);
        }
    }
    return out;
}

camera_ptr
UtoGp2Cpp::open_camera(const std::string & port)
{
    for (auto & test_cam : _test_cams)
    {
        if (test_cam->port == port)
        {
            auto ptr = std::make_shared<GP2::Camera>();
            _camera_to_test[ptr] = test_cam;
            test_cam->open_count++;
            return ptr;
        }
    }
    return nullptr;
}

str_vec
UtoGp2Cpp::read_choices(const camera_ptr & camera, const std::string & property)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->read_choices_count++;
    return test_cam->choice_map[property];
}

bool
UtoGp2Cpp::read_config(const camera_ptr & camera)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->read_config_count++;
    return test_cam->read_config_result;
}

bool
UtoGp2Cpp::read_property(
    const camera_ptr & camera,
    const std::string & property,
    std::string & output)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->read_property_count++;

    if (property == "manufacturer")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->maker;
        }
    }

    else if (property == "cameramodel")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->model;
        }
    }

    else if (property == "serialnumber")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->serial;
        }
    }

    else if (property == "batterylevel")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->batt_level;
        }
    }

    else if (property == "shutterspeed2")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->shutter;
        }
    }

    else if (property == "expprogram")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->mode;
        }
    }

    else if (property == "f-number")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->fstop;
        }
    }

    else if (property == "iso")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->iso;
        }
    }

    else if (property == "imagequality")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->quality;
        }
    }

    else if (property == "availableshots")
    {
        if(test_cam->read_property_result)
        {
            output = test_cam->num_photos;
        }
    }
    else
    {
        return false;
    }

    return test_cam->read_property_result;
}

void
UtoGp2Cpp::reset_cache(const camera_ptr & camera)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->reset_cache_count++;
}

bool
UtoGp2Cpp::trigger(const camera_ptr & camera)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->trigger_count++;
    return test_cam->trigger_result;
}

bool
UtoGp2Cpp::write_config(camera_ptr & camera)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->write_config_count++;
    return test_cam->write_config_result;
}

bool
UtoGp2Cpp::write_property(
    camera_ptr & camera,
    const std::string & property,
    const std::string & value)
{
    auto test_cam = _camera_to_test[camera];
    test_cam->write_property_count++;

    // Read-only property.
    if (property == "manufacturer")
    {
        return false;
    }

    // Read-only property.
    else if (property == "cameramodel")
    {
        return false;
    }

    // Read-only property.
    else if (property == "batterylevel")
    {
        return false;
    }

    else if (property == "shutterspeed2")
    {
        if(test_cam->write_property_result)
        {
            test_cam->shutter = value;
        }
    }

    else if (property == "expprogram")
    {
        if(test_cam->write_property_result)
        {
            test_cam->mode = value;
        }
    }

    else if (property == "f-number")
    {
        if(test_cam->write_property_result)
        {
            test_cam->fstop = value;
        }
    }

    else if (property == "iso")
    {
        if(test_cam->write_property_result)
        {
            test_cam->iso = value;
        }
    }

    else if (property == "imagequality")
    {
        if(test_cam->write_property_result)
        {
            test_cam->quality = value;
        }
    }

    else if (property == "availableshots")
    {
        if(test_cam->write_property_result)
        {
            test_cam->num_photos = value;
        }
    }
    else
    {
        return false;
    }

    return test_cam->write_property_result;
}


milliseconds
FakeClock::now()
{
    return time_ms;
}

TempFile::TempFile(const std::string & filename, const std::string & content)
{
    path = std::filesystem::temp_directory_path() / filename;
    std::ofstream file_stream(path);
    file_stream << content;
    file_stream.close();
}

TempFile::~TempFile()
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
}


Harness::Harness()
    : cmd_socket()
    , tlm_socket()
    , gp2cpp()
    , clock()
    , cc(cmd_socket, tlm_socket, gp2cpp, clock, {})
{}

result
Harness::dispatch(milliseconds ms)
{
    auto res = cc.dispatch();
    clock.time_ms += ms;
    return res;
}

Telem
Harness::dispatch_to(milliseconds destination)
{
    while(cc.control_time() < destination)
    {
        REQUIRE(cc.dispatch() == result::success);
        clock.time_ms += 50;
    }
    auto & telem_vec = tlm_socket.from_send();
    auto current_size = telem_vec.size();
    return parse_telem(telem_vec[current_size - 1]);
}

Telem
Harness::dispatch_to_next_message(milliseconds ms)
{
    auto & telem_vec = tlm_socket.from_send();
    auto current_size = telem_vec.size();
    while (telem_vec.size() == current_size)
    {
        REQUIRE(cc.dispatch() == result::success);
        clock.time_ms += ms;
    }
    return parse_telem(telem_vec[current_size]);
}
