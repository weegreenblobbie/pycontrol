#pragma once

#include <string>
#include <string_view>

#include <common/types.h>


namespace pycontrol
{

enum class Channel: unsigned int
{
    burst_number,
    capture_mode,
    fps,
    fstop,
    iso,
    mode,
    quality,
    shooting_speed,
    shutter_speed,
    trigger,

    null
};

inline
std::string_view
to_string(Channel ch)
{
    switch(ch)
    {
        case Channel::burst_number:   return "burst_number";
        case Channel::capture_mode:   return "capture_mode";
        case Channel::fps:            return "fps";
        case Channel::fstop:          return "fstop";
        case Channel::iso:            return "iso";
        case Channel::mode:           return "mode";
        case Channel::quality:        return "quality";
        case Channel::shooting_speed: return "shooting_speed";
        case Channel::shutter_speed:  return "shutter_speed";
        case Channel::trigger:        return "trigger";
        case Channel::null:           return "null";
    }
    return "to_string(Channel) error";
}


struct Event
{
    std::string  event_id;
    milliseconds event_time_offset_ms;
    std::string  camera_id;
    Channel      channel;
    std::string  channel_value;

    // Convenient constructor to enable std::vector::emplace_back().
    Event(
        std::string          event_id,
        const milliseconds & event_time_offset_ms,
        std::string          camera_id,
        const Channel &      channel,
        std::string          channel_value
    )
    : event_id(std::move(event_id)),
      event_time_offset_ms(event_time_offset_ms),
      camera_id(std::move(camera_id)),
      channel(channel),
      channel_value(std::move(channel_value))
    {}
};


} /* namespace */