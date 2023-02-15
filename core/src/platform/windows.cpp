//
// Created by josh on 2/14/23.
//
#include <Windows.h>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include "platform.h"

namespace df {
class WindowsProcess : public Process {};

static void throwWSAError(DWORD err = WSAGetLastError())
{
    LPTSTR buffer;
    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &buffer,
            0,
            nullptr
    );
    throw std::runtime_error(buffer);
}

namespace net {

    Socket::Socket(UShort port, Type type, bool blocking) : handle(INVALID_SOCKET), port(port)
    {
        if (port < 1024)
            spdlog::warn("Attempted to bind socket to port {}, ports < 1024 are reserved by the system", port);

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
        if (handle == INVALID_SOCKET)
            throwWSAError();
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(handle, reinterpret_cast<const sockaddr*>(&address), sizeof(sockaddr_in)) < 0) {
            close();
            throwWSAError();
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
        DWORD nonBlocking = blocking ? 0 : 1;
        if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0)
            throwWSAError();
    }

    bool Socket::isOpen() const
    {
        return handle != INVALID_SOCKET;
    }

    void Socket::close() noexcept
    {
        if (handle != INVALID_SOCKET) {
            if (closesocket(handle) != 0)
                spdlog::error("Error closing socket");
            handle = INVALID_SOCKET;
        }
    }

    Socket::Socket() : handle(INVALID_SOCKET), port(UINT16_MAX)
    {
    }
}   // namespace net

static void initSockets()
{
    WSADATA data;
    int err = WSAStartup(MAKEWORD(2, 2), &data);
    if (err != 0) {
        throwWSAError(err);
    }
}

void initPlatform()
{
    initSockets();
}

void shutdownPlatform()
{
    WSACleanup();
}
}   // namespace df