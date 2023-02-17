//
// Created by josh on 2/13/23.
//
#include <game.h>

#pragma once

namespace df {

class Server : public BaseGame {
public:
    Server(int argc, char** argv);
    ~Server() override;
    DF_NO_MOVE_COPY(Server);
    struct Config {
        UShort port = 3333;
        UInt maxPlayerCount = 16;
    }config;

    void loadServerConfig();
protected:
    void update(double deltaSeconds) override;
};

}   // namespace df