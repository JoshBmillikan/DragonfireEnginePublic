//
// Created by josh on 2/15/23.
//

#pragma once
#include "platform.h"

namespace df::net {

class Address {
public:
    /**
     * @brief Creates a network address
     * @param port port number
     * @param address ip address
     */
    Address(UShort port, UInt address) : address(address), port(port) {}
    /**
     * @brief Creates a network address from a port and 4 single byte components
     * @param port port number
     * @param a first byte of the ip address
     * @param b second byte of the ip address
     * @param c third byte of the ip address
     * @param d fourth byte of the ip address
     */
    Address(UShort port, UByte a, UByte b, UByte c, UByte d) : Address(port, (a << 24) | (b << 16) | (c << 8) | (d)) {}
    /**
     * @brief Creates a network address for the given port pointing to localhost
     * @param port port number
     */
    Address(UShort port) : Address(port, 127, 0, 0, 1) {}
    Address() = default;
    [[nodiscard]] UInt getAddress() const { return address; }
    [[nodiscard]] UShort getPort() const { return port; }
    void setPort(UShort newPort) { Address::port = newPort; }
    void setAddress(UInt addr) { address = addr; }

private:
    UInt address = 0;
    UShort port = UINT16_MAX;
};

struct MessageHeader {
    std::endian endian;
    enum class MessageType {
        heartbeat = 0x687b
    }type;
    UInt sequenceNumber;
    UInt checksum;
};

class Connection {
public:
    static constexpr int BUFFER_SIZE = 512;

private:
    UByte msgBuf[BUFFER_SIZE];

    Socket socket;
    void receiveMessage();
};


}   // namespace df::net