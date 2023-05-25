//
// Created by josh on 5/23/23.
//

#pragma once
#include "allocation.h"
#include <model.h>
#include <mutex>
#include <span>

namespace dragonfire {
class Mesh {
    VmaVirtualAllocation vertexAllocation{}, indexAllocation{};
    VmaVirtualAllocationInfo vertexInfo{}, indexInfo{};

public:
    class MeshRegistry {
    public:
        MeshRegistry(vk::Device device, VmaAllocator allocator, vk::Queue graphicsQueue, UInt32 graphicsFamily);
        MeshRegistry() = default;
        Mesh uploadMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices);
        void freeMesh(const Mesh& mesh);
        void bindBuffers(vk::CommandBuffer buf);
        void destroy() noexcept;

        MeshRegistry(MeshRegistry&) = delete;
        MeshRegistry& operator=(MeshRegistry&) = delete;
        MeshRegistry(MeshRegistry&& other) noexcept;
        MeshRegistry& operator=(MeshRegistry&& other) noexcept;

        ~MeshRegistry() noexcept { destroy(); }

    private:
        USize maxVertexCount = 0, maxIndexCount = 0;
        VmaAllocator allocator = nullptr;
        VmaVirtualBlock vertexBlock{}, indexBlock{};
        vk::Device device;
        Buffer vertexBuffer, indexBuffer, stagingBuffer;
        vk::CommandPool pool;
        vk::CommandBuffer cmd;
        vk::Queue graphicsQueue;
        vk::Fence fence;
        std::mutex mutex;

        void* getStagingPtr(USize size);
    };
};

}   // namespace dragonfire