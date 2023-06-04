//
// Created by josh on 5/23/23.
//

#pragma once
#include "allocation.h"
#include <model.h>
#include <mutex>
#include <span>
#include <vector>

namespace dragonfire {
class Mesh {
    VmaVirtualAllocation vertexAllocation{}, indexAllocation{};
    VmaVirtualAllocationInfo vertexInfo{}, indexInfo{};

public:
    UInt32 vertexCount = 0, indexCount = 0;

    UInt32 getVertexOffset() const;
    UInt32 getIndexOffset() const;

    class MeshRegistry {
    public:
        MeshRegistry(vk::Device device, VmaAllocator allocator, vk::Queue graphicsQueue, UInt32 graphicsFamily);
        MeshRegistry() = default;
        MeshHandle createMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices);
        void freeMesh(MeshHandle mesh);
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
        std::vector<Mesh*> meshes;
        std::mutex mutex;

        Mesh uploadMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices);
        void freeMeshRegion(const Mesh& mesh);
        void* getStagingPtr(USize size);
    };
};

}   // namespace dragonfire