#include <camera_control/WallClock.h>

#include <chrono>
#include <ctime>
#include <iomanip>


namespace pycontrol
{

milliseconds
WallClock::now()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}


std::string
format_iso8601_utc(pycontrol::milliseconds ms_since_epoch)
{
    // Create a duration and time_point from the incoming integer.
    // We trust that this integer is the "correct" POSIX time in milliseconds.
    auto const duration = std::chrono::milliseconds(ms_since_epoch);
    auto const time_point = std::chrono::time_point<std::chrono::system_clock>(duration);

    // Separate into seconds and the fractional millisecond part
    auto const seconds_since_epoch = std::chrono::time_point_cast<std::chrono::seconds>(time_point);
    auto const ms_part = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - seconds_since_epoch);

    std::time_t time_t_seconds = std::chrono::system_clock::to_time_t(seconds_since_epoch);

    std::stringstream ss;
    // std::gmtime correctly interprets a POSIX time_t as UTC.
    ss << std::put_time(std::gmtime(&time_t_seconds), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setw(3) << std::setfill('0') << ms_part.count() << 'Z';

    return ss.str();
}


} /* namespace pycontrol */
