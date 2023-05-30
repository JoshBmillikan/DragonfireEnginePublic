//
// Created by josh on 5/20/23.
//
#define VMA_IMPLEMENTATION
#include "allocation.h"

namespace dragonfire {

void* Allocation::map() noexcept
{
    void* ptr;
    if (vmaMapMemory(allocator, allocation, &ptr) != VK_SUCCESS)
        return nullptr;
    return ptr;
}

void Allocation::flush()
{
    vk::resultCheck(
            static_cast<vk::Result>(vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE)),
            "Failed to flush allocation memory"
    );
}

Buffer Buffer::Builder::build()
{
    VkBufferCreateInfo& create = createInfo;
    Buffer buf;
    VkBuffer vkBuffer;
    buf.allocator = allocator;
    vk::resultCheck(
            static_cast<vk::Result>(
                    vmaCreateBuffer(allocator, &create, &allocInfo, &vkBuffer, &buf.allocation, &buf.info)
            ),
            "Failed to allocate buffer"
    );
    buf.buffer = vkBuffer;
    spdlog::trace("Allocated buffer of size {} bytes", createInfo.size);

    return buf;
}

void Buffer::destroy()
{
    if (buffer) {
        vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = nullptr;
    }
}

Buffer::Buffer(Buffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        buffer = other.buffer;
        allocator = other.allocator;
        other.buffer = nullptr;
    }
}

Buffer& Buffer::operator=(dragonfire::Buffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        buffer = other.buffer;
        allocator = other.allocator;
        other.buffer = nullptr;
    }
    return *this;
}

Image Image::Builder::build()
{
    VkImageCreateInfo& create = createInfo;
    Image i;
    VkImage vkImage;
    i.allocator = allocator;
    i.extent = createInfo.extent;
    i.imageFormat = createInfo.format;
    vk::resultCheck(
            static_cast<vk::Result>(vmaCreateImage(allocator, &create, &allocInfo, &vkImage, &i.allocation, &i.info)),
            "Failed to allocate image"
    );
    i.image = vkImage;
    spdlog::trace("Allocated image of extent {}x{}x{}", i.extent.width, i.extent.height, i.extent.depth);
    return i;
}

vk::ImageView Image::createView(
        vk::Device device,
        const vk::ImageSubresourceRange& subresourceRange,
        vk::ImageViewType type
)
{
    assert(image);
    vk::ImageViewCreateInfo createInfo;
    createInfo.image = image;
    createInfo.viewType = type;
    createInfo.format = imageFormat;
    createInfo.subresourceRange = subresourceRange;

    return device.createImageView(createInfo);
}

void Image::destroy()
{
    if (image) {
        vmaDestroyImage(allocator, image, allocation);
        image = nullptr;
    }
}

Image::Image(dragonfire::Image&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        image = other.image;
        extent = other.extent;
        imageFormat = other.imageFormat;
        allocator = other.allocator;
        other.image = nullptr;
    }
}

Image& Image::operator=(dragonfire::Image&& other) noexcept
{
    if (this != &other) {
        destroy();
        allocation = other.allocation;
        info = other.info;
        image = other.image;
        extent = other.extent;
        imageFormat = other.imageFormat;
        allocator = other.allocator;
        other.image = nullptr;
    }
    return *this;
}

Buffer::Builder::Builder()
{
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    allocInfo.priority = 1.0f;
}

Buffer::Builder& Buffer::Builder::withSize(vk::DeviceSize size)
{
    createInfo.size = size;
    return *this;
}

Buffer::Builder& Buffer::Builder::withBufferUsage(vk::BufferUsageFlags usage)
{
    createInfo.usage = usage;
    return *this;
}

Buffer::Builder& Buffer::Builder::withSharingMode(vk::SharingMode mode)
{
    createInfo.sharingMode = mode;
    return *this;
}

Buffer::Builder& Buffer::Builder::withBufferFlags(vk::BufferCreateFlags flags)
{
    createInfo.flags = flags;
    return *this;
}

Buffer::Builder& Buffer::Builder::withQueueFamilies(const vk::ArrayProxy<UInt32>& array)
{
    createInfo.pQueueFamilyIndices = array.data();
    createInfo.queueFamilyIndexCount = array.size();
    return *this;
}

Image::Builder::Builder()
{
    createInfo.initialLayout = vk::ImageLayout::eUndefined;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.tiling = vk::ImageTiling::eOptimal;
}

Image::Builder& Image::Builder::withFormat(vk::Format fmt)
{
    createInfo.format = fmt;
    return *this;
}

Image::Builder& Image::Builder::withExtent(vk::Extent3D ext)
{
    createInfo.extent = ext;
    return *this;
}

Image::Builder& Image::Builder::withExtent(vk::Extent2D ext)
{
    createInfo.extent = vk::Extent3D(ext, 1);
    return *this;
}

Image::Builder& Image::Builder::withImageUsage(vk::ImageUsageFlags usage)
{
    createInfo.usage = usage;
    return *this;
}

Image::Builder& Image::Builder::withImageType(vk::ImageType type)
{
    createInfo.imageType = type;
    return *this;
}

Image::Builder& Image::Builder::withMipLevels(UInt32 levels)
{
    createInfo.mipLevels = levels;
    return *this;
}

Image::Builder& Image::Builder::withArrayLayers(UInt32 layers)
{
    createInfo.arrayLayers = layers;
    return *this;
}

Image::Builder& Image::Builder::withSamples(vk::SampleCountFlagBits samples)
{
    createInfo.samples = samples;
    return *this;
}

Image::Builder& Image::Builder::withTiling(vk::ImageTiling tiling)
{
    createInfo.tiling = tiling;
    return *this;
}

Image::Builder& Image::Builder::withSharingMode(vk::SharingMode mode)
{
    createInfo.sharingMode = mode;
    return *this;
}

Image::Builder& Image::Builder::withInitialLayout(vk::ImageLayout layout)
{
    createInfo.initialLayout = layout;
    return *this;
}

Image::Builder& Image::Builder::withQueueFamilies(const vk::ArrayProxy<UInt32>& array)
{
    createInfo.pQueueFamilyIndices = array.data();
    createInfo.queueFamilyIndexCount = array.size();
    return *this;
}

}   // namespace dragonfire