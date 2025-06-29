#pragma once

#include <interface/WallClock.h>

namespace pycontrol
{

class WallClock : public interface::WallClock
{
public:

    milliseconds now() override;
};


std::string format_iso8601_utc(pycontrol::milliseconds ms_since_epoch);


} /* namespace pycontrol */
