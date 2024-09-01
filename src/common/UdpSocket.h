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


class UdpSocket
{
public:

    result init(
        const std::string & ipv4,
        const std::uint64_t & port);

    result send(const std::string & msg);

private:

//~    AutoFd         _socket_fd {-1};
    int _socket_fd { -1 };
    socketaddr_ptr _sockaddr {nullptr};
};


}
