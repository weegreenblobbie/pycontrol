#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include <gphoto2cpp/gphoto2cpp.h>

#include <common/types.h>

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
        std::string num_photos;
    };

    Camera(
        gphoto2cpp::camera_ptr & camera,
        const std::string & port,
        const std::string & serial,
        const std::string & config_file
    );

    void reconnect(gphoto2cpp::camera_ptr & camera, const std::string & port);
    void disconnect() {_info.connected = false; }

    void fetch_settings();
    const Info & read_settings() { return _info; }
    void write_settings();

    void set_shutter(const std::string & speed);
    void set_fstop(const std::string & fstop);
    void set_iso(const std::string & iso);

private:

    bool                   _stale_shutter {1};
    bool                   _stale_fstop   {1};
    bool                   _stale_iso     {1};
    Info                   _info;
    gphoto2cpp::camera_ptr _camera;
};


}
