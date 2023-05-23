//
// Created by josh on 5/17/23.
//

#pragma once
#include "allocation.h"
#include "descriptor_set.h"
#include "swapchain.h"
#include "vk_material.h"
#include <renderer.h>

namespace dragonfire {

class VkRenderer : public Renderer {
public:
    void init() override;
    void shutdown() override;
    Material::Library* getMaterialLibrary() override;

    static constexpr USize FRAMES_IN_FLIGHT = 2;

private:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    vk::SampleCountFlagBits msaaSamples;

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

    struct Frame {
        vk::CommandPool pool;
        vk::CommandBuffer cmd;
    } frames[FRAMES_IN_FLIGHT], *currentFrame = nullptr;

private:
    void createInstance(bool validation);
    void getPhysicalDevice();
    bool getQueueFamilies(vk::PhysicalDevice pDevice) noexcept;
    void createDevice();
    void createGpuAllocator();
    void createDepthImage();
    void createMsaaImage();
    void createRenderPass();

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

}   // namespace dragonfire