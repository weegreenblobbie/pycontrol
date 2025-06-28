#include <interface/UdpSocket.h>
#include <interface/GPhoto2Cpp.h>
#include <camera_control/CameraControl.h>


#include <catch2/catch_test_macros.hpp>


#include <stdexcept>


namespace GP2
{
    struct _Camera {};
}


using namespace pycontrol;


struct UtoSocket : interface::UdpSocket
{
    UtoSocket()
        : _to_recv(),
          _from_send(),
          _recv_itor(_to_recv.begin())
    {}

    result recv(std::string & out) override
    {
        out = "";
        if (_recv_itor >= _to_recv.end())
        {
            return result::success;
        }
        out = *_recv_itor++;
        return result::success;
    }

    result send(const std::string & out) override
    {
        _from_send.push_back(out);
        return result::success;
    }

    void to_recv(const str_vec & messages)
    {
        _to_recv = messages;
        _recv_itor = _to_recv.begin();
    }

    const str_vec & from_send() const { return _from_send; }

private:
    str_vec _to_recv;
    str_vec _from_send;

    str_vec::iterator _recv_itor;
};


using choice_map_t = std::map<std::string, str_set>;

struct TestCamera
{
    std::string maker = "N/A";
    std::string model = "N/A";

    bool connected = false;
    std::string serial = "N/A";
    std::string port = "N/A";
    std::string desc = "N/A";
    std::string mode = "N/A";
    std::string shutter = "N/A";
    std::string fstop = "N/A";
    std::string iso = "N/A";
    std::string quality = "N/A";
    std::string batt_level = "N/A";
    std::string num_photos = "-1";

    choice_map_t choice_map;

    int open_count = 0;
    int read_choices_count = 0;
    int read_config_count = 0;
    int read_property_count = 0;
    int reset_cache_count = 0;
    int trigger_count = 0;
    int write_config_count = 0;
    int write_property_count = 0;

    bool read_config_result = true;
    bool read_property_result = true;
    bool trigger_result = true;
    bool write_config_result = true;
    bool write_property_result = true;

};

using test_camera_ptr = std::shared_ptr<TestCamera>;
using gphoto2cpp::camera_ptr;


struct UtoGp2Cpp : interface::GPhoto2Cpp
{
    void add_camera(test_camera_ptr cam)
    {
        if (not cam)
        {
            throw std::runtime_error("cam is null");
        }
        _test_cams.insert(cam);
    }

    str_vec auto_detect() override
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

    camera_ptr open_camera(const std::string & port) override
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

    str_set read_choices(const camera_ptr & camera, const std::string & property) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->read_choices_count++;
        return test_cam->choice_map[property];
    }

    bool _read_config_result = true;

    bool read_config(const camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->read_config_count++;
        return test_cam->read_config_result;
    }

    bool read_property(
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

    void reset_cache(const camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->reset_cache_count++;
    }

    bool trigger(const camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->trigger_count++;
        return test_cam->trigger_result;
    }

    bool write_config(camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->write_config_count++;
        return test_cam->write_config_result;
    }

    bool write_property(
        camera_ptr & camera,
        const std::string & property,
        const std::string & value) override
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

private:

    std::set<test_camera_ptr> _test_cams;
    std::map<camera_ptr, test_camera_ptr> _camera_to_test;

};



struct Harness
{
    Harness()
        : cmd_socket()
        , tlm_socket()
        , gp2cpp()
        , cc(cmd_socket, tlm_socket, gp2cpp, {})
    {}


    UtoSocket cmd_socket;
    UtoSocket tlm_socket;
    UtoGp2Cpp gp2cpp;
    CameraControl cc;

};




TEST_CASE("CameraControl", "init")
{
    Harness harness;
}