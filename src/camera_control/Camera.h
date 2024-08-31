#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace pycontrol
{


class Camera
{
public:

    struct Info
    {
        std::string serial;
        std::string port;
        std::string desc;
        std::string mode; // manual, A, S, P, etc.
        std::string shutter;
        std::string fstop;
        std::string iso;
        std::uint32_t quality;
        std::uint32_t battery_level;
        std::uint32_t num_photos;
    };

    using info_vec = std::vector<Info>;

    static info_vec detect_cameras();

    bool init(const std::string & config_file);
    bool reconnect(const std::string & port);

    const Info & read_settings();
    void write_settings();

    void set_shutter(const std::string & speed);
    void set_fstop(const std::string & fstop);
    void set_iso(const std::string & iso);

private:

    bool _stale_shutter {1};
    bool _stale_fstop   {1};
    bool _stale_iso     {1};
    Info _info;
};


}
