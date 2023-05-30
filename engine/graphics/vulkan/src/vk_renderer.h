//
// Created by josh on 5/17/23.
//

#pragma once
#include "allocation.h"
#include "descriptor_set.h"
#include "mesh.h"
#include "pipeline.h"
#include "swapchain.h"
#include "texture.h"
#include <glm/glm.hpp>
#include <renderer.h>
#include <thread>

namespace dragonfire {

class VkRenderer : public Renderer {
public:
    void init() override;
    void shutdown() override;

    MeshHandle createMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices) override;
    void freeMesh(MeshHandle mesh) override;
    void render(World& world, const Camera& camera) override;
    void startImGuiFrame() override;
    UInt32 loadTexture(
            const std::string& name,
            const void* data,
            UInt32 width,
            UInt32 height,
            UInt bitDepth,
            UInt pixelSize,
            Material::TextureWrapMode wrapS,
            Material::TextureWrapMode wrapT,
            Material::TextureFilterMode minFilter,
            Material::TextureFilterMode magFilter
    ) override;

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
    Pipeline::PipelineLibrary pipelineLibrary;
    Mesh::MeshRegistry meshRegistry;
    Texture::TextureRegistry textureRegistry;
    vk::Pipeline cullComputePipeline;
    vk::PipelineLayout cullComputeLayout;
    vk::DescriptorPool descriptorPool;
    Buffer globalUBO;
    USize uboOffset = 0;

    struct Frame {
        vk::CommandPool pool;
        vk::CommandBuffer cmd;
        vk::DescriptorSet globalDescriptorSet, computeSet, frameSet;
        Buffer drawData, culledMatrices, commandBuffer, countBuffer, textureIndexBuffer;
        vk::Semaphore renderSemaphore, presentSemaphore;
        vk::Fence fence;
        UInt32 textureBinding = 0;
    } frames[FRAMES_IN_FLIGHT], *presentingFrame = nullptr;

    struct {
        std::jthread thread;
        std::mutex mutex;
        std::condition_variable_any condVar;
        vk::Result result = vk::Result::eSuccess;
    } presentData;

    struct DrawData {
        glm::mat4 transform{};
        UInt32 vertexOffset = 0, vertexCount = 0, indexOffset = 0, indexCount = 0;
        TextureIds textureIndices;
    };

    struct PipelineDrawInfo {
        UInt32 index = 0, drawCount = 0;
        vk::PipelineLayout layout;
    };

    ankerl::unordered_dense::map<vk::Pipeline, PipelineDrawInfo> pipelineMap;

private:
    void present(const std::stop_token& stopToken);
    void startFrame();
    void beginRenderingCommands(const World& world, const Camera& camera);
    void computePrePass(UInt32 drawCount);
    void renderMainPass();
    void renderImGui();
    void startRenderPass(vk::RenderPass pass, std::span<vk::ClearValue> clearValues);
    void endFrame();

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
    void createDescriptorPool();
    void initFrame(Frame& frame, UInt32 frameIndex);
    void writeDescriptors(Frame& frame, UInt32 frameIndex);
    void initImGui();

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

struct UBOData {
    alignas(16) glm::mat4 perspective;
    alignas(16) glm::mat4 orthographic;
    alignas(16) glm::vec3 sunDirection;
    alignas(16) glm::vec3 cameraPosition;
    alignas(16) glm::vec2 resolution;
};

}   // namespace dragonfire