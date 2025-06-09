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
    unsigned int pos() const;
    void reset() {_idx = 0;}
    bool empty() const;
    std::size_t size() const;

private:
    std::vector<Event> _sequence {};
    unsigned int _idx {0};
};

} /* namespace */
