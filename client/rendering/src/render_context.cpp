//
// Created by josh on 11/13/22.
//

#include "render_context.h"
#include "config.h"
#include "material.h"
#include "render_asset_loaders.h"
#include "renderer.h"

namespace df {
static SDL_Window* createWindow()
{
    int flags = Renderer::SDL_WINDOW_FLAGS | SDL_WINDOW_MOUSE_CAPTURE;
    auto& cfg = Config::get().graphics;

    switch (cfg.windowMode) {
        case GraphicsSettings::WindowMode::borderless: flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP; break;
        case GraphicsSettings::WindowMode::fullscreen: flags |= SDL_WINDOW_FULLSCREEN; break;
        case GraphicsSettings::WindowMode::windowed: flags |= SDL_WINDOW_RESIZABLE; break;
    }

    SDL_Window* window = SDL_CreateWindow(
            APP_NAME,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            (int) cfg.resolution[0],
            (int) cfg.resolution[1],
            flags
    );
    if (window)
        return window;
    throw std::runtime_error(fmt::format("SDL failed to create window: {}", SDL_GetError()));
}

RenderContext::RenderContext()
{
    window = createWindow();
    renderer = new Renderer(window);
}

void RenderContext::shutdown() noexcept
{
    if (window) {
        delete renderer;
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void RenderContext::drawFrame()
{
    renderer->beginRendering(camera);
    for (auto& [model, matrices] : models)
        renderer->render(model, matrices);
    renderer->endRendering();
    for (auto& [model, matrices] : models)
        matrices.clear();
}

void RenderContext::loadTextures(const char* path)
{
    //todo
}

void RenderContext::loadMaterials(const char* path)
{
    MaterialLoader loader(renderer->getPipelineFactory(), renderer->getDevice(), renderer->getGlobalDescriptorSetLayout());
    auto& registry = AssetRegistry::getRegistry();
    registry.loadDir("assets/materials", loader);
}

void RenderContext::loadModels(const char* path)
{
    ObjLoader loader(renderer);
    auto& registry = AssetRegistry::getRegistry();
    registry.loadDir("assets/models", loader);
}

}   // namespace df