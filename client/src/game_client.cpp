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
    window = createWindow();
    renderer = Renderer::getOrInit(window); // todo
}

GameClient::~GameClient()
{
    renderer->shutdown();
    SDL_DestroyWindow(window);
}
}// namespace df