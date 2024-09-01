#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <common/io.h>
#include <common/types.h>
#include <common/UdpSocket.h>

namespace boost { namespace process { class ipstream; } }
namespace pycontrol
{

using port_set = std::set<std::string>;


class Camera;
using camera_map = std::map<std::string, std::shared_ptr<Camera>>;

class CameraControl
{
public:

    enum class State: unsigned int
    {
        init,
        scan,
        monitor,
        execute_ready,
        executing,
    };

    result init(const std::string & config_file);
    result dispatch();

private:

    void _camera_scan();

    State             _state   {State::init};
    camera_map        _cameras {};
    port_set          _current_ports {};
    std::stringstream _message {};

    std::string    _ip    {"239.192.168.1"};
    std::uint16_t  _port  {10018};
    std::uint64_t  _control_time {0};
    std::uint64_t  _control_period {100}; // 10 Hz.
    std::uint64_t  _send_time {0};

    UdpSocket      _socket;
};


inline
std::ostream & operator<<(std::ostream & out, const CameraControl::State & s)
{
    switch(s)
    {
        case CameraControl::State::init: out << "init"; break;
        case CameraControl::State::scan: out << "scan"; break;
        case CameraControl::State::monitor: out << "monitor"; break;
        case CameraControl::State::execute_ready: out << "execute_ready"; break;
        case CameraControl::State::executing: out << "executing"; break;
    }
    return out;
}


}