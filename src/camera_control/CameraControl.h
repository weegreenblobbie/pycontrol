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
using camera_alias = std::map<std::string, std::string>;

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

    CameraControl() {}

    const std::uint64_t & control_time() const { return _control_time; }
    const std::uint64_t & control_period() const { return _control_period; }

private:

    CameraControl(const CameraControl & copy) = delete;
    CameraControl & operator=(const CameraControl & rhs) = delete;

    void _camera_scan();

    State             _state   {State::init};
    camera_map        _cameras {};
    camera_alias      _camera_aliases {};
    port_set          _current_ports {};
    std::stringstream _message {};

    std::string    _ip {};
    std::uint16_t  _cam_info_port {};
    std::uint16_t  _cam_rename_port {};
    std::uint64_t  _control_time {0};
    std::uint64_t  _control_period {};
    std::uint64_t  _send_time {0};
    std::uint64_t  _read_time {500};  // Keep out of phase with _send_time.

    UdpSocket      _cam_info_socket {};
    UdpSocket      _cam_rename_socket {};
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