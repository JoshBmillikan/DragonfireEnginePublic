//
// Created by josh on 11/10/22.
//

#pragma once
#include "asset.h"

namespace df {

class Game {
public:
    Game(int argc, char** argv);
    virtual ~Game();
    DF_NO_MOVE_COPY(Game);
    void run();
    void stop() noexcept { running = false; }

    AssetRegistry assetRegistry;

protected:
    virtual void mainLoop(double deltaSeconds) = 0;
    bool running = true;
};

}   // namespace df