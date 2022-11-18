//
// Created by josh on 11/9/22.
//

#include "game_client.h"

namespace df {

GameClient::GameClient(int argc, char** argv) : Game(argc, argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        crash("Failed to initialize SDL: {}", SDL_GetError());
    SDL_version version;
    SDL_GetVersion(&version);
    spdlog::info("SDL version {}.{}.{} loaded", version.major, version.minor, version.patch);
    renderContext = new RenderContext();
}

GameClient::~GameClient()
{
    assetRegistry.destroy();
    delete renderContext;
    SDL_Quit();
}

void GameClient::mainLoop(double deltaSeconds)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym != SDLK_ESCAPE)
                    break;
            case SDL_QUIT:
                spdlog::info("Quit requested");
                stop();
                break;
        }
    }
}
}   // namespace df