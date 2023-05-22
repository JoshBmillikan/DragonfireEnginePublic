//
// Created by josh on 5/17/23.
//

#include "vk_renderer.h"
#include "renderer.h"

namespace dragonfire {

void VkRenderer::shutdown()
{
    if (!instance)
        return;
    device.waitIdle();

    device.destroy(mainRenderPass);
    device.destroy(msaaView);
    msaaImage.destroy();
    device.destroy(depthView);
    depthImage.destroy();
    vmaDestroyAllocator(allocator);
    swapchain.destroy();
    device.destroy();
    instance.destroy(surface);
    if (debugMessenger)
        instance.destroy(debugMessenger);
    instance.destroy();
    logger->info("Vulkan renderer shutdown successful");
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkRenderer::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
)
{
    auto logger = reinterpret_cast<spdlog::logger*>(pUserData);
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            logger->error("Validation Layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            logger->warn("Validation Layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            logger->info("Validation Layer: {}", pCallbackData->pMessage);
            break;
        default: logger->trace("Validation Layer: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

static VkRenderer RENDERER;

Renderer* getRenderer()
{
    return &RENDERER;
}

}   // namespace dragonfire

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE