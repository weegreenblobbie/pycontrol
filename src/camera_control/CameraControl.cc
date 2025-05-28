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
//     udp_ip            239.192.168.1  # The UDP IP address the telemetry is sent to.
//     cam_info_port     10018          # UDP port telemetry is sent to.
//     cam_rename_port   10019          # UDP port camera rename packets arrive on.
//     period            50             # 20 Hz or 50 ms dispatch period.
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
        if (pair.key == "cam_info_port")
        {
            _cam_info_port = as_type<std::uint16_t>(pair.value);
            ABORT_IF(_cam_info_port < 1024, "port too low, pick a higher port", result::failure);
        }
        else
        if (pair.key == "cam_rename_port")
        {
            _cam_rename_port = as_type<std::uint16_t>(pair.value);
            ABORT_IF(_cam_rename_port < 1024, "port too low, pick a higher port", result::failure);
        }
        else
        if (pair.key == "period")
        {
            _control_period = as_type<std::uint64_t>(pair.value);
            ABORT_IF(_control_period < 10, "100+ Hz is probably too fast", result::failure);
        }
        else
        if (pair.key == "camera_aliases")
        {
            kv_pair_vec data;
            ABORT_IF(read_config(pair.value, data), "Failed to read camera_aliases", result::failure);
            for (const auto & pair : data)
            {
                _camera_aliases[pair.key] = pair.value;
                INFO_LOG << "init(): mapping camera alias: " << pair.key << " to " << pair.value << "\n";
            }
        }
    }

    INFO_LOG << "init():          udp_ip: " << _ip << "\n";
    INFO_LOG << "init():   cam_info_port: " << _cam_info_port << "\n";
    INFO_LOG << "init(): cam_rename_port: " << _cam_rename_port << "\n";
    INFO_LOG << "init():  control_period: " << _control_period << " ms\n";

    ABORT_ON_FAILURE(_cam_info_socket.init(_ip, _cam_info_port), "UpdSocket::init() failed", result::failure);
    ABORT_ON_FAILURE(_cam_rename_socket.init(_ip, _cam_rename_port), "UpdSocket::init() failed", result::failure);
    ABORT_ON_FAILURE(_cam_rename_socket.bind(), "UdpSocket::bind() failed", result::failure);

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
                    auto cam = std::make_shared<Camera>(
                        camera,
                        port,
                        serial,
                        "camera.config"
                    );
                    _cameras[serial] = cam;
                    if (_camera_aliases.count(serial) == 0)
                    {
                        _camera_aliases[serial] = cam->read_settings().desc;
                    }
                }

                // Update existing camera.
                else
                {
                    _cameras[serial]->reconnect(camera, port);
                }

                DEBUG_LOG << "adding "
                          << "serial=" << serial << " "
                          << "desc=" << _camera_aliases[serial] << " "
                          << "port=" << port
                          << "\n";

                _current_ports.insert(port);
            }
        }

        // Mark any cameras not detected as disconnected.
        for (auto & pair : _cameras)
        {
            auto & cam = pair.second;
            const auto & info = cam->read_settings();
            const auto & port = info.port;
            if (not detections.contains(port))
            {
                DEBUG_LOG << "removing "
                          << "serial=" << info.serial << " "
                          << "desc=" << _camera_aliases[info.serial] << " "
                          << "port=" << port
                          << "\n";
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
    bool scan_cameras = false;

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
            scan_cameras = true;
            send_telemetry = true;
            next_state = CameraControl::State::monitor;
            break;
        }

        case CameraControl::State::monitor:
        {
            scan_cameras = true;
            send_telemetry = true;
            // TODO: transition to execute_ready if we have a valid camera
            // schedule.
            next_state = CameraControl::State::monitor;
            break;
        }

        case CameraControl::State::execute_ready:
        {
            scan_cameras = true;
            send_telemetry = true;

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

    // Scan for camera changes.
    if (_scan_time <= _control_time)
    {
        _scan_time += 2000;  // 0.5 Hz.
        if (scan_cameras)
        {
            _camera_scan();
        }
    }

    // Send out 1 Hz telemetry.
    if (_send_time <= _control_time)
    {
        _send_time += 1000;  // 1 Hz.
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

                const auto & entry = _camera_aliases.find(info.serial);

                const auto desc = entry != _camera_aliases.end() ?
                                  entry->second :
                                  info.desc;

                _message << "connected " << info.connected << "\n"
                         << "serial " << info.serial << "\n"
                         << "port " << info.port << "\n"
                         << "desc " << desc << "\n"
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
                _cam_info_socket.send(_message.str()),
                "UdpSocket::send() failed",
                result::failure
            );
        }
    }

    // Read any commands.
    if (_read_time <= _control_time)
    {
        _read_time += 1000; // 1 Hz.
        std::string msg = "";
        ABORT_ON_FAILURE(
            _cam_rename_socket.read(msg),
            "UdpSocket::read() failed",
            result::failure
        );

        if (not msg.empty())
        {
            auto tokens = split(msg);
            if (tokens.size() == 2)
            {
                INFO_LOG << "mapping serial " << tokens[0] << " to " << tokens[1] << "\n";
                _camera_aliases[tokens[0]] = tokens[1];
            }
            else
            {
                ERROR_LOG << "Got bad camera rename message: '" << msg << "'" << std::endl;
                return result::failure;
            }
        }
    }

    return result::success;
}


}
