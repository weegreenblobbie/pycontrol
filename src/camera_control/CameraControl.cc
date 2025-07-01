


#include <common/str_utils.h>
#include <camera_control/Camera.h>
#include <camera_control/CameraControl.h>
#include <camera_control/CameraSequence.h>
#include <camera_control/CameraSequenceFileReader.h>

#include <interface/UdpSocket.h>
#include <interface/GPhoto2Cpp.h>
#include <interface/WallClock.h>


namespace pycontrol
{


CameraControl::
CameraControl(
    interface::UdpSocket & cmd_socket,
    interface::UdpSocket & telem_socket,
    interface::GPhoto2Cpp & gp2cpp,
    interface::WallClock & clock,
    const kv_pair_vec & cam_to_ids) : _command_socket(cmd_socket),
                                      _telem_socket(telem_socket),
                                      _gp2cpp(gp2cpp),
                                      _clock(clock)

{
    for (const auto & [serial, id] : cam_to_ids)
    {
        INFO_LOG << "init(): mapping camera alias: " << serial << " to " << id << "\n";
        _serial_to_id[serial] = id;
        _id_to_serial[id] = serial;
    }
}

void
CameraControl::
_camera_scan()
{
    const auto new_detections = _gp2cpp.auto_detect();
    const auto detections = port_set(new_detections.begin(), new_detections.end());
    const bool cameras_changed = detections != _current_ports;
    if (cameras_changed)
    {
        // Add or update cameras.
        for (const auto & port : detections)
        {
            if (not _current_ports.contains(port))
            {
                auto camera = _gp2cpp.open_camera(port);
                if (not camera)
                {
                    INFO_LOG << "gphoto2cpp::open_camera() failed, ignoring" << std::endl;
                    continue;
                }

                if (not _gp2cpp.read_config(camera))
                {
                    ERROR_LOG
                        << "gphoto2cpp::read_config(camera) failed"
                        << std::endl;
                    continue;
                };

                std::string serial;
                if (not _gp2cpp.read_property(camera, "serialnumber", serial))
                {
                    ERROR_LOG
                        << "gphoto2cpp::read_property(\"serialnumber\") failed"
                        << std::endl;
                    continue;

                }

                // Add new camera.
                if (not _cameras.contains(serial))
                {
                    auto cam = std::make_shared<Camera>(
                        _gp2cpp,
                        camera,
                        port,
                        serial,
                        "camera.config"
                    );
                    _cameras[serial] = cam;
                    if (not _serial_to_id.contains(serial))
                    {
                        const auto & id = cam->info().desc;
                        _serial_to_id[serial] = id;
                        _id_to_serial[id] = serial;
                    }
                }

                // Update existing camera.
                else
                {
                    _cameras[serial]->reconnect(camera, port);
                }

                INFO_LOG
                    << "adding "
                    << "port=" << port << " "
                    << "serial=" << serial << " "
                    << "desc=" << _serial_to_id[serial] << "\n";

                _current_ports.insert(port);
            }
        }

        // Mark any cameras not detected as disconnected.
        for (auto & pair : _cameras)
        {
            auto & cam = pair.second;
            const auto & info = cam->info();
            const auto & port = info.port;
            if (not detections.contains(port))
            {
                std::size_t num_erased = _current_ports.erase(port);
                if (num_erased > 0)
                {
                    INFO_LOG
                        << "removing "
                        << "port=" << port << " "
                        << "serial=" << info.serial << " "
                        << "desc=" << _serial_to_id[info.serial] << "\n";
                    cam->disconnect();
                }
            }
        }
    }

    // Always read the camera configuration to reflect the camera state.
    for (auto & [_, camera] : _cameras)
    {
        camera->read_config();
    }
}


result
CameraControl::
_send_telemetry()
{
    _telem_message.seekp(0, std::ios::beg);

    //-------------------------------------------------------------------------
    // state
    //
    _telem_message << "{\"state\":\"";
    switch(_state)
    {
        case State::init: { _telem_message << "init"; break;}
        case State::scan: { _telem_message << "scan"; break;}
        case State::monitor: { _telem_message << "monitor"; break;}
        case State::execute_ready: { _telem_message << "execute_ready"; break;}
        case State::executing: { _telem_message << "executing"; break;}
    }
    _telem_message << "\",";

    //-------------------------------------------------------------------------
    // command_response
    //
    _telem_message << "\"command_response\":" << _command_response << ",";

    //-------------------------------------------------------------------------
    // detected_cameras
    //
    _telem_message << "\"detected_cameras\":[";
    {
        std::size_t idx = 0;
        for (const auto & [serial, cam_ptr] : _cameras)
        {
            _telem_message << "{";

            const auto & info = cam_ptr->info();
            const auto & entry = _serial_to_id.find(serial);

            const auto desc = entry != _serial_to_id.end() ?
                              entry->second :
                              info.desc;

            // TODO: cam_ptr->to_json(desc);

            _telem_message
                << std::boolalpha
                << "\"connected\":"  << info.connected     << ","
                << "\"serial\":\""   << info.serial        << "\","
                << "\"port\":\""     << info.port          << "\","
                << "\"desc\":\""     << desc               << "\","
                << "\"mode\":\""     << info.mode          << "\","
                << "\"shutter\":\""  << info.shutter       << "\","
                << "\"fstop\":\""    << info.fstop         << "\","
                << "\"iso\":\""      << info.iso           << "\","
                << "\"quality\":\""  << info.quality       << "\","
                << "\"batt\":\""     << info.battery_level << "\","
                << "\"num_photos\":" << info.num_photos    << "}";

            if (++idx < _cameras.size()) _telem_message << ",";
        }
    }
    _telem_message << "],";

    //-------------------------------------------------------------------------
    // events
    //
    _telem_message << "\"events\":{";
    {
        std::size_t idx = 0;
        for (const auto & [event_id, timestamp] : _event_map)
        {
            _telem_message << "\"" << event_id << "\":" << timestamp;
            if (++idx < _event_map.size()) _telem_message << ",";
        }
    }
    _telem_message << "},";

    //-------------------------------------------------------------------------
    // sequence
    //
    _telem_message << "\"sequence\":\"" << _sequence_filename << "\",";

    //-------------------------------------------------------------------------
    // sequence_state
    //
    _telem_message << "\"sequence_state\":[";
    {
        std::size_t idx = 0;
        for (const auto & [cam_id, sequence] : _sequence_map)
        {
            _telem_message
                << "{\"num_events\":" << sequence->size() << ","
                << "\"id\":\"" << cam_id << "\","
                << "\"events\":[";

            auto itor = sequence->begin();
            auto pos = sequence->pos();
            int count = 0;
            bool erase_comma = false;
            while (itor < sequence->end())
            {
                if (++count > 10)
                {
                    break;
                }
                const auto & event = *itor;
                const auto event_time = _get_event_time(event);

                const auto eta = event_time == MAX_TIME ?
                    "N/A" :
                    convert_milliseconds_to_hms(event_time - _control_time);

                _telem_message
                    << "{"
                    << "\"pos\":" << pos++ << ","
                    << "\"event_id\":\"" << event.event_id << "\","
                    << "\"event_time_offset\":\"" << convert_milliseconds_to_hms(event.event_time_offset_ms) << "\","
                    << "\"eta\":\"" << eta << "\","
                    << "\"channel\":\"" << cam_id << "." << to_string(event.channel) << "\","
                    << "\"value\":\"" << event.channel_value << "\""
                    << "}";

                erase_comma = true;
                _telem_message << ",";

                ++itor;
            }
            // Overwrite the trailing comma.
            if (erase_comma)
            {
                auto last_pos = _telem_message.tellp() - std::streamoff(1);
                _telem_message.seekp(last_pos);
            }
            _telem_message << "]}";
        }

        if (++idx < _sequence_map.size())
        {
            _telem_message << ",";
        }
    }
    _telem_message << "]}";

    // Mark the end of the stream and rewind before sending.
    _telem_message << '\0';
    _telem_message.seekp(0, std::ios::beg);

    ABORT_ON_FAILURE(
        _telem_socket.send(_telem_message.str()),
        "UdpSocket::send() failed",
        result::failure
    );

    return result::success;
}

result
CameraControl::
_read_command()
{
    ABORT_ON_FAILURE(
        _command_socket.recv(_command_buffer),
        "UdpSocket::read() failed",
        result::failure
    );

    // No message avialable.
    if (_command_buffer.empty() or _command_buffer.size() == 0)
    {
        return result::success;
    }

    std::uint32_t cmd_id = 0;
    std::string command;

    std::ostringstream oss;
    oss << "{\"id\":" << _command_id << ",\"success\":";

    std::istringstream iss(_command_buffer);
    if (not (iss >> cmd_id >> command))
    {
        oss << "false,\"message\":\"failed to parse command '" << _command_buffer << "'\"}";
        _command_response = oss.str();
        return result::success;
    }

    // Nothing new to process.
    if (cmd_id == _command_id)
    {
        return result::success;
    }

    _command_id = cmd_id;

    oss.str("");
    oss << "{\"id\":" << _command_id << ",\"success\":";

    //-------------------------------------------------------------------------
    // set_camera_id
    //
    if (command == "set_camera_id")
    {
        std::string serial;
        std::string cam_id;
        if (not (iss >> serial >> cam_id))
        {
            oss << "false,\"message\":\""
                << "Got bad camera rename message: '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _serial_to_id.contains(serial))
        {
            oss << "false,\"message\":\"serial \"" << serial << "\" does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (_id_to_serial.contains(cam_id))
        {
            oss << "false,\"message\":\"id \"" << cam_id << "\" already exists\"}";
            _command_response = oss.str();
            return result::success;
        }

        INFO_LOG << "mapping serial " << serial << " to " << cam_id << "\n";

        _serial_to_id[serial] = cam_id;
        _id_to_serial[cam_id] = serial;
    }
    //-------------------------------------------------------------------------
    // set_events
    //
    else if (command == "set_events")
    {
        _event_map.clear();
        while (true)
        {
            std::string event_id;
            milliseconds timestamp;
            if (not (iss >> event_id >> timestamp))
            {
                break;
            }
            _event_map[event_id] = timestamp;
        }

        if (_event_map.empty())
        {
            oss << "false,\"message\":\"failed to parse events from '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }
    }
    //-------------------------------------------------------------------------
    // load_sequence
    //
    else if (command == "load_sequence")
    {
        std::string sequence_fn;
        if (not (std::getline(iss, sequence_fn)))
        {
            oss << "false,\"message\":\"failed to parse command '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }
        strip(sequence_fn, ' ');

        // Load the camer sequence file.
        auto seq_reader = CameraSequenceFileReader();
        if (result::failure == seq_reader.read_file(sequence_fn))
        {
            oss << "false,\"message\":\"failed to parse camera sequence file '"
                << sequence_fn << "'}";
            _command_response = oss.str();
            return result::success;
        }
        _sequence_filename = sequence_fn;
        _sequence_map.clear();

        const auto & event_seq = seq_reader.get_events();
        const auto & cam_ids = seq_reader.get_camera_ids();

        for (const auto & id : cam_ids)
        {
            auto cam_seq = std::make_shared<CameraSequence>();
            if (not cam_seq)
            {
                oss << "false,\"message\":\"failed to allocate CameraSequence\"}";
                _command_response = oss.str();
                return result::success;
            }
            if (result::failure == cam_seq->load(id, event_seq))
            {
                oss << "false,\"message\":\"CameraSequence.load() failed\"}";
                _command_response = oss.str();
                return result::success;
            }
            _sequence_map[id] = cam_seq;
        }

        command = "reset_sequence";
    }
    //-------------------------------------------------------------------------
    // read_choices
    //
    else if (command == "read_choices")
    {
        std::string serial;
        std::string property;
        if (not (iss >> serial >> property))
        {
            oss << "false,\"message\":\""
                << "Failed to parse read_choices command: '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(serial))
        {
            oss << "false,\"message\":\"serial '" << serial << "' does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }

        auto choice_vec = _cameras[serial]->read_choices(property);

        // If the vector is empty, probably doesn't exist.
        if (choice_vec.empty())
        {
            oss << "false,\"message\":\""
                << "property '" << property << "' does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }

        oss << "true,\"data\":[";
        std::size_t idx = 0;
        for (const auto & choice : choice_vec)
        {
            oss << "\"" << choice << "\"";
            if (++idx < choice_vec.size()) oss << ",";
        }
        oss << "]}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "set_choice")
    {
        std::string serial;
        std::string property;
        std::string value;
        if (not ((iss >> serial >> property) and (std::getline(iss, value))))
        {
            oss << "false,\"message\":\""
                << "Failed to parse set_choice command: '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(serial))
        {
            oss << "false,\"message\":\"serial '" << serial << "' does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }
        strip(value, ' ');

        auto cam = _cameras[serial];

        INFO_LOG << "setting '" << property << "' to '" << value << "'" << std::endl;

        if (result::failure == cam->write_property(property, value))
        {
            oss << "false,\"message\":\""
                << "property '" << property << "' does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (result::failure == cam->write_config())
        {
            oss << "false,\"message\":\""
                << "writing '" << property << "' with '" << value << "' failed\"}";
            _command_response = oss.str();
            return result::success;
        }

        oss << "true}";
        _command_response = oss.str();
        return result::success;
    }
    //-------------------------------------------------------------------------
    // reset_sequence
    else if (command != "reset_sequence")
    {
        oss << "false,\"message\":\"Unknown command: '" << command << "'\"}";
        _command_response = oss.str();
        return result::success;
    }

    //-------------------------------------------------------------------------
    // reset_sequence
    //
    // Note breaking up the if else chain so we can reset the sequence when a
    // sequence file is loaded.
    if (command == "reset_sequence")
    {
        _event_map.clear();
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

    oss << "true}";
    _command_response = oss.str();
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
        if (not cam_ptr->info().connected)
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

            // Flush camera settings if the next event is a trigger.
            if (seq->front().channel == Channel::trigger)
            {
                auto res2 = cam_ptr->write_config();
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
    bool scan_cameras = true;

    _control_time = _clock.now();

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

            next_state = CameraControl::State::monitor;
            break;
        }

        case CameraControl::State::monitor:
        {
            // Transition to the next state if we have an event time for any
            // currently connected camera.
            const auto next_event_time = _get_next_event_time();

            if (next_event_time < MAX_TIME)
            {
                next_state = CameraControl::State::execute_ready;
            }
            else
            {
                next_state = CameraControl::State::monitor;
            }
            break;
        }

        case CameraControl::State::execute_ready:
        {
            const auto next_event_time = _get_next_event_time();

            if (next_event_time < (_control_time + 60'000))
            {
                next_state = CameraControl::State::executing;
            }
            else
            {
                next_state = CameraControl::State::execute_ready;
            }
            break;
        }

        case CameraControl::State::executing:
        {
            scan_cameras = false;

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
            else
            {
                next_state = CameraControl::State::executing;
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
        _scan_time = _control_time + 1000;  // 1 Hz.
        if (scan_cameras)
        {
            _camera_scan();
        }
    }

    // Read any commands.
    if (_read_time <= _control_time)
    {
        _read_time = _control_time + 250; // 4 Hz.

        if (result::failure == _read_command())
        {
            ERROR_LOG << "_read_command() failed, ignoring" << std::endl;
        }
    }

    if (_send_time <= _control_time)
    {
        // Send out 4 Hz telemetry unless we are monitoring cameras.
        if (scan_cameras)
        {
            _send_time = _control_time + 1000;  // 1 Hz.
        }
        else
        {
            _send_time = _control_time + 250;  // 4 Hz.
        }
        if (result::failure == _send_telemetry())
        {
            ERROR_LOG << "_send_telemetry() failed, ignoring" << std::endl;
        }
    }

    return result::success;
}


} /* namespace pycontrol */
