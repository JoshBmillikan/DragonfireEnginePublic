//
// Created by josh on 5/17/23.
//

#include "renderer.h"
#include <config.h>

namespace dragonfire {

void Renderer::createWindow(SDL_WindowFlags sdlFlags)
{
    int width = int(Config::INSTANCE.get<Int64>("graphics.window.resolution.0"));
    int height = int(Config::INSTANCE.get<Int64>("graphics.window.resolution.1"));
    UInt32 flags = sdlFlags;
    Int64 windowMode = Config::INSTANCE.get<Int64>("graphics.window.mode");
    switch (windowMode) {
        case 0: flags |= SDL_WINDOW_RESIZABLE; break;
        case 2:
            flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP
                     | SDL_WINDOW_MOUSE_CAPTURE;
            break;
        default: logger->error("Invalid window mode \"{}\", valid are 0,1,2", windowMode);
        case 1: flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MOUSE_CAPTURE;
    }
    window = SDL_CreateWindow(
            APP_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags
    );
    if (window == nullptr)
        crash("SDL failed to create window: {}", SDL_GetError());
    logger->info("Created window of resolution {}x{}", width, height);
}
}   // namespace dragonfire