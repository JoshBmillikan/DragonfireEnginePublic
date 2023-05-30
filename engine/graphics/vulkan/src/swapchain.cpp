//
// Created by josh on 5/18/23.
//

#include "swapchain.h"
#include "config.h"
#include <SDL2/SDL_vulkan.h>

namespace dragonfire {

/***
 * @brief Checks if the preferred surface format is supported and returns it,
 * or returns the first supported format if the preferred target is not supported.
 * @param physicalDevice physical device handle
 * @param surface surface handle
 * @param logger logger to log to
 * @return The surface format
 */
static vk::SurfaceFormatKHR getSurfaceFormat(
        vk::PhysicalDevice physicalDevice,
        vk::SurfaceKHR surface,
        spdlog::logger* logger
);

/***
 * @brief Checks if the preferred presentation mode is supported.
 * Defaults to FIFO if it's not supported.
 * @param physicalDevice physical device handle
 * @param surface surface in use
 * @param vsync true if a vsync mode(mailbox) should be preferred
 * @param logger logger to log to
 * @return the presentation mode
 */
static vk::PresentModeKHR getPresentMode(
        vk::PhysicalDevice physicalDevice,
        vk::SurfaceKHR surface,
        spdlog::logger* logger
) noexcept;

Swapchain::Swapchain(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        SDL_Window* window,
        vk::SurfaceKHR surface,
        UInt32 graphicsFamily,
        UInt32 presentFamily,
        vk::SwapchainKHR old
)
    : device(device)
{
    logger = spdlog::get("Rendering");
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    extent = capabilities.currentExtent;
    if (extent.width == UINT32_MAX && extent.height == UINT32_MAX) {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        extent.width = width;
        extent.height = height;
    }

    UInt32 minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0)
        minImageCount = std::min(minImageCount, capabilities.maxImageCount);

    format = getSurfaceFormat(physicalDevice, surface, logger.get()).format;
    vk::PresentModeKHR presentMode = getPresentMode(physicalDevice, surface, logger.get());
    vk::SharingMode shareMode = graphicsFamily == presentFamily ? vk::SharingMode::eExclusive
                                                                : vk::SharingMode::eConcurrent;
    UInt32 queueIndices[] = {graphicsFamily, presentFamily};

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface;
    createInfo.minImageCount = minImageCount;
    createInfo.imageFormat = format;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageSharingMode = shareMode;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueIndices;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.oldSwapchain = old;

    swapchain = device.createSwapchainKHR(createInfo);
    UInt i;
    try {
        vk::resultCheck(
                device.getSwapchainImagesKHR(swapchain, &imageCount, nullptr),
                "Failed to get swapchain image count"
        );
        images = new vk::Image[imageCount];
        vk::resultCheck(device.getSwapchainImagesKHR(swapchain, &imageCount, images), "Failed to get swapchain images");

        views = new vk::ImageView[imageCount]();
        for (i = 0; i < imageCount; i++) {
            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.format = format;
            viewInfo.viewType = vk::ImageViewType::e2D;
            viewInfo.image = images[i];
            viewInfo.components.r = vk::ComponentSwizzle::eIdentity;
            viewInfo.components.g = vk::ComponentSwizzle::eIdentity;
            viewInfo.components.b = vk::ComponentSwizzle::eIdentity;
            viewInfo.components.a = vk::ComponentSwizzle::eIdentity;
            viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            views[i] = device.createImageView(viewInfo);
        }
    }
    catch (...) {
        if (views) {
            for (UInt j = 0; j < i; j++)
                device.destroy(views[j]);
            delete[] views;
        }
        delete[] images;
        device.destroy(swapchain);
        logger->error("Swapchain creation failed");
        throw;
    }
    logger->info("Created swapchain with {} images of extent {}x{}", imageCount, extent.width, extent.height);
}

void Swapchain::initFramebuffers(vk::RenderPass renderPass, vk::ImageView msaaView, vk::ImageView depthView)
{
    assert(framebuffers == nullptr);
    framebuffers = new vk::Framebuffer[imageCount];
    UInt i;
    try {
        for (i = 0; i < imageCount; i++) {
            vk::ImageView attachments[] = {msaaView, depthView, views[i]};
            vk::FramebufferCreateInfo createInfo{};
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = msaaView ? 3 : 2;
            createInfo.pAttachments = attachments;
            createInfo.width = extent.width;
            createInfo.height = extent.height;
            createInfo.layers = 1;

            framebuffers[i] = device.createFramebuffer(createInfo);
        }
    }
    catch (...) {
        for (UInt j = 0; j < i; j++)
            device.destroy(framebuffers[j]);
        delete[] framebuffers;
        framebuffers = nullptr;
        throw;
    }
}

void Swapchain::destroy()
{
    if (swapchain) {
        for (UInt i = 0; i < imageCount; i++) {
            if (views)
                device.destroy(views[i]);
            if (framebuffers)
                device.destroy(framebuffers[i]);
        }
        device.destroy(swapchain);
        delete[] framebuffers;
        delete[] views;
        delete[] images;
        swapchain = nullptr;
        logger->info("Swapchain destroyed");
    }
}

vk::Result Swapchain::next(vk::Semaphore semaphore, vk::Fence fence) noexcept
{
    auto [result, index] = device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, fence);
    if (result == vk::Result::eSuccess)
        currentImage = index;
    return result;
}

Swapchain::Swapchain(Swapchain&& other) noexcept
{
    if (this != &other) {
        destroy();
        swapchain = other.swapchain;
        other.swapchain = nullptr;
        device = other.device;
        extent = other.extent;
        format = other.format;
        imageCount = other.imageCount;
        currentImage = other.currentImage;
        images = other.images;
        views = other.views;
        framebuffers = other.framebuffers;
        logger = other.logger;
    }
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
{
    if (this != &other) {
        destroy();
        swapchain = other.swapchain;
        other.swapchain = nullptr;
        device = other.device;
        extent = other.extent;
        format = other.format;
        imageCount = other.imageCount;
        currentImage = other.currentImage;
        images = other.images;
        views = other.views;
        framebuffers = other.framebuffers;
        logger = other.logger;
    }
    return *this;
}

vk::SurfaceFormatKHR getSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, spdlog::logger* logger)
{
    auto formats = physicalDevice.getSurfaceFormatsKHR<FrameAllocator<vk::SurfaceFormatKHR>>(surface);

    if (formats.empty())
        throw std::runtime_error("No surface formats found");

    for (vk::SurfaceFormatKHR fmt : formats) {
        if (fmt.format == vk::Format::eB8G8R8A8Srgb && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return fmt;
    }

    for (vk::SurfaceFormatKHR fmt : formats) {
        if (fmt.format == vk::Format::eR8G8B8A8Srgb && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return fmt;
    }

    for (vk::SurfaceFormatKHR fmt : formats) {
        if (fmt.format == vk::Format::eB8G8R8A8Unorm && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return fmt;
    }

    logger->warn("Preferred surface format not found, falling back to first supported format");
    return formats[0];
}

vk::PresentModeKHR getPresentMode(
        vk::PhysicalDevice physicalDevice,
        vk::SurfaceKHR surface,
        spdlog::logger* logger
) noexcept
{
    const bool vsync = Config::INSTANCE.get<bool>("graphics.vsync");
    vk::PresentModeKHR target = vsync ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate;
    std::vector<vk::PresentModeKHR, FrameAllocator<vk::PresentModeKHR>> modes;
    try {
        modes = physicalDevice.getSurfacePresentModesKHR<FrameAllocator<vk::PresentModeKHR>>(surface);
    }
    catch (...) {
        logger->error("Failed to get surface presentation modes");
        return vk::PresentModeKHR::eFifo;
    }
    for (vk::PresentModeKHR mode : modes) {
        if (mode == target)
            return target;
    }

    logger->warn("Requested presentation mode not available, falling back to FIFO");
    return vk::PresentModeKHR::eFifo;
}
}   // namespace dragonfire