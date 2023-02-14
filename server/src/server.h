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
protected:
    void update(double deltaSeconds) override;
};

}   // namespace df