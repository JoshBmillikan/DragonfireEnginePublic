//
// Created by josh on 5/29/23.
//

#pragma once
#include "allocation.h"
#include "material.h"
#include <map>

namespace dragonfire {

class Texture {
    UInt32 id = 0;
    vk::Sampler sampler;
    vk::ImageView view;
    Image image;

public:
    Texture(UInt32 id, Image&& image, vk::Sampler sampler, vk::Device device);
    Texture() = default;

    class TextureRegistry {
    public:
        TextureRegistry() = default;
        TextureRegistry(
                vk::Device device,
                VmaAllocator allocator,
                vk::Queue graphicsQueue,
                UInt32 graphicsFamily,
                float maxSamplerAnisotropy
        );

        UInt32 loadTexture(
                const std::string& name,
                const void* data,
                UInt32 width,
                UInt32 height,
                UInt bitDepth,
                UInt pixelSize,
                Material::TextureWrapMode wrapS,
                Material::TextureWrapMode wrapT,
                Material::TextureFilterMode minFilter,
                Material::TextureFilterMode magFilter
        );
        void destroy() noexcept;

        ~TextureRegistry() noexcept { destroy(); };

        TextureRegistry(TextureRegistry&) = delete;
        TextureRegistry& operator=(TextureRegistry&) = delete;
        TextureRegistry(TextureRegistry&& other) noexcept;
        TextureRegistry& operator=(TextureRegistry&& other) noexcept;

        void writeDescriptor(const std::string& textureId, vk::DescriptorSet set, UInt32 binding);

    private:
        VmaAllocator allocator = nullptr;
        std::map<std::string, Texture> textures;
        Buffer stagingBuffer;
        vk::Device device = nullptr;
        vk::CommandPool pool;
        vk::CommandBuffer cmd;
        vk::Queue graphicsQueue;
        vk::Fence fence;
        std::mutex mutex;
        UInt32 imageIndex = 1;
        float maxSamplerAnisotropy = 0.0f;

        void* getStagingPtr(USize size);
    };
};

}   // namespace dragonfire