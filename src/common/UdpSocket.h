#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <common/types.h>

// Forward.
struct sockaddr_in;

namespace pycontrol
{

using socketaddr_ptr = std::shared_ptr<sockaddr_in>;

class UdpSocket
{
public:

    UdpSocket():
        _port(0),
        _socket_fd(-1),
        _sockaddr(nullptr),
        _bound(false)
    {}

    result init(
        const std::string & ipv4,
        const std::uint16_t & port);

    result send(const std::string & msg);
    result read(std::string & msg);
    result bind();

private:

    explicit UdpSocket(const UdpSocket & copy) = delete;
    UdpSocket & operator=(const UdpSocket & rhs) = delete;

    unsigned int _port {0};
    int _socket_fd { -1 };
    socketaddr_ptr _sockaddr {nullptr};
    bool _bound {false};
};


}
