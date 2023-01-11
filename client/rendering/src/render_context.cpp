//
// Created by josh on 11/13/22.
//

#include "render_context.h"
#include "config.h"
#include "file.h"
#include "material.h"
#include "render_asset_loaders.h"
#include "renderer.h"
#include <stb_image.h>

namespace df {

static Camera createCamera(SDL_Window* window)
{
    auto& cfg = Config::get().graphics;
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return Camera(cfg.fov, static_cast<UInt>(width), static_cast<UInt>(height));
}

static SDL_Window* createWindow();

RenderContext::RenderContext()
{
    window = createWindow();
    renderer = new Renderer(window);
    camera = createCamera(window);
    camera.position.x -= 5;
    camera.lookAt({0.0f, 0.0f, 0.0f});
}

void RenderContext::destroy() noexcept
{
    if (window) {
        delete renderer;
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void RenderContext::drawFrame()
{
    if (!models.empty()) {
        renderer->beginRendering(camera);
        for (auto& [model, matrices] : models) {
            if (!matrices.empty())
                renderer->render(&model, matrices);
        }
        renderer->endRendering();
        for (auto& [model, matrices] : models)
            matrices.clear();
    }
}

void RenderContext::loadTextures(const char* path)
{
    PngLoader loader(renderer);
    auto& registry = AssetRegistry::getRegistry();
    registry.loadDir(path, loader);
}

void RenderContext::loadMaterials(const char* path)
{
    MaterialLoader loader(renderer->getPipelineFactory(), renderer->getDevice(), renderer->getGlobalDescriptorSetLayout());
    auto& registry = AssetRegistry::getRegistry();
    registry.loadDir(path, loader);
}

void RenderContext::loadModels(const char* path)
{
    ObjLoader loader(renderer);
    auto& registry = AssetRegistry::getRegistry();
    registry.loadDir(path, loader);
}

void RenderContext::stopRendering()
{
    renderer->stopRendering();
}

static void loadWindowIcon(SDL_Window* window)
{
    File file("assets/icons/window_icon.png");
    auto data = file.readBytes();
    file.close();

    int width, height, pixelSize;
    stbi_uc* pixels = stbi_load_from_memory(data.data(), (int) data.size(), &width, &height, &pixelSize, STBI_rgb_alpha);
    if (pixels == nullptr)
        throw std::runtime_error("STB image failed to load png");

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
            pixels,
            width,
            height,
            pixelSize * 8,
            pixelSize * width,
            pixelSize == 4 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24
    );
    if (surface == nullptr) {
        stbi_image_free(pixels);
        throw std::runtime_error(fmt::format("SDL failed to create window icon surface: {}", SDL_GetError()));
    }

    SDL_SetWindowIcon(window, surface);
    SDL_FreeSurface(surface);
    stbi_image_free(pixels);
}

SDL_Window* createWindow()
{
    UInt flags = Renderer::SDL_WINDOW_FLAGS | SDL_WINDOW_MOUSE_CAPTURE;
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
    if (window == nullptr)
        throw std::runtime_error(fmt::format("SDL failed to create window: {}", SDL_GetError()));

    try {
        loadWindowIcon(window);
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load window icon: {}", e.what());
    }

    return window;
}

}   // namespace df