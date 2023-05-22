//
// Created by josh on 5/17/23.
//

#pragma once
#include "allocation.h"
#include "swapchain.h"
#include <renderer.h>

namespace dragonfire {

class VkRenderer : public Renderer {
public:
    void init() override;
    void shutdown() override;

private:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    vk::SampleCountFlagBits msaaSamples;

    struct Queues {
        uint32_t graphicsFamily = 0, presentFamily = 0, transferFamily = 0;
        vk::Queue graphics, present, transfer;
    } queues;

    Swapchain swapchain;
    VmaAllocator allocator;
    Image depthImage, msaaImage;
    vk::ImageView depthView, msaaView;
    vk::RenderPass mainRenderPass;

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

    [[nodiscard]] std::array<vk::DescriptorSetLayout, 4> getDescriptorSetLayouts() const { return {/*TODO*/}; }

    [[nodiscard]] vk::SampleCountFlagBits getSampleCount() const { return msaaSamples; }

    [[nodiscard]] std::vector<vk::RenderPass> getRenderPasses() const { return {mainRenderPass}; }
};

}   // namespace dragonfire