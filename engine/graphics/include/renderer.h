//
// Created by josh on 5/17/23.
//

#pragma once
#include <SDL2/SDL_video.h>

namespace dragonfire {

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void init() = 0;
    virtual void shutdown() = 0;

protected:
    UInt64 frameCount = 0;
    std::shared_ptr<spdlog::logger> logger;
    SDL_Window* window = nullptr;

    void createWindow(SDL_WindowFlags sdlFlags);
};

extern Renderer* getRenderer();

}   // namespace dragonfire