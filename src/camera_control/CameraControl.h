#pragma once

#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <common/io.h>
#include <common/types.h>

namespace pycontrol
{

// Forwards.
namespace interface
{
    class UdpSocket;
    class GPhoto2Cpp;
    class WallClock;
}

class Camera;
class CameraSequence;
struct Event;


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

    CameraControl(
        interface::UdpSocket & cmd_socket,
        interface::UdpSocket & telem_socket,
        interface::GPhoto2Cpp & gp2cpp,
        interface::WallClock & clock,
        const kv_pair_vec & cam_to_ids
    );

    result dispatch();

    const milliseconds & control_time() const { return _control_time; }
    const milliseconds & control_period() const { return _control_period; }

private:

    CameraControl(const CameraControl & copy) = delete;
    CameraControl & operator=(const CameraControl & rhs) = delete;

    void _camera_scan();
    result _send_telemetry();
    result _read_command();
    result _dispatch_camera_events();

    milliseconds _get_event_time(const Event & event) const;
    milliseconds _get_next_event_time() const;

    using event_map = std::map<std::string, milliseconds>;
    using port_set = std::set<UsbPort>;
    using camera_map = std::map<Serial, std::shared_ptr<Camera>>;
    using serial_to_id = std::map<Serial, CamId>;
    using id_to_serial = std::map<CamId, Serial>;
    using sequence_map = std::map<CamId, std::shared_ptr<CameraSequence>>;

    State             _state   {State::init};
    camera_map        _cameras {};
    serial_to_id      _serial_to_id {};
    id_to_serial      _id_to_serial {};
    port_set          _current_ports {};

    std::string       _command_buffer = std::string(1024, '\0');
    std::string       _command_response = "{\"id\":0,\"success\":true}";
    std::stringstream _telem_message = std::stringstream(std::string(4096, '\0'));

    milliseconds      _control_time {0};
    milliseconds      _control_period {0};
    milliseconds      _scan_time {0};
    milliseconds      _send_time {0};
    milliseconds      _read_time {500};  // Keeping it out of phase

    std::uint32_t     _command_id {0};
    std::string       _sequence_filename {};
    event_map         _event_map {};
    sequence_map      _sequence_map {};

    interface::UdpSocket &     _command_socket;
    interface::UdpSocket &     _telem_socket;
    interface::GPhoto2Cpp &    _gp2cpp;
    interface::WallClock &     _clock;
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