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
    Swapchain swapchain;
    VmaAllocator allocator;

    struct Queues {
        uint32_t graphicsFamily = 0, presentFamily = 0, transferFamily = 0;
        vk::Queue graphics, present, transfer;
    } queues;

private:
    void createInstance(bool validation);
    void getPhysicalDevice();
    bool getQueueFamilies(vk::PhysicalDevice pDevice) noexcept;
    void createDevice();
    void createGpuAllocator();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    );
};

}   // namespace dragonfire