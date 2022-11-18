//
// Created by josh on 11/9/22.
//
#define VMA_IMPLEMENTATION
#include "allocation.h"

namespace df {
VmaAllocator Allocation::allocator;

void Allocation::initAllocator(VmaAllocatorCreateInfo* createInfo)
{
    assert(createInfo);
    if (vmaCreateAllocator(createInfo, &Allocation::allocator) != VK_SUCCESS)
        crash("Failed to initialize VMA allocator");
}

Buffer::Buffer(vk::BufferCreateInfo& createInfo, VmaAllocationCreateInfo& allocInfo)
{
    VkResult result;
    if ((result = vmaCreateBuffer(
                 allocator,
                 reinterpret_cast<const VkBufferCreateInfo*>(&createInfo),
                 &allocInfo,
                 reinterpret_cast<VkBuffer*>(&buffer),
                 &allocation,
                 &info
         ))
        != VK_SUCCESS) {
        vk::throwResultException(static_cast<vk::Result>(result), "Vma failed to allocate buffer");
    }
    spdlog::trace("VMA allocated GPU buffer of size {}", info.size);
}

Buffer::Buffer(Buffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        buffer = other.buffer;
        other.buffer = nullptr;
    }
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        buffer = other.buffer;
        other.buffer = nullptr;
    }
    return *this;
}

void Buffer::destroy() noexcept
{
    if (buffer) {
        vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = nullptr;
    }
}

Image::Image(vk::ImageCreateInfo& createInfo, VmaAllocationCreateInfo& allocInfo)
{
    format = createInfo.format;
    VkResult result;
    if ((result = vmaCreateImage(
                 allocator,
                 reinterpret_cast<const VkImageCreateInfo*>(&createInfo),
                 &allocInfo,
                 reinterpret_cast<VkImage*>(&image),
                 &allocation,
                 &info
         ))
        != VK_SUCCESS) {
        vk::throwResultException(static_cast<vk::Result>(result), "VMA failed to allocate image");
    }
}

vk::ImageView Image::createView(vk::Device device, const vk::ImageSubresourceRange& subresourceRange, vk::ImageViewType type)
{
    assert(image);
    vk::ImageViewCreateInfo createInfo;
    createInfo.image = image;
    createInfo.viewType = type;
    createInfo.format = format;
    createInfo.subresourceRange = subresourceRange;

    return device.createImageView(createInfo);
}

Image::Image(Image&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        image = other.image;
        other.image = nullptr;
        format = other.format;
    }
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        image = other.image;
        other.image = nullptr;
        format = other.format;
    }
    return *this;
}

void Image::destroy() noexcept
{
    if (image) {
        vmaDestroyImage(allocator, image, allocation);
        image = nullptr;
    }
}
}   // namespace df