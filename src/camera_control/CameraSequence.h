#pragma once

#include <string>
#include <vector>
#include <set>

namespace pycontrol
{

// Forward.
struct CameraSequenceEntry;

class CameraSequence
{
public:
    bool load_from_file(const std::string & file_path);
    const std::vector<CameraSequenceEntry> & get_entries() const;
    void clear();

private:

    std::string _strip(const std::string& str) const;

    bool _is_ignorable_line(const std::string& line) const;

    bool _is_valid_shutter_speed(const std::string& value) const;

    bool _is_valid_fstop(const std::string& value) const;

    bool _is_valid_fps_value(const std::string& value) const;

    std::vector<CameraSequenceEntry> _sequence_entries {};
};


struct CameraSequenceEntry
{
    std::string event_id;
    float event_time_offset_seconds;
    std::string camera_id;
    std::string channel_name;
    std::string channel_value;

    // Convient constructor to enable std::vector::emplace_back().
    CameraSequenceEntry(
        std::string event_id,
        const float & event_time_offset_seconds,
        std::string camera_id,
        std::string channel_name,
        std::string channel_value
    )
    : event_id(std::move(event_id)),
      event_time_offset_seconds(event_time_offset_seconds),
      camera_id(std::move(camera_id)),
      channel_name(std::move(channel_name)),
      channel_value(std::move(channel_value))
    {}
};


} /* Namespace */
