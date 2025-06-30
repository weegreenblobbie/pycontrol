#pragma once

#include <vector>
#include <string>

#include <interface/GPhoto2Cpp.h>

#include <common/types.h>
#include <camera_control/Event.h>

namespace pycontrol
{


class Camera
{
public:

    struct Info
    {
        bool        connected {false};
        std::string serial {"N/A"};
        std::string port {"N/A"};
        std::string desc {"N/A"};
        std::string mode {"N/A"};
        std::string shutter {"N/A"};
        std::string fstop {"N/A"};
        std::string iso {"N/A"};
        std::string quality {"N/A"};
        std::string battery_level {"N/A"};
        std::string num_photos {"-1"};
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

    void set_fstop(const std::string & fstop);
    void set_iso(const std::string & iso);
    void set_mode(const std::string & mode);
    void set_quality(const std::string & quality);
    void set_shutter(const std::string & speed);

private:

    interface::GPhoto2Cpp & _gp2cpp;
    Info                   _info;
    gphoto2cpp::camera_ptr _camera;
};


}
