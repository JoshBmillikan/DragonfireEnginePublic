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

    /** @brief enqueues a model and transform to be drawn with the next frame
     *
     * Does not actually do any rendering, call drawFrame to draw all queued models
     * @param model
     * @param transform
     */
    void enqueueModel(Model model, const Transform& transform) { models[model].push_back(transform); }

    /** @brief renders all queued models and draws the next frame
     *
     *  call enqueueModel(model, transform) to queue models to render
     */
    void drawFrame();

    /** @brief stops the rendering engine.
     *
     * Joins all rendering threads and ensures no gpu resources remain in use,
     * so models, textures, etc. can safely be destroyed
     */
    void stopRendering();

    // Destroys all data related to the rendering context
    void destroy() noexcept;

    void loadTextures(const char* path);
    void loadMaterials(const char* path);
    void loadModels(const char* path);
    [[nodiscard]] SDL_Window* getWindow() const { return window; }
    ~RenderContext() noexcept { destroy(); }
    DF_NO_MOVE_COPY(RenderContext);

private:
    Camera camera{};
    class Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
    HashMap<Model, std::vector<glm::mat4>> models;
};

}   // namespace df