//
// Created by josh on 5/20/23.
//

#pragma once
#include <vk_mem_alloc.h>

namespace dragonfire {

class Allocation {
public:
    void* map() noexcept;
    void flush();
    virtual ~Allocation() = default;
    [[nodiscard]] const VmaAllocationInfo& getInfo() const { return info; };

protected:
    Allocation() = default;
    VmaAllocator allocator = nullptr;
    VmaAllocation allocation{};
    VmaAllocationInfo info{};

    template<typename T>
    class Builder {
    public:
        Builder();
        T& withAllocator(VmaAllocator alloc);
        T& withAllocationFlags(VmaAllocationCreateFlags flags);
        T& withPriority(float priority);
        T& withRequiredFlags(vk::MemoryPropertyFlags flags);
        T& withPreferredFlags(vk::MemoryPropertyFlags flags);
        T& withUsage(VmaMemoryUsage usage);

    protected:
        VmaAllocator allocator = nullptr;
        VmaAllocationCreateInfo allocInfo;
    };
};

class Buffer : public Allocation {
    vk::Buffer buffer = nullptr;

public:
    Buffer() = default;

    Buffer(Buffer&) = delete;
    Buffer& operator=(Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    operator vk::Buffer() const { return buffer; }

    void destroy();

    ~Buffer() override { destroy(); }

    class Builder : public Allocation::Builder<Builder> {
        vk::BufferCreateInfo createInfo;

    public:
        Builder();

        Builder& withSize(vk::DeviceSize size);
        Builder& withBufferUsage(vk::BufferUsageFlags usage);
        Builder& withSharingMode(vk::SharingMode mode);
        Builder& withBufferFlags(vk::BufferCreateFlags flags);
        Builder& withQueueFamilies(const vk::ArrayProxy<uint32_t>& array);

        Buffer build();
    };
};

class Image : public Allocation {
    vk::Image image = nullptr;
    vk::Format imageFormat{};
    vk::Extent3D extent{};

public:
    Image() = default;

    Image(Image&) = delete;
    Image& operator=(Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    operator vk::Image() const { return image; }

    void destroy();

    ~Image() override { destroy(); }

    [[nodiscard]] vk::ImageView createView(
            vk::Device device,
            const vk::ImageSubresourceRange& subresourceRange,
            vk::ImageViewType type = vk::ImageViewType::e2D
    );

    class Builder : public Allocation::Builder<Builder> {
        vk::ImageCreateInfo createInfo;

    public:
        Builder();

        Builder& withFormat(vk::Format fmt);
        Builder& withExtent(vk::Extent3D ext);
        Builder& withExtent(vk::Extent2D ext);
        Builder& withImageUsage(vk::ImageUsageFlags usage);
        Builder& withImageType(vk::ImageType type);
        Builder& withMipLevels(uint32_t levels);
        Builder& withArrayLayers(uint32_t layers);
        Builder& withSamples(vk::SampleCountFlagBits samples);
        Builder& withTiling(vk::ImageTiling tiling);
        Builder& withSharingMode(vk::SharingMode mode);
        Builder& withInitialLayout(vk::ImageLayout layout);
        Builder& withQueueFamilies(const vk::ArrayProxy<uint32_t>& array);

        Image build();
    };
};

template<typename T>
Allocation::Builder<T>::Builder() : allocInfo({})
{
    allocInfo.priority = 1.0f;
}

template<typename T>
T& Allocation::Builder<T>::withAllocator(VmaAllocator alloc)
{
    this->allocator = alloc;
    return static_cast<T&>(*this);
}

template<typename T>
T& Allocation::Builder<T>::withAllocationFlags(VmaAllocationCreateFlags flags)
{
    allocInfo.flags = flags;
    return static_cast<T&>(*this);
}

template<typename T>
T& Allocation::Builder<T>::withPriority(float priority)
{
    allocInfo.priority = priority;
    return static_cast<T&>(*this);
}

template<typename T>
T& Allocation::Builder<T>::withRequiredFlags(vk::MemoryPropertyFlags flags)
{
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(flags);
    return static_cast<T&>(*this);
}

template<typename T>
T& Allocation::Builder<T>::withPreferredFlags(vk::MemoryPropertyFlags flags)
{
    allocInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(flags);
    return static_cast<T&>(*this);
}

template<typename T>
T& Allocation::Builder<T>::withUsage(VmaMemoryUsage usage)
{
    allocInfo.usage = usage;
    return static_cast<T&>(*this);
}
}   // namespace dragonfire