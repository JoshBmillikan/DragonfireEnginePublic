//
// Created by josh on 11/13/22.
//

#pragma once
#include "camera.h"
#include "model.h"
#include "transform.h"
#include "vertex.h"
#include <SDL_video.h>

namespace df {

class RenderContext {
public:
    RenderContext();
    void shutdown() noexcept;
    ~RenderContext() noexcept { shutdown(); }
    DF_NO_MOVE_COPY(RenderContext);

    void addModel(Model* model, const Transform& transform) { models[model].push_back(transform); }

    void drawFrame();
    void waitForLastFrame();

    void loadTextures(const char* path);
    void loadMaterials(const char* path);
    void loadModels(const char* path);

private:
    Camera camera{};
    class Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
    HashMap<Model*, std::vector<glm::mat4>> models;
};

}   // namespace df