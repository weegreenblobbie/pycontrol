#pragma once

#include <common/types.h>

namespace pycontrol
{
namespace interface
{


class WallClock
{
public:
    virtual ~WallClock() = default;

    virtual milliseconds now() = 0;
};


} /* namespace interface */
} /* namespace pycontrol */
