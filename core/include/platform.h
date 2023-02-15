//
// Created by josh on 2/14/23.
//

#pragma once

namespace df {

class Process {};

void initPlatform();
void shutdownPlatform();

namespace net {
    class Socket {
    public:
        enum class Type {
            UDP,
            TCP,
        };
        Socket();
        Socket(UShort port, Type type = Type::TCP, bool blocking = false);
        ~Socket() noexcept { close(); };
        void setBlocking(bool blocking) const;
        void close() noexcept;
        [[nodiscard]] bool isOpen() const;
        operator bool() const { return isOpen(); }

        Socket(const Socket& other) = delete;
        Socket(Socket&& other) noexcept
        {
            if (this != &other) {
                close();
                handle = other.handle;
                port = other.port;
            }
        }
        Socket& operator=(const Socket& other) = delete;
        Socket& operator=(Socket&& other) noexcept
        {
            if (this != &other) {
                close();
                handle = other.handle;
                port = other.port;
            }
            return *this;
        }

    private:
#ifdef _WIN32
        UIntPtr handle;
#else
        int handle;
#endif
        UShort port;
    };

}   // namespace net

}   // namespace df