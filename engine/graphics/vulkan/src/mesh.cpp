//
// Created by josh on 5/23/23.
//

#include "mesh.h"
#include <config.h>

namespace dragonfire {

Mesh Mesh::MeshRegistry::uploadMesh(const std::span<Model::Vertex> vertices, const std::span<UInt32> indices)
{
    const vk::DeviceSize vertexSize = vertices.size() * sizeof(Model::Vertex);
    const vk::DeviceSize indexSize = indices.size() * sizeof(UInt32);
    char* ptr = static_cast<char*>(getStagingPtr(vertexSize + indexSize));
    memcpy(ptr, vertices.data(), vertexSize);
    ptr += vertexSize;
    memcpy(ptr, indices.data(), indexSize);

    VmaVirtualAllocationCreateInfo vertexAllocInfo{}, indexAllocInfo{};
    vertexAllocInfo.size = vertexSize;
    vertexAllocInfo.alignment = 16;
    indexAllocInfo.size = indexSize;
    indexAllocInfo.alignment = 16;

    Mesh mesh{};
    VkResult result = vmaVirtualAllocate(vertexBlock, &vertexAllocInfo, &mesh.vertexAllocation, nullptr);
    if (result == VK_SUCCESS)
        result = vmaVirtualAllocate(indexBlock, &indexAllocInfo, &mesh.indexAllocation, nullptr);
    if (result != VK_SUCCESS)
        throw std::runtime_error("VMA virtual allocation failed");
    vmaGetVirtualAllocationInfo(vertexBlock, mesh.vertexAllocation, &mesh.vertexInfo);
    vmaGetVirtualAllocationInfo(vertexBlock, mesh.vertexAllocation, &mesh.indexInfo);

    device.resetCommandPool(pool);
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);

    vk::BufferCopy vertexCopy;
    vertexCopy.size = vertexSize;
    vertexCopy.srcOffset = 0;
    vertexCopy.dstOffset = mesh.vertexInfo.offset;

    cmd.copyBuffer(stagingBuffer, vertexBuffer, vertexCopy);

    vk::BufferCopy indexCopy;
    indexCopy.size = indexSize;
    indexCopy.srcOffset = vertexSize;
    indexCopy.dstOffset = mesh.indexInfo.offset;

    cmd.copyBuffer(stagingBuffer, indexBuffer, indexCopy);

    vk::SubmitInfo submitInfo{};
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.commandBufferCount = 1;
    cmd.end();

    graphicsQueue.submit(submitInfo, fence);

    vk::resultCheck(device.waitForFences(fence, true, UINT64_MAX), "Fence wait failed");
    device.resetFences(fence);
    mesh.vertexCount = vertices.size();
    mesh.indexCount = indices.size();

    return mesh;
}

void* Mesh::MeshRegistry::getStagingPtr(USize size)
{
    if (stagingBuffer.getInfo().size <= size) {
        stagingBuffer = Buffer::Builder()
                                .withSize(std::min(size, stagingBuffer.getInfo().size * 2))
                                .withSharingMode(vk::SharingMode::eExclusive)
                                .withBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
                                .withAllocator(allocator)
                                .withUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                                .withAllocationFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                .withRequiredFlags(vk::MemoryPropertyFlagBits::eHostVisible)
                                .build();
    }
    return stagingBuffer.getInfo().pMappedData;
}

MeshHandle Mesh::MeshRegistry::createMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices)
{
    std::unique_lock lock(mutex);
    Mesh* mesh = new Mesh(uploadMesh(vertices, indices));
    meshes.push_back(mesh);
    MeshHandle handle = reinterpret_cast<MeshHandle>(mesh);
    return handle;
}

void Mesh::MeshRegistry::freeMesh(MeshHandle mesh)
{
    std::unique_lock lock(mutex);
    Mesh* ptr = reinterpret_cast<Mesh*>(mesh);
    freeMeshRegion(*ptr);
    meshes.erase(std::find(meshes.begin(), meshes.end(), ptr));
    delete ptr;
}

UInt32 Mesh::getVertexOffset()
{
    return vertexInfo.offset / sizeof(Model::Vertex);
}

UInt32 Mesh::getIndexOffset()
{
    return indexInfo.offset / sizeof(UInt32);
}

Mesh::MeshRegistry::MeshRegistry(
        vk::Device device,
        VmaAllocator allocator,
        vk::Queue graphicsQueue,
        UInt32 graphicsFamily
)
    : allocator(allocator), device(device), graphicsQueue(graphicsQueue)
{
    try {
        maxVertexCount = Config::INSTANCE.get<Int64>("graphics.maxVertexCount");
    }
    catch (...) {
        maxVertexCount = 1 << 26;
    }
    try {
        maxIndexCount = Config::INSTANCE.get<Int64>("graphics.maxIndexCount");
    }
    catch (...) {
        maxIndexCount = 1 << 27;
    }
    vertexBuffer = Buffer::Builder()
                           .withSize(sizeof(Model::Vertex) * maxVertexCount)
                           .withBufferUsage(
                                   vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer
                                   | vk::BufferUsageFlagBits::eTransferDst
                           )
                           .withSharingMode(vk::SharingMode::eExclusive)
                           .withRequiredFlags(vk::MemoryPropertyFlagBits::eDeviceLocal)
                           .withPreferredFlags(vk::MemoryPropertyFlagBits::eLazilyAllocated)
                           .withAllocationFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                           .withUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                           .withAllocator(allocator)
                           .build();

    indexBuffer = Buffer::Builder()
                          .withSize(sizeof(UInt32) * maxIndexCount)
                          .withBufferUsage(
                                  vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                                  | vk::BufferUsageFlagBits::eTransferDst
                          )
                          .withSharingMode(vk::SharingMode::eExclusive)
                          .withRequiredFlags(vk::MemoryPropertyFlagBits::eDeviceLocal)
                          .withPreferredFlags(vk::MemoryPropertyFlagBits::eLazilyAllocated)
                          .withAllocationFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                          .withUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                          .withAllocator(allocator)
                          .build();

    stagingBuffer = Buffer::Builder()
                            .withSize(1 << 15)
                            .withSharingMode(vk::SharingMode::eExclusive)
                            .withBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
                            .withAllocator(allocator)
                            .withUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                            .withAllocationFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                            .withRequiredFlags(vk::MemoryPropertyFlagBits::eHostVisible)
                            .build();

    VmaVirtualBlockCreateInfo blockCreateInfo{};
    blockCreateInfo.size = vertexBuffer.getInfo().size;
    VkResult result = vmaCreateVirtualBlock(&blockCreateInfo, &vertexBlock);
    if (result == VK_SUCCESS) {
        blockCreateInfo.size = indexBuffer.getInfo().size;
        result = vmaCreateVirtualBlock(&blockCreateInfo, &indexBlock);
    }
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create virtual blocks for vertex/index buffers");

    vk::CommandPoolCreateInfo createInfo{};
    createInfo.queueFamilyIndex = graphicsFamily;
    pool = device.createCommandPool(createInfo);
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::resultCheck(device.allocateCommandBuffers(&allocInfo, &cmd), "Failed to allocate command buffer");
    fence = device.createFence(vk::FenceCreateInfo());

    spdlog::get("Rendering")
            ->info("Initialized mesh registry, max vertex count {}, max index count {}", maxVertexCount, maxIndexCount);
}

void Mesh::MeshRegistry::freeMeshRegion(const Mesh& mesh)
{
    vmaVirtualFree(vertexBlock, mesh.vertexAllocation);
    vmaVirtualFree(indexBlock, mesh.indexAllocation);
}

void Mesh::MeshRegistry::bindBuffers(vk::CommandBuffer buf)
{
    buf.bindVertexBuffers(0, {vertexBuffer}, {0});
    buf.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
}

void Mesh::MeshRegistry::destroy() noexcept
{
    if (device) {
        device.destroy(pool);
        vmaClearVirtualBlock(vertexBlock);
        vmaClearVirtualBlock(indexBlock);
        device.destroy(fence);
        vertexBuffer.destroy();
        indexBuffer.destroy();
        stagingBuffer.destroy();
        device = nullptr;
    }
}

Mesh::MeshRegistry::MeshRegistry(Mesh::MeshRegistry&& other) noexcept
{
    if (this != &other) {
        destroy();
        std::unique_lock l1(mutex), l2(other.mutex);
        device = other.device;
        other.device = nullptr;
        maxVertexCount = other.maxVertexCount;
        maxIndexCount = other.maxIndexCount;
        allocator = other.allocator;
        vertexBlock = other.vertexBlock;
        indexBlock = other.indexBlock;
        pool = other.pool;
        cmd = other.cmd;
        graphicsQueue = other.graphicsQueue;
        fence = other.fence;
        vertexBuffer = std::move(other.vertexBuffer);
        indexBuffer = std::move(other.indexBuffer);
        stagingBuffer = std::move(other.stagingBuffer);
    }
}

Mesh::MeshRegistry& Mesh::MeshRegistry::operator=(Mesh::MeshRegistry&& other) noexcept
{
    if (this != &other) {
        destroy();
        std::unique_lock l1(mutex), l2(other.mutex);
        device = other.device;
        other.device = nullptr;
        maxVertexCount = other.maxVertexCount;
        maxIndexCount = other.maxIndexCount;
        allocator = other.allocator;
        vertexBlock = other.vertexBlock;
        indexBlock = other.indexBlock;
        pool = other.pool;
        cmd = other.cmd;
        graphicsQueue = other.graphicsQueue;
        fence = other.fence;
        vertexBuffer = std::move(other.vertexBuffer);
        indexBuffer = std::move(other.indexBuffer);
        stagingBuffer = std::move(other.stagingBuffer);
    }
    return *this;
}
}   // namespace dragonfire