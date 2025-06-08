#include <chrono>

#include <gphoto2cpp/gphoto2cpp.h>

#include <camera_control/Camera.h>
#include <camera_control/CameraControl.h>
#include <camera_control/CameraSequence.h>
#include <camera_control/CameraSequenceFileReader.h>
#include <common/str_utils.h>


namespace
{

// Returns the current system time in miliseconds since UNIX epoch.
pycontrol::milliseconds
now()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} /* namespace */


namespace pycontrol
{

//-----------------------------------------------------------------------------
// Reads the input config files and update settings found theirin.  All settings
// are key value pair strings.  Here's a complete example with default values:
//
//     # comments start with '#' and the rest of the line is ignored.
//     udp_ip            239.192.168.1  # The UDP IP address the telemetry is sent to.
//     cam_info_port     10018          # Output UDP port for detected camera info.
//     cam_rename_port   10019          # Input UDP port camera rename packets arrive on.
//     event_port        10020          # Input UDP port for reading event packets on.
//     seq_state_port    10021          # Output UDP port for sending camera sequence state.
//     period            50             # 20 Hz or 50 ms dispatch period.
//     camera_aliases    filename       # A file to persistently map camera serial numbers to short names.
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

    _control_period = 0;

    std::string udp_ip = "";
    auto info_port = std::uint16_t {0};
    auto rename_port = std::uint16_t {0};
    auto event_port = std::uint16_t {0};
    auto seq_port = std::uint16_t {0};

    for (const auto & pair : config_pairs)
    {
        if (pair.key == "udp_ip")
        {
            udp_ip = pair.value;
        }
        else
        if (pair.key == "cam_info_port")
        {
            ABORT_ON_FAILURE(
                as_type<std::uint16_t>(pair.value, info_port),
                "as_type<std::uint16_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "cam_rename_port")
        {
            ABORT_ON_FAILURE(
                as_type<std::uint16_t>(pair.value, rename_port),
                "as_type<std::uint16_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "event_update_port")
        {
            ABORT_ON_FAILURE(
                as_type<std::uint16_t>(pair.value, event_port),
                "as_type<std::uint16_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "seq_state_port")
        {
            ABORT_ON_FAILURE(
                as_type<std::uint16_t>(pair.value, seq_port),
                "as_type<std::uint16_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "period")
        {
            ABORT_ON_FAILURE(
                as_type<milliseconds>(pair.value, _control_period),
                "as_type<std::uint64_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "camera_aliases")
        {
            kv_pair_vec data;
            ABORT_ON_FAILURE(read_config(pair.value, data), "Failed to read camera_aliases", result::failure);
            for (const auto & pair : data)
            {
                const auto & serial = pair.key;
                const auto & id = pair.value;
                INFO_LOG << "init(): mapping camera alias: " << serial << " to " << id << "\n";
                _serial_to_id[serial] = id;
                _id_to_serial[id] = serial;
            }
        }
    }

    ABORT_IF_NOT(udp_ip.starts_with("239."), "Please use a local multicast IP address", result::failure);
    ABORT_IF(info_port < 1024, "cam_info_port too low, pick a higher port", result::failure);
    ABORT_IF(rename_port < 1024, "cam_rename_port too low, pick a higher port", result::failure);
    ABORT_IF(event_port < 1024, "event_update_port too low, pick a higher port", result::failure);
    ABORT_IF(seq_port < 1024, "seq_state_port too low, pick a higher port", result::failure);
    ABORT_IF(_control_period < 10, "100+ Hz is probably too fast", result::failure);

    INFO_LOG << "init():            udp_ip: " << udp_ip << "\n";
    INFO_LOG << "init():     cam_info_port: " << info_port << "\n";
    INFO_LOG << "init():   cam_rename_port: " << rename_port << "\n";
    INFO_LOG << "init(): event_update_port: " << event_port << "\n";
    INFO_LOG << "init():    seq_state_port: " << seq_port << "\n";
    INFO_LOG << "init():    control_period: " << _control_period << " ms\n";

    ABORT_ON_FAILURE(_cam_info_socket.init(udp_ip, info_port), "UpdSocket::init() failed", result::failure);
    ABORT_ON_FAILURE(_seq_state_socket.init(udp_ip, seq_port), "UpdSocket::init() failed", result::failure);

    ABORT_ON_FAILURE(_cam_rename_socket.init(udp_ip, rename_port), "UpdSocket::init() failed", result::failure);
    ABORT_ON_FAILURE(_cam_rename_socket.bind(), "UdpSocket::bind() failed", result::failure);

    ABORT_ON_FAILURE(_event_socket.init(udp_ip, event_port), "UpdSocket::init() failed", result::failure);
    ABORT_ON_FAILURE(_event_socket.bind(), "UdpSocket::bind() failed", result::failure);

    return result::success;
}

void
CameraControl::
_camera_scan()
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
                    if (not _serial_to_id.contains(serial))
                    {
                        const auto & id = cam->read_settings().desc;
                        _serial_to_id[serial] = id;
                        _id_to_serial[id] = serial;
                    }
                }

                // Update existing camera.
                else
                {
                    _cameras[serial]->reconnect(camera, port);
                }

                DEBUG_LOG << "adding "
                          << "serial=" << serial << " "
                          << "desc=" << _serial_to_id[serial] << " "
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
                          << "desc=" << _serial_to_id[info.serial] << " "
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
CameraControl::
_send_detected_cameras()
{
    _message.seekp(0, std::ios::beg);
    _message << "num_cameras " << _cameras.size() << "\n";
    for (const auto & [serial, cam_ptr] : _cameras)
    {
        const auto & info = cam_ptr->read_settings();
        const auto & entry = _serial_to_id.find(serial);

        const auto desc = entry != _serial_to_id.end() ?
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
    // Mark the end of the stream and rewind before sending.
    _message << '\0';
    _message.seekp(0, std::ios::beg);

    ABORT_ON_FAILURE(
        _cam_info_socket.send(_message.str()),
        "UdpSocket::send() failed",
        result::failure
    );

    return result::success;
}


result
CameraControl::
_send_sequence_state()
{
    _message.seekp(0, std::ios::beg);
    _message << "state ";
    switch(_state)
    {
        case State::init: { _message << "init"; break;}
        case State::scan: { _message << "scan"; break;}
        case State::monitor: { _message << "monitor"; break;}
        case State::execute_ready: { _message << "execute_ready"; break;}
        case State::executing: { _message << "executing"; break;}
    }
    _message << "\n";
    _message << "num_cameras " << _cameras.size() << "\n";
    for (const auto & [serial, cam_ptr] : _cameras)
    {
        const auto & info = cam_ptr->read_settings();
        const auto & entry = _serial_to_id.find(serial);

        const auto name = entry != _serial_to_id.end() ?
                          entry->second :
                          info.desc;

        _message << "connected " << info.connected << "\n"
                 << "name " << name << "\n";

        const auto seq_itor = _sequence_map.find(name);

        if (seq_itor != _sequence_map.end())
        {
            const auto & seq = *seq_itor->second;
            const auto & event = seq.front();

            const auto eta_ms = _get_event_time(seq.front()) - _control_time;

            _message << "num_events " << seq.size() << "\n"
                     << "position " << seq.pos() << "\n"
                     << "event_id " << event.event_id << "\n"
                     << "eta " << eta_ms << "\n"
                     << "channel " << to_string(event.channel) << "\n"
                     << "value " << event.channel_value << "\n";
        }
        else
        {
            _message << "num_events 0\n"
                     << "position 0\n"
                     << "event_id none\n"
                     << "eta none\n"
                     << "channel none\n"
                     << "value none\n";
        }
    }
    // Mark the end of the stream and rewind before sending.
    _message << '\0';
    _message.seekp(0, std::ios::beg);

    ABORT_ON_FAILURE(
        _seq_state_socket.send(_message.str()),
        "UdpSocket::send() failed",
        result::failure
    );

    return result::success;
}


result
CameraControl::
_read_camera_renames()
{
    ABORT_ON_FAILURE(
        _cam_rename_socket.read(_buffer),
        "UdpSocket::read() failed",
        result::failure
    );

    // No message avialable.
    if (_buffer.empty())
    {
        return result::success;
    }

    // TODO: remove memory allocation?
    auto tokens = split(_buffer);

    if (tokens.size() == 2)
    {
        const auto & serial = tokens[0];
        const auto & id = tokens[1];

        INFO_LOG << "mapping serial " << serial << " to " << id << "\n";
        _serial_to_id[serial] = id;
        _id_to_serial[id] = serial;
    }
    else
    {
        ERROR_LOG << "Got bad camera rename message: '" << _buffer << "'" << std::endl;
        return result::failure;
    }

    return result::success;
}


result
CameraControl::
_read_events()
{
    ABORT_ON_FAILURE(
        _event_socket.read(_buffer),
        "UdpSocket::read() failed",
        result::failure
    );

    // No message avialable.
    if (_buffer.empty())
    {
        return result::success;
    }

    // Parse the message sent from event_solver.py.
    //     uint32 packt_id
    //     string sequence filename
    //     bool   reset
    //     str int64  event_id timestamp
    //
    std::stringstream ss(_buffer);

    std::uint32_t packet_id {0};
    ABORT_IF_NOT(
        ss >> packet_id,
        "Bad event message, failed to read 'packet_id'",
        result::failure
    );

    // If we've already processed this packet, skip it.
    if (packet_id == _event_packet_id)
    {
        return result::success;
    }

    _event_packet_id = packet_id;

    // Parse the reset of the message;

    auto sequence_fn = std::string(128, '\0');
    auto reset       = bool{false};
    auto event_id    = std::string(16, '\0');
    auto timestamp   = milliseconds{0};

    ABORT_IF_NOT(
        ss >> sequence_fn,
        "Bad event message, failed to read the sequence filename",
        result::failure
    );

    ABORT_IF_NOT(
        ss >> reset,
        "Bad event message, failed to read the bool reset",
        result::failure
    );

    // Clear the event map on reset, as there will likely be a large change.
    if (reset)
    {
        _event_map.clear();
    }

    // Read event_id timestamp pairs, abort the loop on failure.
    while (true)
    {
        if (not (ss >> event_id))
        {
            break;
        }
        ABORT_IF_NOT(
            ss >> timestamp,
            "Bad event message, failed to read the timestamp",
            result::failure
        );
        _event_map[event_id] = timestamp;
    }

    // If the camera sequence changed, reread the file and remake the camera
    // sequences.
    if (_sequence_filename != sequence_fn)
    {
        auto seq_reader = CameraSequenceFileReader();
        ABORT_ON_FAILURE(
            seq_reader.read_file(sequence_fn),
            "Failed to read camera sequence '" << sequence_fn << "'",
            result::failure
        );
        _sequence_filename = sequence_fn;

        _sequence_map.clear();

        const auto & event_seq = seq_reader.get_events();
        const auto & cam_ids = seq_reader.get_camera_ids();

        for (const auto & id : cam_ids)
        {
            auto cam_seq = std::make_shared<CameraSequence>();
            ABORT_IF_NOT(
                cam_seq,
                "Failed to make CameraSequence",
                result::failure
            );
            ABORT_ON_FAILURE(
                cam_seq->load(id, event_seq),
                "CameraSequence.load() failed",
                result::failure
            );
            _sequence_map[id] = cam_seq;
        }
    }

    // If reset was requested, we reset the camera sequences and pop off events
    // that are in the past.
    if (reset)
    {
        for (auto & [id, cam_seq] : _sequence_map)
        {
            cam_seq->reset();

            // Pop off events that are in the past.
            while (
                not cam_seq->empty() and
                _get_event_time(cam_seq->front()) < _control_time
            )
            {
                cam_seq->pop();
            }
        }
    }

    INFO_LOG << "successfully processed event packet " << packet_id << std::endl;

    return result::success;
}

milliseconds
CameraControl::
_get_event_time(const Event & event) const
{
    const auto & event_time_ms = _event_map.find(event.event_id);
    if (event_time_ms != _event_map.end())
    {
        return event_time_ms->second + event.event_time_offset_ms;
    }
    return MAX_TIME;
}

milliseconds
CameraControl::
_get_next_event_time() const
{
    milliseconds next_event = MAX_TIME;

    for (const auto & [serial, cam_ptr] : _cameras)
    {
        if (not cam_ptr->read_settings().connected)
        {
            continue;
        }
        const auto & id = _serial_to_id.find(serial);
        if (id == _serial_to_id.end())
        {
            continue;
        }
        const auto & seq = _sequence_map.find(id->second);
        if (seq == _sequence_map.end() or seq->second->empty())
        {
            continue;
        }

        next_event = std::min(
            next_event,
            _get_event_time(seq->second->front())
        );
    }

    return next_event;
}


result
CameraControl::
_dispatch_camera_events()
{
    for (auto & [serial, cam_ptr] : _cameras)
    {
        const auto & id = _serial_to_id.find(serial);
        if (id == _serial_to_id.end())
        {
            continue;
        }
        const auto & seq_itor = _sequence_map.find(id->second);
        if (seq_itor == _sequence_map.end() or seq_itor->second->empty())
        {
            continue;
        }

        auto & seq = seq_itor->second;

        if (seq->empty())
        {
            continue;
        }

        auto event_time = _get_event_time(seq->front());

        while (event_time <= _control_time)
        {
            // Execute the camera event.
            auto res1 = cam_ptr->handle(seq->front());

            if (res1 == result::failure)
            {
                ERROR_LOG << "camera->handle(event) failed" << std::endl;
                // TODO count camera errors and report to UI.
            }

            // Move to the next event.
            seq->pop();
            if (seq->empty())
            {
                // Hit the end of the event sequence.
                break;
            }

            event_time = _get_event_time(seq->front());

            // Flush camera settings if the next event is trigger.
            if (seq->front().channel == Channel::trigger)
            {
                auto res2 = cam_ptr->write_settings();
                if (res2 == result::failure)
                {
                    ERROR_LOG << "camera->write-settings() failed" << std::endl;
                    // TODO count camera errors and report to UI.
                }
            }
        }
    }
    return result::success;
}


result
CameraControl::
dispatch()
{
    auto next_state = CameraControl::State::init;
    bool send_telemetry = false;
    bool scan_cameras = false;

    _control_time = now();

    switch(_state)
    {
        case CameraControl::State::init:
        {
            _scan_time += _control_time;
            _send_time += _control_time;
            _read_time += _control_time;
            next_state = State::scan;
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

            // Transition to the next state if we have an event time for any
            // currently connected camera.
            const auto next_event_time = _get_next_event_time();

            if (next_event_time < MAX_TIME)
            {
                next_state = CameraControl::State::monitor;
            }
            break;
        }

        case CameraControl::State::execute_ready:
        {
            scan_cameras = true;
            send_telemetry = true;

            const auto next_event_time = _get_next_event_time();

            if (next_event_time < (_control_time + 60'000))
            {
                next_state = CameraControl::State::execute_ready;
            }
            break;
        }

        case CameraControl::State::executing:
        {
            scan_cameras = false;
            send_telemetry = true;

            ABORT_ON_FAILURE(
                _dispatch_camera_events(),
                "_dispatch_camera_events() failed",
                result::failure
            );

            const auto next_event_time = _get_next_event_time();

            // Transition back to execute_ready if the next event is far away.
            if (next_event_time >= (_control_time + 60'000))
            {
                next_state = State::execute_ready;
            }
            break;
        }

        default:
        {
            ABORT_IF(true, "Oops, state" << _state << " is't implemneted!", result::failure);
            break;
        }
    }

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
        _scan_time = _control_time + 2000;  // 0.5 Hz.
        if (scan_cameras)
        {
            _camera_scan();
        }
    }


    // Read any commands.
    if (_read_time <= _control_time)
    {
        _read_time = _control_time + 1000; // 1 Hz.

        ABORT_ON_FAILURE(
            _read_camera_renames(),
            "_read_camera_renames() failed",
            result::failure
        );
        ABORT_ON_FAILURE(
            _read_events(),
            "_read_events() failed",
            result::failure
        );
    }

    // Send out 1 Hz telemetry.
    if (_send_time <= _control_time)
    {
        _send_time = _control_time + 1000;  // 1 Hz.
        if (send_telemetry)
        {
            ABORT_ON_FAILURE(
                _send_detected_cameras(),
                "_send_detected_cameras() failed",
                result::failure
            );

            ABORT_ON_FAILURE(
                _send_sequence_state(),
                "_send_sequence_state() failed",
                result::failure
            );
        }
    }

    return result::success;
}


}
