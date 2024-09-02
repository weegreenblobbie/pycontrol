#include <gphoto2cpp/gphoto2cpp.h>

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

    ABORT_ON_FAILURE(_socket.init(_ip, _port), "UpdSocket::init() failed", result::failure);

    return result::success;
}


void
CameraControl::_camera_scan()
{
    const auto new_detections = gphoto2cpp::auto_detect();
    const auto detections = port_set(new_detections.begin(), new_detections.end());
    const bool cameras_changed = detections != _current_ports;
    if (cameras_changed)
    {
        // Add or update cameras.
        for (const auto & port : detections)
        {
            if (not _current_ports.contains(port))
            {
                auto camera = gphoto2cpp::open_camera(port);
                if (not camera)
                {
                    INFO_LOG << "open_camera() failed, ignoring" << std::endl;
                    continue;
                }

                const auto serial = gphoto2cpp::read_property(camera, "serialnumber");

                // Add new camera.
                if (not _cameras.contains(serial))
                {
                    DEBUG_LOG << "adding " << port << "\n";
                    auto cam = std::make_shared<Camera>(
                        camera,
                        port,
                        serial,
                        "config_camera"
                    );
                    _cameras[serial] = cam;
                }

                // Update existing camera.
                else
                {
                    _cameras[serial]->reconnect(camera, port);
                }

                _current_ports.insert(port);
            }
        }

        // Mark any cameras not detected as disconnected.
        for (auto & pair : _cameras)
        {
            auto & cam = pair.second;
            const auto & port = cam->read_settings().port;
            if (not detections.contains(port))
            {
                DEBUG_LOG << "removing " << port << "\n";
                cam->disconnect();
                _current_ports.erase(port);
            }
        }
    }

    // Always fetch camera settings to reflect the camera state.
    for (auto & pair : _cameras)
    {
        auto & cam = pair.second;
        cam->fetch_settings();
    }
}


result
CameraControl::dispatch()
{
    auto next_state = CameraControl::State::init;
    bool send_telemetry = false;

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
            _camera_scan();
            send_telemetry = true;
            next_state = CameraControl::State::monitor;
            break;
        }

        case CameraControl::State::monitor:
        {
            _camera_scan();
            send_telemetry = true;
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
        INFO_LOG << "time: " << _control_time
                 << " state: " << _state << " -> " << next_state
                 << std::endl;
        _state = next_state;
    }

    // Send out 1 Hz telemetry.
    if (_send_time <= _control_time)
    {
        _send_time = (_send_time + 1000);
        if (send_telemetry)
        {
            _message.seekp(0, std::ios::beg);
            _message << "CAM_CONTROL\n"
                     << "state " << _state << "\n"
                     << "num_cameras " << _cameras.size() << "\n";
            for (const auto & pair : _cameras)
            {
                const auto & cam = pair.second;
                const auto & info = cam->read_settings();
                _message << "connected " << info.connected << "\n"
                         << "serial " << info.serial << "\n"
                         << "port " << info.port << "\n"
                         << "desc " << info.desc << "\n"
                         << "mode " << info.mode << "\n"
                         << "shutter " << info.shutter << "\n"
                         << "fstop " << info.fstop << "\n"
                         << "iso " << info.iso << "\n"
                         << "quality " << info.quality << "\n"
                         << "batt " << info.battery_level << "\n"
                         << "num_photos " << info.num_photos << "\n";
            }
            // Mark the end of the buffer for strlen().
            _message << '\0';
            // Rewind.
            _message.seekp(0, std::ios::beg);

            // Emit telemetry packet.
            ABORT_ON_FAILURE(
                _socket.send(_message.str()),
                "UdpSocket::send() failed",
                result::failure
            );
        }
    }

    return result::success;
}


}
