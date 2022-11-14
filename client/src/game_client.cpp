//
// Created by josh on 11/9/22.
//

#include "game_client.h"
#include "renderer.h"

namespace df {
static SDL_Window* createWindow() {
    int flags = Renderer::SDL_WINDOW_FLAGS;

    SDL_Window* window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, flags);
    if (window)
        return window;
    crash("SDL failed to create window: {}", SDL_GetError());
}

GameClient::GameClient(int argc, char** argv) : Game(argc, argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        crash("Failed to initialize SDL: {}", SDL_GetError());
    SDL_version version;
    SDL_GetVersion(&version);
    spdlog::info("SDL version {}.{}.{} loaded", version.major, version.minor, version.patch);
    window = createWindow();
    renderer = Renderer::getOrInit(window); // todo
}

GameClient::~GameClient()
{
    renderer->shutdown();
    SDL_DestroyWindow(window);
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
}// namespace df