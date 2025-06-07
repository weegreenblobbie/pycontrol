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

/*
class AutoFd
{
public:
    explicit AutoFd(int fd) : _fd{fd} {}
    ~AutoFd();
    int get() const { return _fd; }

    operator bool() const { return _fd >= 0; }
    operator int() const { return _fd; }

private:
    int _fd;
};
*/


class UdpSocket
{
public:

    result init(
        const std::string & ipv4,
        const std::uint16_t & port);

    result send(const std::string & msg);
    result read(std::string & msg);
    result bind();

    UdpSocket() {}

private:

    explicit UdpSocket(const UdpSocket & copy) = delete;
    UdpSocket & operator=(const UdpSocket & rhs) = delete;

    // TODO: use AutoFd instead of int?
    int _socket_fd { -1 };
    socketaddr_ptr _sockaddr {nullptr};
    bool _bound {false};
};


}
