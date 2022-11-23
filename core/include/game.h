//
// Created by josh on 11/10/22.
//

#pragma once
#include "asset.h"
#include <entt/entt.hpp>

namespace df {

class Game {
public:
    /**
     * Initialize the game
     * @param argc number of args from the command line
     * @param argv program arguments array
     */
    Game(int argc, char** argv);

    virtual ~Game();
    DF_NO_MOVE_COPY(Game);

    /**
     * Start the game
     */
    void run();

    /**
     * Stops the game
     */
    void stop() noexcept { running = false; }

    AssetRegistry assetRegistry;

protected:
    /**
     * The program's main loop,
     * Will be called once every frame
     * @param deltaSeconds number of seconds since the last frame
     */
    virtual void mainLoop(double deltaSeconds) = 0;

    entt::registry registry;
    bool running = true;
};

}   // namespace df