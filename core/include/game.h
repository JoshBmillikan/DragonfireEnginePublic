//
// Created by josh on 11/10/22.
//

#pragma once
#include "asset.h"
#include "physics.h"
#include "world/world.h"
#include <entt/entt.hpp>
#include <sol/sol.hpp>

namespace df {

class BaseGame {
public:
    /**
     * Initialize the game
     * @param argc number of args from the command line
     * @param argv program arguments array
     */
    BaseGame(int argc, char** argv);

    virtual ~BaseGame();
    DF_NO_MOVE_COPY(BaseGame);

    /**
     * Start the game
     */
    void run();

    /**
     * Stops the game
     */
    void stop() noexcept { running = false; }

    AssetRegistry assetRegistry;
    World* getWorld() noexcept { return world.get(); }
    static BaseGame& get() noexcept { return *game; }

protected:
    /**
     * The program's main loop,
     * Will be called once every frame
     * @param deltaSeconds number of seconds since the last frame
     */
    virtual void update(double deltaSeconds) = 0;

    bool running = true;
    std::unique_ptr<World> world;
    sol::state lua;

private:
    static BaseGame* game;
};

}   // namespace df