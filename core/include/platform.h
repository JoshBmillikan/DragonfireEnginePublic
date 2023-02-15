//
// Created by josh on 2/14/23.
//

#pragma once

namespace df {
using ProcessHandle = std::unique_ptr<class Process>;
class Process {
public:
    static Process* spawn(const char* path, char** argv = nullptr);
    static ProcessHandle spawnHandle(const char* path, char** argv = nullptr)
    {
        return std::unique_ptr<Process>(spawn(path, argv));
    }
    virtual int stopProcess() = 0;
    virtual ~Process() noexcept = default;

protected:
    Process() = default;
};

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