#pragma once

#include <vector>
#include <string>

#include <common/types.h>

#include <interface/GPhoto2Cpp.h>

namespace pycontrol
{

class Event;


class Camera
{
public:

    struct Info
    {
        bool        connected      {false};
        std::string serial         {"N/A"};
        std::string port           {"N/A"};
        std::string desc           {"N/A"};
        std::string mode           {"N/A"};
        std::string shutter        {"N/A"};
        std::string fstop          {"N/A"};
        std::string iso            {"N/A"};
        std::string quality        {"N/A"};
        std::string battery_level  {"N/A"};
        std::string capture_mode   {"N/A"};
        std::string shooting_speed {"N/A"};

        int num_avail             {0};
        int num_photos            {0};
        int burst_number          {0};
    };

    Camera(
        interface::GPhoto2Cpp & gp2cpp,
        gphoto2cpp::camera_ptr & camera,
        const std::string & port,
        const std::string & serial,
        const std::string & config_file
    );

    const Info & info() { return _info; }

    void reconnect(gphoto2cpp::camera_ptr & camera, const std::string & port);
    void disconnect();

    result handle(const Event & event);

    std::vector<std::string>
    read_choices(const std::string & property) const;
    result write_property(
        const std::string & property,
        const std::string & value);

    result read_config();
    result write_config();
    result trigger();
    result drain_events();

    void set_burst_number(const std::string & burst_number);
    void set_capture_mode(const std::string & capture_mode);
    void set_fstop(const std::string & fstop);
    void set_iso(const std::string & iso);
    void set_mode(const std::string & mode);
    void set_quality(const std::string & quality);
    void set_shooting_speed(const std::string & shooting_speed);
    void set_shutter(const std::string & speed);

    bool have_num_avail() const { return _have_num_avail; }
    bool have_burst_number() const { return _have_burst_number; }
    bool have_capture_mode() const { return _have_capture_mode; }
    bool have_shooting_speed() const { return _have_shooting_speed; }

private:

    void _query_props();

    interface::GPhoto2Cpp &        _gp2cpp;
    gphoto2cpp::camera_ptr         _camera;
    Info                           _info;

    bool                           _have_shutterspeed2 {false};
    bool                           _have_num_avail {false};
    bool                           _have_burst_number {false};
    bool                           _have_capture_mode {false};
    bool                           _have_shooting_speed {false};
};


}
