#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <interface/UdpSocket.h>

// Forward.
struct sockaddr_in;

namespace pycontrol
{

class UdpSocket : public interface::UdpSocket
{
public:

    UdpSocket() = default;

    result init(
        const std::string & ipv4,
        const std::uint16_t & port);

    result bind();

    result send(const std::string & msg) override;
    result recv(std::string & msg) override;

private:

    explicit UdpSocket(const UdpSocket & copy) = delete;
    UdpSocket & operator=(const UdpSocket & rhs) = delete;

    using socketaddr_ptr = std::shared_ptr<sockaddr_in>;

    unsigned int _port {0};
    int _socket_fd { -1 };
    socketaddr_ptr _sockaddr {nullptr};
    bool _bound {false};
};


} /* namespace pycontrol */
