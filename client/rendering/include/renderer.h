//
// Created by josh on 11/9/22.
//

#pragma once
#include "../src/allocation.h"
#include "../src/swapchain.h"
#include <SDL_video.h>

namespace df {
class Renderer {
    friend class Swapchainp;
public:
    static Renderer* getOrInit(SDL_Window* window) noexcept;

    void shutdown() noexcept;
    static constexpr SDL_WindowFlags SDL_WINDOW_FLAGS = SDL_WINDOW_VULKAN;
private:
    std::shared_ptr<spdlog::logger> logger;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    Swapchain swapchain;

    struct Queues {
        UInt graphicsFamily = 0, presentFamily = 0, transferFamily = 0;
        vk::Queue graphics, present, transfer;
        bool getFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    }queues;

private:
    static Renderer rendererInstance;
    void init(SDL_Window* window, bool validation);
    void createInstance(SDL_Window* window, bool validation);
    void getPhysicalDevice();
    void createDevice();
    void initAllocator() const;

};

}   // namespace df