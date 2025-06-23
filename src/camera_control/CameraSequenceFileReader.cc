#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

#include <common/io.h>
#include <common/str_utils.h>
#include <camera_control/CameraSequenceFileReader.h>

namespace {

const std::set<std::string> _valid_shutter_speeds = {
    "1/8000",
    "1/6400",
    "1/5000",
    "1/4000",
    "1/3200",
    "1/2500",
    "1/2000",
    "1/1600",
    "1/1250",
    "1/1000",
    "1/800",
    "1/640",
    "1/500",
    "1/400",
    "1/320",
    "1/250",
    "1/200",
    "1/160",
    "1/125",
    "1/100",
    "1/80",
    "1/60",
    "1/50",
    "1/40",
    "1/30",
    "1/25",
    "1/20",
    "1/15",
    "1/13",
    "1/10",
    "1/8",
    "1/6",
    "1/5",
    "1/4",
    "1/3",
    "1/2.5",
    "1/2",
    "1/1.6",
    "1/1.3",
    "1",
    "1.3",
    "1.6",
    "2",
    "2.5",
    "3",
    "4",
    "5",
    "6",
    "8",
    "10",
    "13",
    "15",
    "20",
    "25",
    "30",
    "60",    // 1 minute
    "90",    // 1.5 minutes
    "120",   // 2 minutes
    "180",   // 3 minutes
    "240",   // 4 minutes
    "300",   // 5 minutes
    "480",   // 8 minutes
    "600",   // 10 minutes
    "720",   // 12 minutes
    "900",   // 15 minutes
};

const std::set<std::string> _valid_fstops = {
    "f/1.2",
    "f/1.4",
    "f/2",
    "f/2.2",
    "f/2.8",
    "f/3.2",
    "f/3.5",
    "f/4",
    "f/4.5",
    "f/5",
    "f/5.6",
    "f/6.3",
    "f/7.1",
    "f/8",
    "f/9",
    "f/10",
    "f/11",
    "f/13",
    "f/14",
    "f/16",
    "f/18",
    "f/20",
    "f/22",
    "f/25",
    "f/29",
    "f/32",
    "f/36",
};

const std::set<std::string> _valid_triggers = {
    "1",
};

} /* namespace */


namespace pycontrol
{


std::string
CameraSequenceFileReader::
_strip(const std::string& str) const
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first)
    {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

bool
CameraSequenceFileReader::
_is_ignorable_line(const std::string& line) const
{
    std::string trimmed_line = _strip(line);
    return trimmed_line.empty() || (trimmed_line[0] == '#');
}

bool
CameraSequenceFileReader::
_is_valid_shutter_speed(const std::string& value) const
{
    return _valid_shutter_speeds.contains(value);
}

bool
CameraSequenceFileReader::
_is_valid_fstop(const std::string& value) const
{
    return _valid_fstops.contains(value);
}

bool
CameraSequenceFileReader::
_is_valid_fps_value(const std::string& value) const
{
    if (value == "start" || value == "stop")
    {
        return true;
    }

    std::istringstream iss(value);
    float f;
    iss >> std::noskipws >> f;
    return iss.eof() && !iss.fail() && f >= 0.0f;
}

bool
CameraSequenceFileReader::
_is_valid_trigger_value(const std::string& value) const
{
    return _valid_triggers.contains(value);
}

result
CameraSequenceFileReader::
read_file(const std::string & file_path)
{
    clear();

    std::ifstream file(file_path);
    ABORT_IF_NOT(file.is_open(), "Could not open file " << file_path, result::failure);

    std::string line;
    unsigned int line_number = 0;
    while (std::getline(file, line))
    {
        line_number++;

        // Strip comments from the line first.
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
        {
            line = line.substr(0, comment_pos);
        }

        if (_is_ignorable_line(line))
        {
            continue;
        }

        std::istringstream iss(line);
        std::string event_id_str;
        std::string event_time_offset_str;
        std::string camera_channel_str;
        std::string channel_value_str;
        milliseconds event_time_offset_ms = 0;

        if (not (iss >> event_id_str
                      >> event_time_offset_str
                      >> camera_channel_str))
        {
            ERROR_LOG << file_path << "(" << line_number << "): "
                      << "Parse Error: Line does not contain the required first 3 columns."
                      << std::endl;
            clear();
            return result::failure;
        }

        // Convert the first 3 strings to lowercase.
        std::transform(event_id_str.begin(), event_id_str.end(), event_id_str.begin(), ::tolower);
        std::transform(event_time_offset_str.begin(), event_time_offset_str.end(), event_time_offset_str.begin(), ::tolower);
        std::transform(camera_channel_str.begin(), camera_channel_str.end(), camera_channel_str.begin(), ::tolower);

        // Then, use std::getline to read the entire remainder of the line into
        // the value string.
        std::getline(iss, channel_value_str);

        // Trim any leading and tailing whitespace.
        channel_value_str = _strip(channel_value_str);

        // Convert the final value string to lowercase.
        std::transform(channel_value_str.begin(), channel_value_str.end(), channel_value_str.begin(), ::tolower);


        // Parse event_time_offset_str...
        // (The rest of your function remains exactly the same)
        if (event_time_offset_str.find(':') == std::string::npos)
        {
            float seconds = 0;
            if (as_type<float>(event_time_offset_str, seconds))
            {
                ERROR_LOG << file_path << "(" << line_number << "): "
                          << "Parser Error: failed to parse event_time_offset "
                          << "'" << event_time_offset_str << "' !"
                          << std::endl;
                clear();
                return result::failure;
            }
            // Convert seconds to milliseconds.
            event_time_offset_ms = static_cast<milliseconds>(seconds * 1000.0f);
        }
        // hour:minute:seconds format.
        else
        {
            if (convert_hms_to_milliseconds(event_time_offset_str, event_time_offset_ms))
            {
                ERROR_LOG << file_path << "(" << line_number << "): "
                          << "Parser Error: failed to parse event_time_offset '"
                          << event_time_offset_str << "'!"
                          << std::endl;
               clear();
               return result::failure;
            }
        }

        // Parse channel...
        size_t dot_pos = camera_channel_str.find('.');
        if (dot_pos == std::string::npos ||
            dot_pos == 0 ||
            dot_pos == camera_channel_str.length() - 1)
        {
            ERROR_LOG << file_path << "(" << line_number << "): Parse Error: "
                      << ": Column 3 ('"
                      << camera_channel_str
                      << "') must be in 'camera_id.channel_name' format."
                      << std::endl;
            clear();
            return result::failure;
        }
        std::string camera_id = camera_channel_str.substr(0, dot_pos);
        std::string channel_name = camera_channel_str.substr(dot_pos + 1);

        _cam_ids.insert(camera_id);

        bool validation_ok = false;
        Channel channel;
        if (channel_name == "shutter_speed")
        {
            validation_ok = _is_valid_shutter_speed(channel_value_str);
            channel = Channel::shutter_speed;
        }
        else if (channel_name == "fstop")
        {
            validation_ok = _is_valid_fstop(channel_value_str);
            channel = Channel::fstop;
        }
        else if (channel_name == "iso")
        {
            validation_ok = true;
            channel = Channel::iso;
        }
        else if (channel_name == "quality")
        {
            validation_ok = true;
            channel = Channel::quality;
        }
        else if (channel_name == "fps")
        {
            validation_ok = _is_valid_fps_value(channel_value_str);
            channel = Channel::fps;
        }
        else if (channel_name == "trigger")
        {
            validation_ok = _is_valid_trigger_value(channel_value_str);
            channel = Channel::trigger;
        }
        else
        {
            ERROR_LOG << file_path << "(" << line_number << "): Parse Error: "
                      << ": Invalid channel name '"
                      << channel_name
                      << "'. Allowed: shutter_speed, fstop, iso, quality, fps, trigger."
                      << std::endl;
            clear();
            return result::failure;
        }

        if (!validation_ok)
        {
            ERROR_LOG << file_path << "(" << line_number << "): Parse Error: "
                      << ": Invalid value '"
                      << channel_value_str
                      << "' for channel '"
                      << channel_name
                      << "'."
                      << std::endl;
            clear();
            return result::failure;
        }

        _sequence.emplace_back(
            event_id_str,
            event_time_offset_ms,
            camera_id,
            channel,
            channel_value_str
        );
    }

    return result::success;
}

const std::vector<Event> &
CameraSequenceFileReader::
get_events() const
{
    return _sequence;
}

const std::set<CamId> &
CameraSequenceFileReader::
get_camera_ids() const
{
    return _cam_ids;
}

void
CameraSequenceFileReader::
clear()
{
    _sequence.clear();
    _cam_ids.clear();
}

} /* namespace */
