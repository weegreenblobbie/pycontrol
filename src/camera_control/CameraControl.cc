#include <common/str_utils.h>
#include <camera_control/Camera.h>
#include <camera_control/CameraControl.h>
#include <camera_control/CameraSequence.h>
#include <camera_control/CameraSequenceFileReader.h>

#include <interface/UdpSocket.h>
#include <interface/GPhoto2Cpp.h>
#include <interface/WallClock.h>

#include <cmath>
#include <numeric>

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
    _telem_message << "{\"state\":\"" << _state << "\",";

    //-------------------------------------------------------------------------
    // time
    //
    _telem_message << "\"time\":" << _control_time << ",";

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
                << "\"num_photos\":" << info.num_photos;

            if (cam_ptr->have_num_avail())
            {
                _telem_message << ",\"num_avail\":" << info.num_avail;
            }
            if (cam_ptr->have_burst_number())
            {
                _telem_message << ",\"burst_number\":" << info.burst_number;
            }
            _telem_message << "}";

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
    _telem_message << "],";

    //-------------------------------------------------------------------------
    // timelapse
    //
    switch (_state)
    {
        case State::timelapse_idle:
        case State::timelapse_running:
        {
            _telem_message << "\"timelapse\":{\"histogram\":[";

            if (_cameras.contains(_timelapse_serial))
            {
                auto cam = _cameras[_timelapse_serial];
                const auto hist_vec = cam->histogram();
                if (not hist_vec.empty())
                {
                    for (const auto h : hist_vec)
                    {
                        _telem_message << h << ",";
                    }
                    // Overwrite last ',';
                    _telem_message.seekp(-1, std::ios_base::cur);
                }
            }

            const std::string serial = _timelapse_serial.empty() ? "none" : _timelapse_serial;
            const float interval = static_cast<float>(_timelapse_interval) / 1000.0f;

            const int target_bin = _timelapse_target_bin + _timelapse_target_offset;
            const int current_bin = target_bin + _timelapse_target_error;

            _telem_message
                << "],"
                << "\"serial\":\""       << serial                    << "\","
                << "\"interval\":"       << interval                  << ","
                << "\"min_shutter\":"    << _timelapse_min_shutter    << ","
                << "\"max_shutter\":"    << _timelapse_max_shutter    << ","
                << "\"min_iso\":"        << _timelapse_min_iso        << ","
                << "\"max_iso\":"        << _timelapse_max_iso        << ","
                << "\"min_hist_mask\":"  << _timelapse_min_hist_mask  << ","
                << "\"max_hist_mask\":"  << _timelapse_max_hist_mask  << ","
                << "\"min_deadband\":"   << _timelapse_min_deadband   << ","
                << "\"max_deadband\":"   << _timelapse_max_deadband   << ","
                << "\"current_bin\":"    << current_bin               << ","
                << "\"target_bin\":"     << target_bin                << ","
                << "\"target_offset\":"  << _timelapse_target_offset  << ","
                << "\"target_percent\":" << _timelapse_target_percent << ","
                << "\"target_error\":"   << _timelapse_target_error   << ","
                << "\"num_captures\":"   << _timelapse_capture_count  << ","
                << "\"pixel_count\":"    << _timelapse_pixel_count
                << "}";

            break;
        }
        default:
        {
            _telem_message << "\"timelapse\":{}";
            break;
        }
    }

    _telem_message << "}";

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
_read_command(bool & got_message, State & next_state)
{
    got_message = false;

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

    // Copy and strip extraneous whitespace.
    std::string raw_cmd(_command_buffer.c_str());
    strip(raw_cmd, ' ');
    strip(raw_cmd, '\n');

    std::istringstream iss(raw_cmd);

    if (not (iss >> cmd_id))
    {
        ++_last_rejected_command_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"failed to parse command ID from '" << _command_buffer << "'\"}";
        _command_response = oss.str();
        got_message = true;
        return result::success;
    }

    if (not (iss >> command))
    {
        _last_rejected_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"failed to parse command from '" << _command_buffer << "'\"}";
        _command_response = oss.str();
        got_message = true;
        return result::success;
    }

    // Nothing new to process.  How do we handle cmd_id rollover?
    if (cmd_id <= std::max(_last_accepted_command_id, _last_rejected_command_id))
    {
        return result::success;
    }

    DEBUG_LOG << "read_command(): control_time: " << _control_time << " cmd: '" << _command_buffer << "'" << std::endl;

    // We have a message, reset the send time so there's no latency to
    // responding to commands.
    got_message = true;

    //-------------------------------------------------------------------------
    // set_camera_id
    //
    if (command == "set_camera_id")
    {
        std::string serial;
        std::string cam_id;
        if (not (iss >> serial >> cam_id))
        {
            _last_rejected_command_id = cmd_id;
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"Got bad camera rename message: '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(serial))
        {
            _last_rejected_command_id = cmd_id;
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"serial '" << serial << "' does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (_id_to_serial.contains(cam_id))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "id '" << cam_id << "' already exists";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        INFO_LOG << "mapping serial " << serial << " to " << cam_id << "\n";

        // First, remove any previous name.
        const auto & old_id = _serial_to_id[serial];
        _id_to_serial.erase(old_id);

        // Update with the new id.
        _serial_to_id[serial] = cam_id;
        _id_to_serial[cam_id] = serial;
    }
    //-------------------------------------------------------------------------
    // set_events
    //
    else if (command == "set_events")
    {
        bool parse_error = false;
        auto new_event_map = event_map();
        while (not iss.eof())
        {
            std::string event_id;
            milliseconds timestamp;

            parse_error = parse_error or not (iss >> event_id);
            parse_error = parse_error or not (iss >> timestamp);

            if (parse_error)
            {
                break;
            }

            new_event_map[event_id] = timestamp;
        }

        if (parse_error)
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "failed to parse events from '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        // All good, update the event map!
        _event_map = new_event_map;
    }
    //-------------------------------------------------------------------------
    // load_sequence
    //
    else if (command == "load_sequence")
    {
        std::string sequence_fn;
        if (not (std::getline(iss, sequence_fn)))
        {
            _last_rejected_command_id = cmd_id;
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"failed to parse command '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }
        strip(sequence_fn, ' ');

        // Load the camer sequence file.
        auto seq_reader = CameraSequenceFileReader();
        if (result::failure == seq_reader.read_file(sequence_fn))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "failed to parse camera sequence file '" << sequence_fn << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
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
                _last_rejected_command_id = cmd_id;
                oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                    << ",\"last_rejected_id\":" << _last_rejected_command_id
                    << ",\"message\":\"failed to allocate CameraSequence\"}";
                _command_response = oss.str();
                return result::success;
            }
            if (result::failure == cam_seq->load(id, event_seq))
            {
                _last_rejected_command_id = cmd_id;
                oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                    << ",\"last_rejected_id\":" << _last_rejected_command_id
                    << ",\"message\":\"CameraSequence.load() failed\"}";
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
            _last_rejected_command_id = cmd_id;
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\""
                << "Failed to parse read_choices command: '" << _command_buffer << "'\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(serial))
        {
            _last_rejected_command_id = cmd_id;
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"serial '" << serial << "' does not exist\"}";
            _command_response = oss.str();
            return result::success;
        }

        auto choice_vec = _cameras[serial]->read_choices(property);

        // If the vector is empty, probably doesn't exist.
        if (choice_vec.empty())
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "property '" << property << "' does not exist";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\""
            << ",\"data\":[";
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
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse set_choice command: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(serial))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "serial '" << serial << "' does not exist";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }
        strip(value, ' ');

        auto cam = _cameras[serial];

        INFO_LOG << "setting '" << property << "' to '" << value << "'" << std::endl;

        if (result::failure == cam->write_property(property, value))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "property '" << property << "' does not exist";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (result::failure == cam->write_config())
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "writing '" << property << "' with '" << value << "' failed";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "timelapse_enable")
    {
        switch (_state)
        {
            // If we're already running, ingore, probably due to webapp UI
            // reloading.
            // TODO: We should fix the webapp instead.
            case CameraControl::State::timelapse_running:
            {
                next_state = CameraControl::State::timelapse_running;
                break;
            }
            default:
            {
                next_state = CameraControl::State::timelapse_idle;
            }
        }
        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "timelapse_update")
    {
        if (not (iss >> _timelapse_serial))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse serial from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(_timelapse_serial))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "serial '" << _timelapse_serial << "' does not exist";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }
        auto camera = _cameras[_timelapse_serial];

        float interval;
        if (not (iss >> interval))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse interval from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        std::string min_shutter;
        std::string max_shutter;
        std::string min_iso;
        std::string max_iso;

        if (not (iss >> min_shutter))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse min_shutter from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not (iss >> max_shutter))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse max_shutter from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not (iss >> min_iso))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse min_iso from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not (iss >> max_iso))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse max_iso from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        int min_hist_mask;
        int max_hist_mask;

        if (not (iss >> min_hist_mask))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse min_hist_mask from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not (iss >> max_hist_mask))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse max_hist_mask from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        int min_deadband;
        int max_deadband;

        if (not (iss >> min_deadband))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse min_deadband from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not (iss >> max_deadband))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse max_deadband from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        int target_offset;
        float target_percent;

        if (not (iss >> target_offset))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse target_offset from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not (iss >> target_percent))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse timelapse target_percent from: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (target_percent < 0.0f or target_percent > 1.0f)
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Bad target_percent: " << target_percent;
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        // Interval is in seconds, so scale it to milliseconds.
        _timelapse_interval = static_cast<milliseconds>(interval * 1000.0f);

        _timelapse_min_shutter = camera->shutter_speed(min_shutter);
        _timelapse_max_shutter = camera->shutter_speed(max_shutter);
        _timelapse_min_iso = camera->iso(min_iso);
        _timelapse_max_iso = camera->iso(max_iso);
        _timelapse_min_hist_mask = min_hist_mask;
        _timelapse_max_hist_mask = max_hist_mask;
        _timelapse_min_deadband = min_deadband;
        _timelapse_max_deadband = max_deadband;
        _timelapse_target_offset = target_offset;
        _timelapse_target_percent = target_percent;

        next_state = _state;
        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "timelapse_start")
    {
        next_state = CameraControl::State::timelapse_running;
        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "timelapse_stop")
    {
        next_state = CameraControl::State::timelapse_idle;
        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "timelapse_disable")
    {
        // reset capture count.
        next_state = CameraControl::State::monitor;
        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }
    else if (command == "trigger")
    {
        std::string serial;
        if (not (iss >> serial))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "Failed to parse trigger command: '" << _command_buffer << "'";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }

        if (not _cameras.contains(serial))
        {
            _last_rejected_command_id = cmd_id;
            {
                std::ostringstream msg_oss;
                msg_oss << "serial '" << serial << "' does not exist";
                _last_rejected_message = msg_oss.str();
            }
            oss << "{\"last_accepted_id\":" << _last_accepted_command_id
                << ",\"last_rejected_id\":" << _last_rejected_command_id
                << ",\"message\":\"" << _last_rejected_message << "\"}";
            _command_response = oss.str();
            return result::success;
        }
        _trigger_serial = serial;

        switch (_state)
        {
            case CameraControl::State::timelapse_idle:
            case CameraControl::State::timelapse_running:
            {
                _trigger_type = TriggerType::histogram;
                break;
            }
            default:
            {
                _trigger_type = TriggerType::trigger;
                break;
            }
        }

        _last_accepted_command_id = cmd_id;
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        return result::success;
    }

    // Unknown command error.
    else if (command != "reset_sequence")
    {
        _last_rejected_command_id = cmd_id;
        {
            std::ostringstream msg_oss;
            msg_oss << "Unknown command: '" << command << "', ignorning";
            _last_rejected_message = msg_oss.str();
        }
        oss << "{\"last_accepted_id\":" << _last_accepted_command_id
            << ",\"last_rejected_id\":" << _last_rejected_command_id
            << ",\"message\":\"" << _last_rejected_message << "\"}";
        _command_response = oss.str();
        ERROR_LOG << _last_rejected_message << std::endl;
        return result::success;
    }

    //-------------------------------------------------------------------------
    // reset_sequence
    //
    // Note breaking up the if else chain so we can reset the sequence when a
    // sequence file is loaded.
    if (command == "reset_sequence")
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

    _last_accepted_command_id = cmd_id;
    oss << "{\"last_accepted_id\":" << _last_accepted_command_id
        << ",\"last_rejected_id\":" << _last_rejected_command_id
        << ",\"message\":\"" << _last_rejected_message << "\"}";
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
_timelapse_dispatch()
{
    if (_timelapse_time > _control_time)
    {
        return result::success;
    }

    if (_timelapse_time == 0)
    {
        _timelapse_time = _control_time;
    }
    _timelapse_time += _timelapse_interval;

    // Trigger the camera and process the histogram.
    auto itor = _cameras.find(_timelapse_serial);
    if (itor == _cameras.end())
    {
        ERROR_LOG << "_timelapse_serial not found in _cameras, aborting" << std::endl;
        return result::failure;
    }

    auto camera = itor->second;

    if (result::success != camera->capture_histogram())
    {
        ERROR_LOG << "camera->capture_histogram() failed, aborting" << std::endl;
        return result::failure;
    }
    _timelapse_capture_count += 1;

    // Grab the histogram an count the total number of pixels.
    const auto histogram = camera->histogram();
    auto begin = histogram.begin();
    if (_timelapse_min_hist_mask >= 0 and _timelapse_max_hist_mask <= 255)
    {
        begin += _timelapse_min_hist_mask;
    }
    auto end = histogram.end() - 1;
    if (_timelapse_max_hist_mask >= 0 and _timelapse_max_hist_mask <= 255)
    {
        end -= (255 - _timelapse_max_hist_mask);
    }

    // Out of bounds?
    if (begin < histogram.begin() or
        begin >= histogram.end() or
        end <= begin or
        end >= histogram.end())
    {
        _timelapse_min_hist_mask = -1;
        _timelapse_max_hist_mask = 256;
        begin = histogram.begin();
        end = histogram.end();
    }

    const int end_idx = end - histogram.begin();
    const int begin_idx = begin - histogram.begin();

    if (begin_idx < 0 or begin_idx > 255)
    {
        ERROR_LOG << "begin_idx " << begin_idx << " out of bounds!" << std::endl;
        return result::failure;
    }
    if (end_idx < 0 or end_idx > 255 or end_idx <= begin_idx)
    {
        ERROR_LOG << "end_idx " << end_idx << " out of bounds!" << std::endl;
        return result::failure;
    }

    _timelapse_pixel_count = 0;
    for (int i = begin_idx; i <= end_idx; ++i)
    {
        _timelapse_pixel_count += histogram[i];
    }

    // Highlights, take the top 5% of the pixels.
    const std::uint64_t top_threshold = static_cast<std::uint64_t>(
        static_cast<float>(_timelapse_pixel_count) * _timelapse_target_percent + 0.5f);

    // Compute the current luminacne for the target percent, what histogram bin
    // accounts for the top taret percent?
    std::uint64_t highlight_sum = 0;
    int current_bin = 0;

    for(int i = end_idx; i >= begin_idx; --i)
    {
        highlight_sum += histogram[i];
        // Reached the target percent?
        if (highlight_sum >= top_threshold)
        {
            current_bin = i;
            break;
        }
    }

    // Exposure lock, if we haven't initalized the target luminance, do it now.
    if (_timelapse_target_bin < 0.0f)
    {
        _timelapse_target_bin = current_bin;
        return result::success;
    }

    // Effective target bin.
    const auto target_bin = _timelapse_target_bin + _timelapse_target_offset;

    _timelapse_target_error = current_bin - target_bin;

    const auto current_iso = camera->iso();
    const auto current_shutter_speed = camera->shutter_speed();

    // We always want to drive ISO to the minimim if we can.
    if (current_iso > _timelapse_min_iso and
        current_shutter_speed < _timelapse_max_shutter)
    {
        camera->step_iso(-1);
        camera->step_shutter_speed(1);
    }

    const auto too_bright_threshold = target_bin + _timelapse_max_deadband;
    const auto too_dark_threshold   = target_bin - _timelapse_min_deadband;

    // Too bright, make the image to be DARKER.
    if (current_bin >= too_bright_threshold)
    {
        if (current_iso > _timelapse_min_iso)
        {
            camera->step_iso(-1);
        }
        else if (current_shutter_speed > _timelapse_min_shutter)
        {
            camera->step_shutter_speed(-1);
        }
    }

    // Too dark, make the image BRIGHTER.
    else if (current_bin <= too_dark_threshold)
    {
        if (current_shutter_speed < _timelapse_max_shutter)
        {
            camera->step_shutter_speed(1);
        }
        else if (current_iso < _timelapse_max_iso)
        {
            camera->step_iso(1);
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

    bool got_message = false;

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

        case CameraControl::State::timelapse_idle:
        {
            next_state = CameraControl::State::timelapse_idle;
            _timelapse_capture_count = 0;
            _timelapse_target_error = 0;
            _timelapse_target_bin = -1;
            _timelapse_time = 0;
            scan_cameras = false;
            break;
        }

        case CameraControl::State::timelapse_running:
        {
            next_state = CameraControl::State::timelapse_running;
            scan_cameras = false;
            if (result::success != _timelapse_dispatch())
            {
                ERROR_LOG << "_timelapse_dispatch() failed, ignoring" << std::endl;
            }
            break;
        }

        default:
        {
            ABORT_IF(true, "Oops, state" << _state << " is't implemneted!", result::failure);
            break;
        }
    }

    if (result::failure == _read_command(got_message, next_state))
    {
        ERROR_LOG << "_read_command() failed, ignoring" << std::endl;
    }

    if (_state != next_state)
    {
        INFO_LOG << "time: " << _control_time
                 << " state: " << _state << " -> " << next_state
                 << std::endl;

        _state = next_state;
    }

    if (got_message)
    {
        if (result::failure == _send_telemetry())
        {
            ERROR_LOG << "_send_telemetry() failed, ignoring" << std::endl;
        }
    }
    else if (_send_time <= _control_time)
    {
        // Send out 4 Hz telemetry unless we are monitoring cameras.
        if (scan_cameras or
            _state == State::timelapse_idle or
            _state == State::timelapse_running)
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

    // Trigger if requsted.
    if (_trigger_type != TriggerType::none)
    {
        auto cam = _cameras[_trigger_serial];
        const auto & entry = _serial_to_id.find(_trigger_serial);
        const auto desc = entry != _serial_to_id.end() ?
                          entry->second :
                          cam->info().desc;
        INFO_LOG << "Trigger camera " << desc << std::endl;

        if (_trigger_type == TriggerType::trigger)
        {
            INFO_LOG << "Trigger camera " << desc << std::endl;
            if (result::failure == cam->trigger())
            {
                ERROR_LOG << "cam->trigger() failed, ignoring" << std::endl;
            }
        }
        else if (_trigger_type == TriggerType::histogram)
        {
            if(result::failure == cam->capture_histogram())
            {
                ERROR_LOG << "cam->capture_histogram() failed, ignoring" << std::endl;
            }
        }
    }
    _trigger_type = TriggerType::none;

    // Scan for camera changes.
    if (_scan_time <= _control_time)
    {
        _scan_time = _control_time + 1000;  // 1 Hz.
        if (scan_cameras)
        {
            _camera_scan();
        }
    }

    return result::success;
}


} /* namespace pycontrol */
