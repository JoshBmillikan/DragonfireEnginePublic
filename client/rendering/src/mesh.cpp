//
// Created by josh on 11/14/22.
//

#include "mesh.h"
#include "renderer.h"

namespace df {

Mesh::Factory::Factory(Renderer* renderer)
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
    allocateInfo.commandBufferCount = 2;
    vk::CommandBuffer buffers[2];
    vk::resultCheck(device.allocateCommandBuffers(&allocateInfo, buffers), "Failed to allocate command buffer");
    cmd = buffers[0];
    secondaryCmd = buffers[1];
    createStagingBuffer(1024);
    fence = device.createFence(vk::FenceCreateInfo());
    semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
}

Mesh* Mesh::Factory::create(Vertex* vertices, UInt vertexCount, UInt* indices, UInt indexCount)
{
    vk::DeviceSize vertexSize = vertexCount * sizeof(Vertex);
    vk::DeviceSize indexSize = indexCount * sizeof(UInt);
    vk::DeviceSize totalSize = vertexSize + indexSize;
    if (stagingBuffer.getInfo().size < totalSize)
        createStagingBuffer(totalSize);

    char* ptr = static_cast<char*>(stagingBuffer.getInfo().pMappedData);
    memcpy(ptr, vertices, vertexSize);
    ptr += vertexSize;
    mempcpy(ptr, indices, indexSize);

    vk::BufferCreateInfo createInfo;
    createInfo.size = totalSize;
    createInfo.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer
                       | vk::BufferUsageFlagBits::eIndexBuffer;
    createInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo;
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.priority = 1;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    Buffer meshBuffer(createInfo, allocInfo);

    device.resetCommandPool(pool);
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);
    vk::BufferCopy copy;
    copy.size = totalSize;
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    cmd.copyBuffer(stagingBuffer, meshBuffer, copy);
    if (transferFamily == graphicsFamily) {
        cmd.end();
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        transferQueue.submit(submitInfo, fence);
    }
    else
        transferQueueOwnership(meshBuffer);

    vk::resultCheck(device.waitForFences(fence, true, UINT64_MAX), "Fence wait failed");
    device.resetFences(fence);
    return new Mesh(std::move(meshBuffer), vertexSize);
}

void Mesh::Factory::destroy() noexcept
{
    if (device) {
        device.destroy(pool);
        device.destroy(fence);
        stagingBuffer.destroy();
        device = nullptr;
    }
}

void Mesh::Factory::createStagingBuffer(vk::DeviceSize size)
{
    vk::BufferCreateInfo createInfo;
    createInfo.size = size;
    createInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    createInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo;
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocInfo.priority = 1;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    stagingBuffer = Buffer(createInfo, allocInfo);
}

void Mesh::Factory::transferQueueOwnership(Buffer& meshBuffer)
{
    {
        vk::BufferMemoryBarrier barrier;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.srcQueueFamilyIndex = transferFamily;
        barrier.dstQueueFamilyIndex = graphicsFamily;
        barrier.buffer = meshBuffer;

        cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                {},
                {},
                barrier,
                {}
        );
        cmd.end();

        vk::SubmitInfo submitInfo;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.commandBufferCount = 1;
        submitInfo.pSignalSemaphores = &semaphore;
        submitInfo.signalSemaphoreCount = 1;

        transferQueue.submit(submitInfo);
    }
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    secondaryCmd.begin(beginInfo);
    vk::BufferMemoryBarrier barrier;
    barrier.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead;
    barrier.srcQueueFamilyIndex = transferFamily;
    barrier.dstQueueFamilyIndex = graphicsFamily;
    barrier.buffer = meshBuffer;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eVertexInput, {}, {}, barrier, {});
    secondaryCmd.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &secondaryCmd;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphore;
    graphicsQueue.submit(submitInfo, fence);
}

Mesh::Factory::Factory(Mesh::Factory&& other) noexcept
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

Mesh::Factory& Mesh::Factory::operator=(Factory&& other) noexcept
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

std::array<vk::VertexInputBindingDescription, 2> Mesh::vertexInputDescriptions = {
        vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
        vk::VertexInputBindingDescription(1, sizeof(glm::mat4), vk::VertexInputRate::eInstance),
};

std::array<vk::VertexInputAttributeDescription, 7> Mesh::vertexAttributeDescriptions = {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)),
        vk::VertexInputAttributeDescription(3, 1, vk::Format::eR32G32B32A32Sfloat, 0),
        vk::VertexInputAttributeDescription(4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4)),
        vk::VertexInputAttributeDescription(5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2),
        vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3),
};
}   // namespace df