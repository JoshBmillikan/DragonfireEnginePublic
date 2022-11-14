//
// Created by josh on 11/13/22.
//

#pragma once
#include <SDL_video.h>

namespace df {

class RenderContext {
    class Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;

public:
    RenderContext();
    void shutdown() noexcept;
    ~RenderContext() noexcept { shutdown(); }
    DF_NO_MOVE_COPY(RenderContext);
};

}   // namespace df