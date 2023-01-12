//
// Created by josh on 11/19/22.
//

#pragma once
#include "allocation.h"
#include <asset.h>

namespace df {

class Texture : public Asset {
    Image image;
    vk::Extent2D imageExtent;
    vk::ImageView view;
    vk::Device device;

public:
    Texture(Image&& image, vk::Extent2D extent, vk::Device device);
    [[nodiscard]] vk::Extent2D getExtent() const noexcept { return imageExtent; }
    ~Texture() noexcept override { destroy(); }
    void destroy() noexcept;
    DF_NO_MOVE_COPY(Texture);

    class Factory {
    public:
        explicit Factory(class Renderer* renderer);
        ~Factory() noexcept { destroy(); }
        void destroy() noexcept;
        Factory() = delete;
        Texture* create(vk::Extent2D extent);
        Texture* create(vk::Extent2D extent, void const* buffer, Size size);
        void* getBufferMemory(vk::DeviceSize size);
        DF_NO_COPY(Factory);
        Factory(Factory&& other) noexcept;
        Factory& operator=(Factory&& other) noexcept;

    private:
        Buffer stagingBuffer;
        vk::Device device;
        vk::CommandPool pool, secondaryPool;
        vk::CommandBuffer cmd, secondaryCmd;
        UInt graphicsFamily, transferFamily;
        vk::Queue transferQueue, graphicsQueue;
        vk::Fence fence;
        vk::Semaphore semaphore;

        void createStagingBuffer(vk::DeviceSize size);
        void transferOwnership(vk::Image texture);
    };
};

}   // namespace df