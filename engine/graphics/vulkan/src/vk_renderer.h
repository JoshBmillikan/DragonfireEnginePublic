//
// Created by josh on 5/17/23.
//

#pragma once
#include "allocation.h"
#include "descriptor_set.h"
#include "mesh.h"
#include "swapchain.h"
#include "vk_material.h"
#include <glm/glm.hpp>
#include <renderer.h>
#include <thread>

namespace dragonfire {

class VkRenderer : public Renderer {
public:
    void init() override;
    void shutdown() override;
    Material::Library* getMaterialLibrary() override;
    MeshHandle createMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices) override;
    void freeMesh(MeshHandle mesh) override;
    void render(class World& world, const Camera& camera) override;

    static constexpr USize FRAMES_IN_FLIGHT = 2;

private:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    vk::SampleCountFlagBits msaaSamples;
    UInt32 maxDrawCount = 0;

    struct Queues {
        UInt32 graphicsFamily = 0, presentFamily = 0, transferFamily = 0;
        vk::Queue graphics, present, transfer;
    } queues;

    Swapchain swapchain;
    VmaAllocator allocator;
    Image depthImage, msaaImage;
    vk::ImageView depthView, msaaView;
    vk::RenderPass mainRenderPass;
    DescriptorLayoutManager layoutManager;
    VkMaterial::VkLibrary materialLibrary;
    Mesh::MeshRegistry meshRegistry;
    vk::Pipeline cullComputePipeline;
    vk::PipelineLayout cullComputeLayout;
    Buffer globalUBO;
    USize uboOffset = 0;

    struct Frame {
        vk::CommandPool pool;
        vk::CommandBuffer cmd;
        vk::DescriptorSet globalDescriptorSet, computeSet;
        Buffer drawData, culledMatrices, commandBuffer, countBuffer;
        vk::Semaphore renderSemaphore, presentSemaphore;
        vk::Fence fence;
    } frames[FRAMES_IN_FLIGHT], *presentingFrame = nullptr;

    struct {
        std::jthread thread;
        std::mutex mutex;
        std::condition_variable_any condVar;
        vk::Result result = vk::Result::eSuccess;
    } presentData;

    struct DrawData {
        glm::mat4 transform;
        UInt32 vertexOffset, vertexCount, indexOffset, indexCount, pipelineIndex;
    };

    struct PipelineDrawInfo {
        UInt32 index, drawCount;
        vk::PipelineLayout layout;
    };

    ankerl::unordered_dense::map<vk::Pipeline, PipelineDrawInfo> pipelineMap;

private:
    void present(const std::stop_token& stopToken);
    void startFrame();
    void beginRenderingCommands(const Camera& camera);
    void computePrePass(UInt32 drawCount);
    void renderMainPass();
    void startRenderPass(vk::RenderPass pass, std::span<vk::ClearValue> clearValues);

    Frame& getCurrentFrame() { return frames[frameCount % FRAMES_IN_FLIGHT]; }

    void createInstance(bool validation);
    void getPhysicalDevice();
    bool getQueueFamilies(vk::PhysicalDevice pDevice) noexcept;
    void createDevice();
    void createGpuAllocator();
    void createDepthImage();
    void createMsaaImage();
    void createRenderPass();
    void createGlobalUBO();
    void initFrame(Frame& frame);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    );

public:
    [[nodiscard]] vk::Device getDevice() const { return device; }

    [[nodiscard]] vk::SampleCountFlagBits getSampleCount() const { return msaaSamples; }

    [[nodiscard]] std::vector<vk::RenderPass> getRenderPasses() const { return {mainRenderPass}; }

    [[nodiscard]] DescriptorLayoutManager* getLayoutManager() { return &layoutManager; }
};

struct alignas(16) UBOData {
    glm::mat4 perspective;
    glm::mat4 orthographic;
    glm::vec3 sunDirection;
    glm::vec2 resolution;
};

}   // namespace dragonfire