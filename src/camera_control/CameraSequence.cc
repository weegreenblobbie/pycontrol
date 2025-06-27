#include <common/io.h>
#include <camera_control/CameraSequence.h>


namespace pycontrol
{

result
CameraSequence::
load(const std::string & camera_id, const std::vector<Event> & sequence)
{
    _sequence.clear();

    for (const auto & event : sequence)
    {
        if (event.camera_id == camera_id)
        {
            _sequence.push_back(event);
        }
    }

    ABORT_IF(
        _sequence.empty(),
        "No events found for '" << camera_id << "'!",
        result::failure
    );

    reset();

    return result::success;
}



//~static const Event null_event {
//~    "N/A",
//~    MAX_TIME,
//~    "N/A",
//~    Channel::null,
//~    "N/A"
//~};

//~const Event &
//~CameraSequence::
//~front() const
//~{
//~    if (_idx < _sequence.size())
//~    {
//~        return _sequence[_idx];
//~    }

//~    ERROR_LOG << "Out of bounds idx: " << _idx << " size: " << _sequence.size()
//~              << std::endl;

//~    return null_event;
//~}

//~void
//~CameraSequence::
//~pop()
//~{
//~    ++_pos;
//~}

//~bool
//~CameraSequence::
//~empty() const
//~{
//~    return _sequence.empty() or _idx >= _sequence.size();
//~}

//~std::size_t
//~CameraSequence::
//~size() const
//~{
//~    return _sequence.size();
//~}

//~const Event &
//~CameraSequence::
//~peek(const std::size_t & offset) const
//~{
//~    if (empty())
//~    {
//~        ERROR_LOG << "CameraSequences is empty!" << std::endl;
//~        return null_event;
//~    }

//~    if (_idx + offset >= size())
//~    {
//~        ERROR_LOG << "CameraSequences pos + offset out of bounds!" << std::endl;
//~        return null_event;
//~    }

//~    return _sequence[_idx + offset];
//~}


} /* namespace */
