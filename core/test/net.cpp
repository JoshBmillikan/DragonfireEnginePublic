//
// Created by josh on 2/15/23.
//

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <net.h>
#include <platform.h>

using namespace df;

TEST_CASE("UDP socket test")
{
    initPlatform();
    UInt port = 3000;
    net::Socket socket = net::Socket(port + 1, net::Socket::Type::UDP, true);
    REQUIRE(socket.isOpen());
    char data[] = "hello";
    Size len = strlen(data);
    net::Address address(port);
    char buffer[10];

    net::Socket receiver(port, net::Socket::Type::UDP, true);
    auto f = std::async(std::launch::async, [&] {
        net::Address sender;
        return receiver.receive(sender, buffer, 10);
    });

    REQUIRE_NOTHROW(socket.send(address, data, len));

    SECTION("Receiving data")
    {
        using namespace std::chrono_literals;
        REQUIRE(f.wait_for(5s) == std::future_status::ready);
        REQUIRE(f.get() == len);
        REQUIRE(strcmp(buffer, data) == 0);
    }
    socket.close();
    receiver.close();
    shutdownPlatform();
}