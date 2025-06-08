#pragma once

#include <camera_control/CameraSequenceFileReader.h>

namespace pycontrol
{

class CameraEventSequence
{
public:
    result load(const std::string & camera_name, const std::vector<CameraSequenceEntry> & sequence);

    const CameraSequenceEntry & front() const;
    void pop();
    void reset();
    bool empty() const;
    std::size_t size() const;

private:
    std::vector<CameraSequenceEntry> _sequence;
    int _pos {0};
};

} /* namespace */
