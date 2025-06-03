#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <iterator>

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
    "1.0",
    "1.3",
    "1.6",
    "2",
    "2.0",
    "2.5",
    "3",
    "3.0",
    "4",
    "4.0",
    "5",
    "5.0",
    "6",
    "6.0",
    "8",
    "8.0",
    "10",
    "10.0",
    "13",
    "13.0",
    "15",
    "15.0",
    "20",
    "20.0",
    "25",
    "25.0",
    "30",
    "30.0",
    "60",
    "60.0",
    "90",
    "90.0",
    "120",   // 2 minutes
    "120.0",
    "180",   // 3 minutes
    "180.0",
    "240",   // 4 minutes
    "240.0",
    "300",   // 5 minutes
    "300.0",
    "480",   // 8 minutes
    "480.0",
    "600",   // 10 minutes
    "600.0",
    "720",   // 12 minutes
    "720.0",
    "900",   // 15 minutes
    "900.0",
};

const std::set<std::string> _valid_fstops = {
    "1.2",
    "1.4",
    "2.0",
    "2",
    "2.2",
    "2.8",
    "3.2",
    "3.5",
    "4.0",
    "4",
    "4.5",
    "5.0",
    "5",
    "5.6",
    "6.3",
    "7.1",
    "8",
    "8.0",
    "9",
    "9.0",
    "10",
    "10.0",
    "11",
    "11.0",
    "13",
    "13.0",
    "14",
    "14.0",
    "16",
    "16.0",
    "18",
    "18.0",
    "20",
    "20.0",
    "22",
    "22.0",
    "25",
    "25.0",
    "29",
    "29.0",
    "32",
    "32.0",
    "36",
    "36.0",
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
    int line_number = 0;
    while (std::getline(file, line))
    {
        line_number++;
        if (_is_ignorable_line(line))
        {
            continue;
        }

        std::istringstream iss(line);
        std::string event_id_str, event_time_offset_str, camera_channel_str, channel_value_str;
        float event_time_offset = 0.0f;

        if (!(iss >> event_id_str
                  >> event_time_offset_str
                  >> camera_channel_str
                  >> channel_value_str))
        {
            ERROR_LOG << file_path << "(" << line_number << "): "
                      << "Parse Error: Incorrect number or format of columns."
                      << std::endl;
            clear();
            return result::failure;
        }

        std::string end_of_line_content;

        if (iss >> end_of_line_content)
        {
            if (_strip(end_of_line_content)[0] != '#')
            {
                ERROR_LOG << file_path << "(" << line_number << "): "
                      << "Parse Error: More than 4 columns detected!"
                      << std::endl;
                clear();
                return result::failure;
            }
        }

        // Parse event_time_offset_str, valid formats:
        //    floats => 1.0  -10.0  1e2
        //    hour:minute:seconds =>   1:15.2   1:23:43.123    -1:15.5

        // If no ':' present, assume it's just a float.
        if (event_time_offset_str.find(':') == std::string::npos)
        {
            if (as_type<float>(event_time_offset_str, event_time_offset))
            {
                ERROR_LOG << file_path << "(" << line_number << "): "
                          << "Parser Error: failed to parse event_time_offset "
                          << "'" << event_time_offset_str << "' !"
                          << std::endl;
                clear();
                return result::failure;
            }
        }
        // hour:minute:seconds format.
        else
        {
            if (convert_hms_to_seconds(event_time_offset_str, event_time_offset))
            {
                ERROR_LOG << file_path << "(" << line_number << "): "
                          << "Parser Error: failed to parse event_time_offset '"
                          << event_time_offset_str << "'!"
                          << std::endl;
               clear();
               return result::failure;
            }
        }

        // Parse channel.
        //
        // camera_id '.' channel_name
        //
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

        bool validation_ok = false;
        if (channel_name == "shutter_speed")
        {
            validation_ok = _is_valid_shutter_speed(channel_value_str);
        }
        else if (channel_name == "fstop")
        {
            validation_ok = _is_valid_fstop(channel_value_str);
        }
        else if (channel_name == "iso")
        {
            validation_ok = true;
        }
        else if (channel_name == "quality")
        {
            validation_ok = true;
        }
        else if (channel_name == "fps")
        {
            validation_ok = _is_valid_fps_value(channel_value_str);
        }
        else if (channel_name == "trigger")
        {
            validation_ok = _is_valid_trigger_value(channel_value_str);
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

        _sequence_entries.emplace_back(
            event_id_str,
            event_time_offset,
            camera_id,
            channel_name,
            channel_value_str
        );
    }

    return result::success;
}

const std::vector<CameraSequenceEntry> &
CameraSequenceFileReader::
get_entries() const
{
    return _sequence_entries;
}

void
CameraSequenceFileReader::
clear()
{
    _sequence_entries.clear();
}

} /* namespace */