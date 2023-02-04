//
// Created by josh on 11/19/22.
//

#include "texture.h"
#include "error_texture.h"
#include "renderer.h"

namespace df {
Texture::Texture(Image&& image, vk::Extent2D extent, vk::Device device)
    : image(std::move(image)), imageExtent(extent), device(device)
{
    vk::ImageSubresourceRange range{};
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    view = this->image.createView(device, range);
    static UInt textureIndex = 0;
    index = textureIndex++;
}

void Texture::destroy() noexcept
{
    if (view) {
        device.destroy(view);
        view = nullptr;
    }
}

Texture::Factory::Factory(Renderer* renderer)
{
    device = renderer->device;
    graphicsFamily = renderer->queues.graphicsFamily;
    transferFamily = renderer->queues.transferFamily;
    transferQueue = renderer->queues.transfer;
    graphicsQueue = renderer->queues.graphics;

    vk::CommandPoolCreateInfo createInfo;
    createInfo.queueFamilyIndex = transferFamily;
    createInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    pool = device.createCommandPool(createInfo);
    vk::CommandBufferAllocateInfo allocateInfo;
    allocateInfo.commandPool = pool;
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandBufferCount = 1;
    vk::resultCheck(device.allocateCommandBuffers(&allocateInfo, &cmd), "Failed to allocate command buffer");
    createInfo.queueFamilyIndex = graphicsFamily;
    secondaryPool = device.createCommandPool(createInfo);
    allocateInfo.commandPool = secondaryPool;
    vk::resultCheck(device.allocateCommandBuffers(&allocateInfo, &secondaryCmd), "Failed to allocate command buffer");
    createStagingBuffer(4096);
    fence = device.createFence(vk::FenceCreateInfo());
    semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
}

static Image createImage(vk::Extent2D extent)
{
    vk::ImageCreateInfo createInfo;
    createInfo.extent = vk::Extent3D(extent, 1);
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = vk::Format::eR8G8B8A8Srgb;
    createInfo.tiling = vk::ImageTiling::eOptimal;
    createInfo.initialLayout = vk::ImageLayout::eUndefined;
    createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    createInfo.samples = vk::SampleCountFlagBits::e1;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.priority = 1;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    return {createInfo, allocInfo};
}

Texture* Texture::Factory::create(vk::Extent2D extent)
{
    Image texture = createImage(extent);
    device.resetCommandPool(pool);
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);

    vk::ImageMemoryBarrier transferBarrier, imageBarrier;
    transferBarrier.image = texture;
    transferBarrier.oldLayout = vk::ImageLayout::eUndefined;
    transferBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    transferBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    transferBarrier.subresourceRange.baseMipLevel = 0;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = 0;
    transferBarrier.subresourceRange.layerCount = 1;

    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            {},
            {},
            transferBarrier
    );

    vk::BufferImageCopy copy;
    copy.imageExtent = vk::Extent3D(extent, 1);
    copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;

    cmd.copyBufferToImage(stagingBuffer, texture, vk::ImageLayout::eTransferDstOptimal, copy);

    imageBarrier.image = texture;
    imageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    imageBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    imageBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;
    if (transferFamily == graphicsFamily) {
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    else {
        imageBarrier.srcQueueFamilyIndex = transferFamily;
        imageBarrier.dstQueueFamilyIndex = graphicsFamily;
    }

    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            transferFamily == graphicsFamily ? vk::PipelineStageFlagBits::eFragmentShader
                                             : vk::PipelineStageFlagBits::eBottomOfPipe,
            {},
            {},
            {},
            imageBarrier
    );

    if (transferFamily == graphicsFamily) {
        cmd.end();
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        transferQueue.submit(submitInfo, fence);
    }
    else
        transferOwnership(texture);

    vk::resultCheck(device.waitForFences(fence, true, UINT64_MAX), "Fence wait failed");
    device.resetFences(fence);
    return new Texture(std::move(texture), extent, device);
}

Texture* Texture::Factory::create(vk::Extent2D extent, void const* buffer, Size size)
{
    void* ptr = getBufferMemory(size);
    memcpy(ptr, buffer, size);
    return create(extent);
}

void Texture::Factory::transferOwnership(vk::Image texture)
{
    {
        cmd.end();
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.pSignalSemaphores = &semaphore;
        submitInfo.signalSemaphoreCount = 1;
        transferQueue.submit(submitInfo);
    }

    device.resetCommandPool(secondaryPool);
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    secondaryCmd.begin(beginInfo);
    vk::ImageMemoryBarrier barrier;
    barrier.image = texture;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    barrier.srcQueueFamilyIndex = transferFamily;
    barrier.dstQueueFamilyIndex = graphicsFamily;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    secondaryCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

    secondaryCmd.end();
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &secondaryCmd;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphore;
    vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eVertexInput;
    submitInfo.pWaitDstStageMask = &mask;
    graphicsQueue.submit(submitInfo, fence);
}

void* Texture::Factory::getBufferMemory(vk::DeviceSize size)
{
    vk::DeviceSize currentSize = stagingBuffer.getInfo().size;
    if (currentSize < size)
        createStagingBuffer(std::max(currentSize * 2, size));
    return stagingBuffer.getInfo().pMappedData;
}

Texture* Texture::Factory::createErrorTexture()
{
    vk::Extent2D extent(gimp_image.width, gimp_image.height);
    UInt size = gimp_image.width * gimp_image.height;
    UByte* ptr = (UByte*) getBufferMemory(size * gimp_image.bytes_per_pixel);
    GIMP_IMAGE_RUN_LENGTH_DECODE(ptr, gimp_image.rle_pixel_data, size, gimp_image.bytes_per_pixel);
    return create(extent);
}

void Texture::Factory::createStagingBuffer(vk::DeviceSize size)
{
    vk::BufferCreateInfo createInfo;
    createInfo.size = size;
    createInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    createInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocInfo.priority = 1;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    stagingBuffer = Buffer(createInfo, allocInfo);
}

void Texture::Factory::destroy() noexcept
{
    if (device) {
        device.destroy(pool);
        device.destroy(secondaryPool);
        device.destroy(fence);
        device.destroy(semaphore);
        stagingBuffer.destroy();
        device = nullptr;
    }
}

Texture::Factory::Factory(Texture::Factory&& other) noexcept
{
    if (this != &other) {
        device = other.device;
        stagingBuffer = std::move(other.stagingBuffer);
        pool = other.pool;
        cmd = other.cmd;
        secondaryCmd = other.secondaryCmd;
        graphicsFamily = other.graphicsFamily;
        transferFamily = other.transferFamily;
        transferQueue = other.transferQueue;
        graphicsQueue = other.graphicsQueue;
        fence = other.fence;
        semaphore = other.semaphore;
        other.device = nullptr;
    }
}

Texture::Factory& Texture::Factory::operator=(Texture::Factory&& other) noexcept
{
    if (this != &other) {
        device = other.device;
        stagingBuffer = std::move(other.stagingBuffer);
        pool = other.pool;
        cmd = other.cmd;
        secondaryCmd = other.secondaryCmd;
        graphicsFamily = other.graphicsFamily;
        transferFamily = other.transferFamily;
        transferQueue = other.transferQueue;
        graphicsQueue = other.graphicsQueue;
        fence = other.fence;
        semaphore = other.semaphore;
        other.device = nullptr;
    }
    return *this;
}
}   // namespace df