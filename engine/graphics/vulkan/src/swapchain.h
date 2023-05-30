//
// Created by josh on 5/18/23.
//

#pragma once

#include <SDL2/SDL_video.h>

namespace dragonfire {

class Swapchain {
public:
    /***
     * @brief Default constructs the swapchain.
     * This swapchain cannot be used to render, but having a default constructor
     * simplifies initialization logic in the renderer.
     */
    Swapchain() = default;

    /***
     * @brief Constructs the swapchain
     * @param physicalDevice physical device handle in use
     * @param device logical device handle in use
     * @param window window in use
     * @param surface surface to draw to
     * @param graphicsFamily queue family of the graphics queue
     * @param presentFamily queue family of the presentation queue
     * @param vsync true to enable vsync, false to use a presentation mode without it
     * @param logger logger to log to
     * @param old old swapchain handle to replace, can be null
     */
    Swapchain(
            vk::PhysicalDevice physicalDevice,
            vk::Device device,
            SDL_Window* window,
            vk::SurfaceKHR surface,
            UInt32 graphicsFamily,
            UInt32 presentFamily,
            vk::SwapchainKHR old = nullptr
    );
    /***
     * @brief Initializes the swapchain framebuffers. The swapchain framebuffers depend
     * on the render pass, but the render pass depends on the swapchain, so initialization of
     * the framebuffers must be deferred until after render pass creation instead of the Swapchain's
     * constructor.
     * @param renderPass Render pass used with the framebuffers;
     * @param msaaView msaa sampling image view
     * @param depthView depth image view
     */
    void initFramebuffers(vk::RenderPass renderPass, vk::ImageView msaaView, vk::ImageView depthView);

    /// Destroy the swapchain and associated resources
    void destroy();

    ~Swapchain() noexcept { destroy(); }

    [[nodiscard]] vk::SwapchainKHR getSwapchain() const { return swapchain; }

    [[nodiscard]] vk::Extent2D getExtent() const { return extent; }

    [[nodiscard]] vk::Format getFormat() const { return format; }

    [[nodiscard]] UInt32 getCurrentImageIndex() const { return currentImage; }

    [[nodiscard]] vk::Image getCurrentImage() const { return images[currentImage]; }

    [[nodiscard]] vk::ImageView getCurrentView() const { return views[currentImage]; }

    [[nodiscard]] vk::Framebuffer getCurrentFramebuffer() const { return framebuffers[currentImage]; }

    operator vk::SwapchainKHR() const { return swapchain; }

    vk::Result next(vk::Semaphore semaphore = {}, vk::Fence fence = {}) noexcept;

    Swapchain(Swapchain&) = delete;
    Swapchain& operator=(Swapchain&) = delete;
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

private:
    std::shared_ptr<spdlog::logger> logger;
    vk::Device device;
    vk::SwapchainKHR swapchain = nullptr;
    vk::Image* images = nullptr;
    vk::ImageView* views = nullptr;
    vk::Framebuffer* framebuffers = nullptr;
    UInt32 imageCount = 0, currentImage = 0;
    vk::Extent2D extent;
    vk::Format format{};
};

}   // namespace dragonfire