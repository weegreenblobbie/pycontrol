#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include <gphoto2cpp/gphoto2cpp.h>

#include <common/types.h>
#include <camera_control/Event.h>

namespace pycontrol
{


class Camera
{
public:

    struct Info
    {
        bool        connected;
        std::string serial;
        std::string port;
        std::string desc;
        std::string mode;     // manual, A, S, P, etc.
        std::string shutter;
        std::string fstop;
        std::string iso;
        std::string quality;
        std::string battery_level;
        std::string num_photos {"__ERROR__"};
    };

    Camera(
        gphoto2cpp::camera_ptr & camera,
        const std::string & port,
        const std::string & serial,
        const std::string & config_file
    );

    void reconnect(gphoto2cpp::camera_ptr & camera, const std::string & port);
    void disconnect();


    std::vector<std::string> read_choices(const std::string & property) const;

    result read_config();
    result write_config();
    const Info & info() { return _info; }
    result trigger();
    result handle(const Event & event);

    void set_shutter(const std::string & speed);
    void set_fstop(const std::string & fstop);
    void set_iso(const std::string & iso);
    void set_quality(const std::string & quality);

private:

    Info                   _info;
    gphoto2cpp::camera_ptr _camera;
};


}
