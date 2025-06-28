#pragma once

#include <string>
#include <common/types.h>

namespace pycontrol
{
namespace interface
{


class UdpSocket
{
public:
    virtual ~UdpSocket() = default;

    virtual result recv(std::string & msg) = 0;
    virtual result send(const std::string & msg) = 0;
};


} /* namespace interface */
} /* namespace pycontrol */
