#pragma once

#include <common/types.h>
#include <camera_control/CameraSequenceFileReader.h>

namespace pycontrol
{

class CameraSequence
{
public:
    result load(const std::string & camera_id, const std::vector<Event> & sequence);

    const Event & front() const;
    void pop();
    std::size_t pos() const;
    void reset() {_idx = 0;}
    bool empty() const;
    std::size_t size() const;

    const Event & peek(const std::size_t & offset) const { return _sequence[_idx + offset]; }

private:
    std::vector<Event> _sequence {};
    std::size_t _idx {0};
};

} /* namespace */
