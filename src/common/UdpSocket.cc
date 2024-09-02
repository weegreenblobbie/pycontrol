#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
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
init(const std::string & ipv4, const std::uint64_t & port)
{
    ABORT_IF(_socket_fd >= 0, "socket already initialized", result::failure);

    // AF_INET = IPv4 Internet protocols.
    // SOCK_DGRAM = UDP datagrams.
    _socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    ABORT_IF(_socket_fd < 0, "Failed to open socket", result::failure);

    _sockaddr = std::make_shared<sockaddr_in>();
    ABORT_IF_NOT(_sockaddr, "make_shared<sockaddr_in>() failed", result::failure);

    memset(_sockaddr.get(), 0, sizeof(sockaddr_in));

    auto addr = _sockaddr.get();
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ipv4.c_str());
    ABORT_IF(addr->sin_addr.s_addr < 0, "inet_addr() failed", result::failure);
    addr->sin_port = htons(port);

    return result::success;
}


result
UdpSocket::
send(const std::string & msg, const std::uint16_t nbytes)
{
    std::uint16_t num_bytes = nbytes;
    if (num_bytes == 0)
    {
        num_bytes = strlen(msg.c_str());
    }
    ABORT_IF(num_bytes == 0, "send(), refusing to send 0 bytes", result::failure);

    auto res = sendto(
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


}