//
// Created by josh on 2/14/23.
//
#include "platform.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace df {

namespace net {
    Socket::Socket(UShort port, Type type, bool blocking) : handle(-1), port(port)
    {
        int protocol, socketType;
        if (type == Type::TCP) {
            socketType = IPPROTO_TCP;
            protocol = SOCK_STREAM;
        }
        else if (type == Type::UDP) {
            socketType = IPPROTO_UDP;
            protocol = SOCK_DGRAM;
        }
        handle = socket(AF_INET, socketType, protocol);
        if (handle < 0)
            throw std::runtime_error(strerror(errno));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(handle, reinterpret_cast<const sockaddr*>(&address), sizeof(sockaddr_in)) < 0) {
            close();
            throw std::runtime_error(strerror(errno));
        }

        try {
            setBlocking(blocking);
        }
        catch (...) {
            close();
            throw;
        }
    }

    void Socket::setBlocking(bool blocking) const
    {
        int nonBlocking = blocking ? 0 : 1;
        if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) < 0)
            throw std::runtime_error(strerror(errno));
    }

    bool Socket::isOpen() const
    {
        return handle >= 0;
    }

    void Socket::close() noexcept
    {
        if (handle >= 0) {
            if (::close(handle) < 0)
                spdlog::error("Error closing socket: {}", strerror(errno));
            handle = -1;
        }
    }

    Socket::Socket() : handle(-1), port(UINT16_MAX)
    {
    }

}   // namespace net

void initPlatform()
{
}
void shutdownPlatform()
{
}
}   // namespace df