//
// Created by josh on 11/14/22.
//

#pragma once
#include "allocation.h"
#include "vertex.h"
#include "vulkan_includes.h"
#include <array>
#include <asset.h>

namespace df {

class Mesh : public Asset {
    Buffer buffer;
    vk::DeviceSize indexOffset = 0, indexCount = 0;

public:
    Mesh() = default;
    Mesh(Buffer&& buffer, vk::DeviceSize indexOffset, vk::DeviceSize indexCount) noexcept
        : buffer(std::move(buffer)), indexOffset(indexOffset), indexCount(indexCount)
    {
    }
    void setName(const std::string& str) noexcept { name = str; }
    void destroy() noexcept { buffer.destroy(); }
    static std::array<vk::VertexInputBindingDescription, 2> vertexInputDescriptions;
    static std::array<vk::VertexInputAttributeDescription, 7> vertexAttributeDescriptions;
    [[nodiscard]] vk::DeviceSize getIndexOffset() const { return indexOffset; }
    [[nodiscard]] vk::DeviceSize getIndexCount() const { return indexCount; }
    [[nodiscard]] vk::Buffer getBuffer() { return buffer; }

    class Factory {
    public:
        explicit Factory(class Renderer* renderer);
        Mesh* create(Vertex* vertices, UInt vertexCount, UInt* indices, UInt numIndices);
        Mesh* create(std::vector<Vertex>& vertices, std::vector<UInt>& indices)
        {
            return create(vertices.data(), vertices.size(), indices.data(), indices.size());
        }
        void destroy() noexcept;
        ~Factory() noexcept { destroy(); }
        DF_NO_COPY(Factory);
        Factory(Factory&& other) noexcept;
        Factory& operator=(Factory&& other) noexcept;

    private:
        vk::Device device;
        Buffer stagingBuffer;
        vk::CommandPool pool, secondaryPool;
        vk::CommandBuffer cmd, secondaryCmd;
        UInt graphicsFamily = 0, transferFamily = 0;
        vk::Queue transferQueue, graphicsQueue;
        vk::Fence fence;
        vk::Semaphore semaphore;
        void transferQueueOwnership(Buffer& meshBuffer);
        void createStagingBuffer(vk::DeviceSize size);
    };
};

}   // namespace df