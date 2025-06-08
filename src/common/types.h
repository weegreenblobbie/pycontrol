#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace pycontrol
{

using EventId = std::string;
using Serial = std::string;
using CamId = std::string;
using UsbPort = std::string;
using milliseconds = std::int64_t;

constexpr milliseconds MAX_TIME = std::numeric_limits<milliseconds>::max();


struct result
{
    static const bool failure = true;
    static const bool success = false;

    result(const bool b = false) : _value(b) {}

    operator bool() const
    {
        return _value;
    }

private:
    bool _value {false};

};


}