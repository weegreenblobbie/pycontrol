#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

#include <common/io.h>
#include <common/str_utils.h>
#include <common/UdpSocket.h>


namespace pycontrol
{


AutoFd::
~AutoFd()
{
    if(_fd >= 0) close(_fd);
}


result
UdpSocket::
init(const std::string & ipv4, const std::uint16_t & port)
{
    ABORT_IF(_socket_fd >= 0, "socket already initialized", result::failure);

    // 1. Create a UDP socket.
    // AF_INET: IPv4 Internet protocols
    // SOCK_DGRAM: Datagram (UDP) socket
    // 0: Default protocol for the given socket type (IPPROTO_UDP for SOCK_DGRAM)
    _socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    ABORT_IF(_socket_fd < 0, "Failed to open socket", result::failure);

    // 2. Prepare the server address structure.
    _sockaddr = std::make_shared<sockaddr_in>();
    ABORT_IF_NOT(_sockaddr, "make_shared<sockaddr_in>() failed", result::failure);

    ::memset(_sockaddr.get(), 0, sizeof(sockaddr_in));

    auto addr = _sockaddr.get();
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = ::inet_addr(ipv4.c_str());
    ABORT_IF(addr->sin_addr.s_addr < 0, "inet_addr() failed", result::failure);
    addr->sin_port = ::htons(port);

    return result::success;
}


result
UdpSocket::
bind()
{
    ABORT_IF(_bound, "UdpSocket already bound!", result::failure);

    // 3. Bind the socket to the server address.
    const int res = ::bind(_socket_fd, (const struct sockaddr *)_sockaddr.get(), sizeof(sockaddr_in));
    if (res == -1)
    {
        ::close(_socket_fd);
        ERROR_LOG << "Error: Could not bind to socket!";
        return result::failure;
    }

    _bound = true;

    return result::success;
}


result
UdpSocket::
send(const std::string & msg)
{
    std::uint16_t num_bytes = ::strlen(msg.c_str());
    ABORT_IF(num_bytes == 0, "send(), refusing to send 0 bytes", result::failure);

    const auto res = ::sendto(
        _socket_fd,
        msg.c_str(),
        num_bytes,
        0 /* flags */,
        reinterpret_cast<sockaddr *>(_sockaddr.get()),
        sizeof(sockaddr_in)
    );

    ABORT_IF(
        res < 0,
        "sendto() failed, errno: " << strerror(errno),
        result::failure
    );

    return result::success;
}


result
UdpSocket::
read(std::string & msg)
{
    ABORT_IF_NOT(_bound, "Must call bind() first!", result::failure);

    msg = "";

    // Try to peek at how many bytes are avialable for reading on the socket.

    int bytes_available = 0;
    const auto res = ::ioctl(_socket_fd, FIONREAD, &bytes_available);
    if (res == -1 or bytes_available <= 0)
    {
        // No message avialable.
        return result::success;
    }

    // Okay, a message is present.
    const std::size_t MAX_BUFFER_SIZE = 1024;
    auto buffer = std::vector<char>(MAX_BUFFER_SIZE, 0);

    struct ::sockaddr_in client_addr;
    ::socklen_t sizeof_sockaddr_in = sizeof(::sockaddr_in);

    const auto bytes_read = ::recvfrom(
        _socket_fd,
        buffer.data(),
        buffer.size(),
        0 /* flags */,
        (struct sockaddr *)&client_addr,
        &sizeof_sockaddr_in
    );

    ABORT_IF(bytes_read < 0, "::recvfrom() failed", result::failure);

    if (bytes_read == 0)
    {
        // Got an empty packet.
        return result::success;
    }

    // We got a packet, construct a std::string from it.
    msg = std::string(buffer.data(), bytes_read);
    return result::success;
}



}