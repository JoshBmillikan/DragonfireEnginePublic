//
// Created by josh on 2/15/23.
//

#pragma once

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
     * @param a first byte
     * @param b second byte
     * @param c third byte
     * @param d fourth byte
     */
    Address(UShort port, UByte a, UByte b, UByte c, UByte d) : Address(port, (a << 24) | (b << 16) | (c << 8) | (d)) {}
    /**
     * @brief Creates a network address for the given port pointing to localhost
     * @param port port number
     */
    Address(UShort port) : Address(port, 127, 0, 0, 1) {}
    [[nodiscard]] UInt getAddress() const { return address; }
    [[nodiscard]] UShort getPort() const { return port; }
    void setPort(UShort newPort) { Address::port = newPort; }
    void setAddress(UInt addr) { address = addr; }
private:
    UInt address;
    UShort port;
};

}   // namespace df::net