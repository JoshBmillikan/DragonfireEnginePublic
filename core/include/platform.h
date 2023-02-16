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
    struct Address;
    class Socket {
    public:
        enum class Type {
            UDP,
            TCP,
        };
        Socket();
        /**
         * @brief Creates and opens a new socket connection
         * @param port port number of the socket
         * @param type type of the socket, either tcp or udp
         * @param blocking should the socket be opened in blocking or non-blocking mode (defaults to non-blocking)
         */
        Socket(UShort port, Type type = Type::TCP, bool blocking = false);
        ~Socket() noexcept { close(); };
        /**
         * @brief Send data from this socket
         * @param address The address to send to
         * @param data data buffer to read from
         * @param size size of the data, in bytes, to send
         */
        void send(const Address& address, void* data, Size size) const;
        /**
         * @brief Receives data from this socket
         * @param [out] sender address of the sender
         * @param [out] data pointer to where received data will be written
         * @param [in] bufferSize maximum size of the buffer, in bytes. if the packet is larger than this it will be discarded
         * @return number of bytes of data received
         */
        Size receive(Address& sender, void* data, Size bufferSize) const;
        /**
         * @brief Set the socket as blocking or non-blocking
         * @param blocking true for blocking, false for non-blocking
         */
        void setBlocking(bool blocking) const;
        /// Close the socket
        void close() noexcept;
        /// returns true if the socket is still open
        [[nodiscard]] bool isOpen() const;
        void listen(int queued = 1) const;
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