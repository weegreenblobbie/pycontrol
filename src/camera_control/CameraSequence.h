#pragma once

#include <common/types.h>
#include <camera_control/CameraSequenceFileReader.h>

#include <iterator>

namespace pycontrol
{

using event_vec = std::vector<Event>;

class CameraSequence
{
private:
    event_vec _sequence {};
    event_vec::const_iterator _pos {};

public:
    result load(const std::string & camera_id, const std::vector<Event> & sequence);

    const Event & front() const { return *_pos; }
    void pop() { ++_pos; }
    void reset() {_pos = _sequence.begin();}
    bool empty() {return _pos >= _sequence.end();}
    std::size_t size() const { return std::distance(_pos, _sequence.end()); }
    std::size_t pos() const {return std::distance(_sequence.begin(), _pos) + 1; }

    event_vec::const_iterator begin() const { return _pos; }
    event_vec::const_iterator end() const { return _sequence.end(); }

};

} /* namespace */
