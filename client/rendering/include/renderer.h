//
// Created by josh on 11/9/22.
//

#pragma once
#include "../src/allocation.h"
#include "../src/swapchain.h"
#include <SDL_video.h>
#include <condition_variable>
#include <mutex>

namespace df {
class Renderer {
    friend class Swapchainp;

public:
    Renderer() noexcept = default;
    static Renderer* getOrInit(SDL_Window* window) noexcept;

    void shutdown() noexcept;
    static constexpr SDL_WindowFlags SDL_WINDOW_FLAGS = SDL_WINDOW_VULKAN;
    static constexpr Size FRAMES_IN_FLIGHT = 2;

private:
    ULong frameCount = 0;
    std::shared_ptr<spdlog::logger> logger;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    Swapchain swapchain;
    Image depthImage;
    vk::ImageView depthView;

    std::mutex presentLock;
    std::condition_variable_any presentCond;
    std::jthread presentThreadHandle;

    struct Queues {
        UInt graphicsFamily = 0, presentFamily = 0, transferFamily = 0;
        vk::Queue graphics, present, transfer;
        bool getFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    } queues;

    struct Frame {
        vk::CommandPool pool;
        vk::CommandBuffer buffer;
        vk::Fence fence;
        vk::Semaphore renderSemaphore, presentSemaphore;
    } frames[FRAMES_IN_FLIGHT], *presentingFrame = nullptr;

private:
    static Renderer rendererInstance;
    Frame& getCurrentFrame() noexcept { return frames[frameCount % FRAMES_IN_FLIGHT]; }
    [[nodiscard]] bool depthHasStencil() const noexcept;
    void presentThread(const std::stop_token& token);

    void init(SDL_Window* window, bool validation);
    void createInstance(SDL_Window* window, bool validation);
    void getPhysicalDevice();
    void createDevice();
    void initAllocator() const;
    void createDepthImage();
    void createFrames();
};

}   // namespace df