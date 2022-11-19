//
// Created by josh on 11/13/22.
//

#pragma once
#include "vertex.h"
#include "model.h"
#include <SDL_video.h>
#include "camera.h"
#include "transform.h"

namespace df {

class RenderContext {
public:

    RenderContext();
    void shutdown() noexcept;
    ~RenderContext() noexcept { shutdown(); }
    DF_NO_MOVE_COPY(RenderContext);

    void addModel(Model* model, const Transform& transform);
    void drawFrame();

private:
    Camera camera;
    class Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
    HashMap<Model*, std::vector<glm::mat4>> models;
};

}   // namespace df