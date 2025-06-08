#pragma once

#include <string>
#include <vector>
#include <set>

#include <common/types.h>
#include <camera_control/Event.h>


namespace pycontrol
{

class CameraSequenceFileReader
{
public:
    result read_file(const std::string & file_path);
    const std::vector<Event> & get_events() const;
    const std::set<CamId> & get_camera_ids() const;
    void clear();

private:

    std::string _strip(const std::string& str) const;

    bool _is_ignorable_line(const std::string& line) const;
    bool _is_valid_shutter_speed(const std::string& value) const;
    bool _is_valid_fstop(const std::string& value) const;
    bool _is_valid_fps_value(const std::string& value) const;
    bool _is_valid_trigger_value(const std::string& value) const;

    std::vector<Event> _sequence {};
    std::set<CamId> _cam_ids {};
};



} /* namespace */
