//
// Created by josh on 11/9/22.
//

#pragma once

#include "vulkan_includes.h"

namespace df {

class Allocation {
public:
    static void initAllocator(VmaAllocatorCreateInfo* createInfo);
    static void shutdownAllocator() { vmaDestroyAllocator(allocator); }
    virtual ~Allocation() = default;

    [[nodiscard]] VmaAllocationInfo& getInfo() noexcept { return info; };
    [[nodiscard]] const VmaAllocationInfo& getInfo() const noexcept { return info; };
    void* map()
    {
        void* data;
        vmaMapMemory(allocator, allocation, &data);
        return data;
    }

    void unmap() { vmaUnmapMemory(allocator, allocation); }

protected:
    Allocation() = default;
    VmaAllocation allocation{};
    VmaAllocationInfo info{};
    static VmaAllocator allocator;
};

class Buffer : public Allocation {
    vk::Buffer buffer = nullptr;

public:
    Buffer() = default;
    Buffer(vk::BufferCreateInfo& createInfo, VmaAllocationCreateInfo& allocInfo);
    ~Buffer() override { destroy(); };
    DF_NO_COPY(Buffer);
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    operator vk::Buffer() noexcept { return buffer; }
    void destroy() noexcept;
};

class Image : public Allocation {
    vk::Image image = nullptr;
    vk::Format format{};

public:
    Image() = default;
    Image(vk::ImageCreateInfo& createInfo, VmaAllocationCreateInfo& allocInfo);
    ~Image() override { destroy(); }
    DF_NO_COPY(Image);
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;
    [[nodiscard]] vk::Format getFormat() const noexcept { return format; }
    operator vk::Image() const noexcept { return image; }
    void destroy() noexcept;
    [[nodiscard]] vk::ImageView createView(
            vk::Device device,
            const vk::ImageSubresourceRange& subresourceRange,
            vk::ImageViewType type = vk::ImageViewType::e2D
    );
};

}   // namespace df