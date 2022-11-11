//
// Created by josh on 11/10/22.
//

#include "game.h"
#include <physfs.h>
#include <ctime>

#ifndef ASSET_PATH
    #define ASSET_PATH "../assets"
#endif

namespace df {
Game::Game(int argc, char** argv)
{
    int err = PHYSFS_init(argc > 0 ? *argv : nullptr);
    if (err != 0)
        err = PHYSFS_setSaneConfig("org", APP_NAME, "zip", false, false);
    if (err != 0)
        err = PHYSFS_mount(ASSET_PATH, "assets", false);
    if(err == 0)
        crash("PhysFS initialization failed: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    spdlog::info("PhysFS initialized");
}

Game::~Game()
{
    PHYSFS_deinit();
}

void Game::run()
{
    clock_t lastTime = std::clock();
    spdlog::info("Startup finished in {} seconds", (float)lastTime / CLOCKS_PER_SEC);
    while (running) {
        clock_t now = std::clock();
        double delta = (double)(now - lastTime) / CLOCKS_PER_SEC;
        lastTime = now;
        mainLoop(delta);
    }
}
}   // namespace df