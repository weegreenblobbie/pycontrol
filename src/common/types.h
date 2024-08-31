#pragma once

namespace pycontrol
{

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