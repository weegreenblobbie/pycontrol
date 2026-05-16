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
        timelapse_idle,
        timelapse_running,
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
    result _read_command(bool & got_message, State & next_state);
    result _dispatch_camera_events();
    result _timelapse_dispatch();

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
    std::string       _command_response = "{\"last_accepted_id\":0,\"last_rejected_id\":0,\"message\":\"\"}";
    std::stringstream _telem_message = std::stringstream(std::string(4096, '\0'));

    milliseconds      _control_time {0};
    milliseconds      _control_period {0};
    milliseconds      _scan_time {0};
    milliseconds      _send_time {0};
    milliseconds      _read_time {500};  // Keeping it out of phase

    std::uint32_t     _last_accepted_command_id {0};
    std::uint32_t     _last_rejected_command_id {0};
    std::string       _last_rejected_message {};
    std::string       _sequence_filename {};
    event_map         _event_map {};
    sequence_map      _sequence_map {};

    // TODO: To constrol multiple cameras in timelapse mode with one raspberry
    //       pi, make this a map of camera serial to a struct with these
    //       settings.
    milliseconds      _timelapse_time {0};
    std::string       _timelapse_serial {};
    milliseconds      _timelapse_interval {0};
    float             _timelapse_min_shutter {0.0f};
    float             _timelapse_max_shutter {0.0f};
    unsigned int      _timelapse_min_iso {0};
    unsigned int      _timelapse_max_iso {0};
    int               _timelapse_min_hist_mask {-1};
    int               _timelapse_max_hist_mask {256};
    int               _timelapse_target_bin {0};
    int               _timelapse_target_offset {0};
    float             _timelapse_target_percent {0.05f};
    int               _timelapse_target_error {0};
    int               _timelapse_min_deadband {10};
    int               _timelapse_max_deadband {1};
    std::uint32_t     _timelapse_capture_count {0};
    std::uint32_t     _timelapse_pixel_count {0};

    enum class TriggerType {none, trigger, histogram};

    TriggerType       _trigger_type {TriggerType::none};
    std::string       _trigger_serial {};

    interface::UdpSocket &  _command_socket;
    interface::UdpSocket &  _telem_socket;
    interface::GPhoto2Cpp & _gp2cpp;
    interface::WallClock &  _clock;
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
        case CameraControl::State::timelapse_idle: out << "timelapse_idle"; break;
        case CameraControl::State::timelapse_running: out << "timelapse_running"; break;
    }
    return out;
}


}