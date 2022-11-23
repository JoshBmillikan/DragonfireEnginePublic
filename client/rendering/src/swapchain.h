//
// Created by josh on 11/10/22.
//

#pragma once
#include "vulkan_includes.h"
#include <SDL_vulkan.h>

namespace df {

class Swapchain {
public:
    Swapchain() = default;
    Swapchain(
            vk::PhysicalDevice physicalDevice,
            vk::Device device,
            SDL_Window* window,
            vk::SurfaceKHR surface,
            UInt graphicsIndex,
            UInt presentIndex,
            bool vsync = true,
            vk::SwapchainKHR old = nullptr
    );
    DF_NO_COPY(Swapchain);
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;
    ~Swapchain() noexcept;

    /**
     * Acquire the next swapchain image
     * @param semaphore semaphore to signal
     * @return the result of acquiring the image
     */
    [[nodiscard]] vk::Result next(vk::Semaphore semaphore);

    /**
     * Destroy the swapchain
     */
    void destroy() noexcept;
    operator vk::SwapchainKHR() noexcept { return swapchain; }
    [[nodiscard]] UInt getCurrentImageIndex() const noexcept { return currentImageIndex; }
    [[nodiscard]] vk::Format getFormat() const noexcept { return format; }
    [[nodiscard]] const vk::Extent2D& getExtent() const noexcept { return extent; }
    [[nodiscard]] vk::Image getCurrentImage() const noexcept { return images[currentImageIndex]; }
    [[nodiscard]] vk::ImageView getCurrentView() const noexcept { return views[currentImageIndex]; }

private:
    vk::SwapchainKHR swapchain;
    UInt imageCount = 0, currentImageIndex = 0;
    vk::Image* images = nullptr;
    vk::ImageView* views = nullptr;
    vk::Format format{};
    vk::Extent2D extent;
    vk::Device device;
};

}   // namespace df