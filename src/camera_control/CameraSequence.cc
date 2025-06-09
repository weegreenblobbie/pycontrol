#include <common/io.h>
#include <camera_control/CameraSequence.h>

namespace pycontrol
{

result
CameraSequence::
load(const std::string & camera_id, const std::vector<Event> & sequence)
{
    _sequence.clear();
    reset();

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

    return result::success;
}

unsigned int
CameraSequence::
pos() const
{
    if (empty())
    {
        return _idx;
    }
    return _idx + 1;
}

const Event &
CameraSequence::
front() const
{
    if (_idx < _sequence.size())
    {
        return _sequence[_idx];
    }

    ERROR_LOG << "Out of bounds idx: " << _idx << " size: " << _sequence.size()
              << std::endl;

    static const auto no_event = Event("N/A", 0.0f, "N/A", Channel::trigger, "N/A");
    return no_event;
}

void
CameraSequence::
pop()
{
    if (_idx + 1 < _sequence.size())
    {
        ++_idx;
    }
}

bool
CameraSequence::
empty() const
{
    return _sequence.empty() or _idx == _sequence.size();
}

std::size_t
CameraSequence::
size() const
{
    return _sequence.size();
}

} /* namespace */
