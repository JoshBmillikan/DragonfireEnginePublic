//
// Created by josh on 5/17/23.
//

#pragma once
#include "camera.h"
#include "material.h"
#include "model.h"
#include <SDL2/SDL_video.h>

namespace dragonfire {

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void init() = 0;
    virtual void shutdown() = 0;
    virtual MeshHandle createMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices) = 0;
    virtual UInt32 loadTexture(
            const std::string& name,
            const void* data,
            UInt32 width,
            UInt32 height,
            UInt bitDepth,
            UInt pixelSize = 1,
            Material::TextureWrapMode wrapS = Material::TextureWrapMode::REPEAT,
            Material::TextureWrapMode wrapT = Material::TextureWrapMode::REPEAT,
            Material::TextureFilterMode minFilter = Material::TextureFilterMode::NONE,
            Material::TextureFilterMode magFilter = Material::TextureFilterMode::NONE
    ) = 0;
    virtual void freeMesh(MeshHandle mesh) = 0;
    virtual void render(class World& world, const Camera& camera) = 0;

protected:
    UInt64 frameCount = 0;
    std::shared_ptr<spdlog::logger> logger;
    SDL_Window* window = nullptr;

    void createWindow(SDL_WindowFlags sdlFlags);
};

extern Renderer* getRenderer();

}   // namespace dragonfire