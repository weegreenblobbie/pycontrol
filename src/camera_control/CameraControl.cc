#include <common/str_utils.h>
#include <camera_control/CameraControl.h>
#include <camera_control/Camera.h>

namespace pycontrol
{

//-----------------------------------------------------------------------------
// Reads the input config files and update settings found theirin.  All settings
// are key value pair strings.  Here's a complete example with default values:
//
//     # comments start with '#' and the rest of the line is ignored.
//     udp_ip     239.192.168.1  # The UDP IP address the telemetry is sent to.
//     udp_port   10018          # UDP port telemetry is sent to.
//     period     50             # 20 Hz or 50 ms dispatch period.
//
//-----------------------------------------------------------------------------
result
CameraControl::init(const std::string & config_file)
{
    //-------------------------------------------------------------------------
    // Read in the config file.
    //-------------------------------------------------------------------------
    kv_pair_vec config_pairs;
    ABORT_IF(read_config(config_file, config_pairs), "failed", result::failure);

    for (const auto & pair : config_pairs)
    {
        if (pair.key == "udp_ip")
        {
            _ip = pair.value;
            ABORT_IF_NOT(_ip.starts_with("239."), "Please use a local multicast IP address", result::failure);
        }
        else
        if (pair.key == "udp_port")
        {
            _port = as_type<std::uint16_t>(pair.value);
            ABORT_IF(_port < 1024, "port too low, pick a higher port", result::failure);
        }
        else
        if (pair.key == "period")
        {
            _control_period = as_type<std::uint64_t>(pair.value);
            ABORT_IF(_control_period < 10, "100+ Hz is probably too fast", result::failure);
        }
    }

    INFO_LOG << "init():         udp_ip: " << _ip << "\n";
    INFO_LOG << "init():       udp_port: " << _port << "\n";
    INFO_LOG << "init(): control_period: " << _control_period << " ms\n";

    return result::success;
}

result
CameraControl::dispatch()
{
    auto next_state = CameraControl::State::init;

    switch(_state)
    {
        case CameraControl::State::init:
        {
            // For now just transition to scan.
            next_state = CameraControl::State::scan;
            break;
        }
        case CameraControl::State::scan:
        {
            auto detected = Camera::detect_cameras();

            // TODO: add detected cameras to serial to camera map.

            next_state = CameraControl::State::monitor;

            break;
        }

        case CameraControl::State::monitor:
        {

            // TODO: check for any usb events.

            // TODO: handle unplug events, moving cameras to an unplugged set.

            // TODO: handle plug events, connect to camera and get port, read
            //       serial, command unplugged camera to reconnect on new port,
            //       if known.

            // TODO: Read all camera settings and store.

            // TODO: transition to execute_ready if we have a schedule.
            next_state = CameraControl::State::monitor;
            break;
        }

        case CameraControl::State::execute_ready:
        {
            // TODO: check for any usb events.

            // TODO: handle unplug events, moving cameras to an unplugged set.

            // TODO: handle plug events, connect to camera and get port, read
            //       serial, command unplugged camera to reconnect on new port,
            //       if known.

            // TODO: Read all camera settings and store.

            // TODO: transition to executing when a scheduled camera event is
            //       "soon", say <= 60s away.
            next_state = CameraControl::State::execute_ready;
            break;
        }

        case CameraControl::State::executing:
        {
            // TODO: dispatch all camera commands up to and including current
            //       control time.
            //
            // This means control_time probably must equal nanos since gps
            // epoch.
            //
            //
            next_state = CameraControl::State::executing;

            // TODO: If next event time > 60s, transition back to execute_ready;
            next_state = CameraControl::State::execute_ready;
            break;
        }

        default:
        {
            ABORT_IF(true, "Oops, state" << _state << " is't implemneted!", result::failure);
            break;
        }
    }

    _control_time += _control_period;

    if (_state != next_state)
    {
        INFO_LOG << "time: " << _control_time << " state: " << _state << " -> " << next_state << "\n";
        _state = next_state;
    }

    auto detected = Camera::detect_cameras();

    // TODO: perform and log state transitions.

    // TODO: send out 1 Hz telemetry.

    return result::success;
}


}
