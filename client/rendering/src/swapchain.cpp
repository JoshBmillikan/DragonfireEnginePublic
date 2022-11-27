//
// Created by josh on 11/10/22.
//

#include "swapchain.h"
#include "renderer.h"
#include <alloca.h>
#include <config.h>

namespace df {
static vk::PresentModeKHR getPresentMode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool vsync);
static vk::SurfaceFormatKHR getSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

Swapchain::Swapchain(Renderer* renderer, SDL_Window* window)
    : Swapchain(
            renderer->physicalDevice,
            renderer->device,
            window,
            renderer->surface,
            renderer->queues.graphicsFamily,
            renderer->queues.presentFamily,
            Config::get().graphics.vsync,
            renderer->swapchain.swapchain
    )
{
}

Swapchain::Swapchain(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        SDL_Window* window,
        vk::SurfaceKHR surface,
        UInt graphicsIndex,
        UInt presentIndex,
        bool vsync,
        vk::SwapchainKHR old
)
    : device(device)
{
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    extent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == UINT32_MAX && capabilities.currentExtent.height == UINT32_MAX) {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        extent.width = width;
        extent.height = height;
    }
    format = getSurfaceFormat(physicalDevice, surface).format;
    auto presentMode = getPresentMode(physicalDevice, surface, vsync);
    UInt queueIndices[] = {graphicsIndex, presentIndex};

    UInt minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount)
        minImageCount = capabilities.maxImageCount;

    vk::SharingMode shareMode = graphicsIndex == presentIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;

    vk::SwapchainCreateInfoKHR createInfo;
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

    vk::resultCheck(device.getSwapchainImagesKHR(swapchain, &imageCount, nullptr), "Failed to get swapchain images");
    images = new vk::Image[imageCount];
    UInt i = 0;
    try {
        views = new vk::ImageView[imageCount];
        vk::resultCheck(device.getSwapchainImagesKHR(swapchain, &imageCount, images), "Failed to get swapchain images");
        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;
        for (; i < imageCount; i++) {
            vk::ImageViewCreateInfo viewCreateInfo;
            viewCreateInfo.image = images[i];
            viewCreateInfo.viewType = vk::ImageViewType::e2D;
            viewCreateInfo.format = format;
            viewCreateInfo.subresourceRange = subresourceRange;
            viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
            viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
            viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
            viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
            views[i] = device.createImageView(viewCreateInfo);
        }
    }
    catch (...) {
        for (UInt j = 0; j < i; j++)
            device.destroy(views[j]);
        delete[] images;
        delete[] views;
        device.destroy(swapchain);
        throw;
    }
}

void Swapchain::createFramebuffers(vk::RenderPass renderPass, vk::ImageView depthView, vk::ImageView msaaView)
{
    framebuffers = new vk::Framebuffer[imageCount];
    UInt i;
    try {
        for (i = 0; i < imageCount; i++) {
            vk::ImageView attachments[] = {msaaView, depthView, views[i]};
            vk::FramebufferCreateInfo framebufferCreateInfo;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = msaaView ? 3 : 2;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = extent.width;
            framebufferCreateInfo.height = extent.height;
            framebufferCreateInfo.layers = 1;

            framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
        }
    } catch (...) {
        for (UInt j=0; j<i;j++)
            device.destroy(framebuffers[j]);
        throw;
    }
}

vk::Result Swapchain::next(vk::Semaphore semaphore)
{
    auto [result, index] = device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore);
    currentImageIndex = index;
    return result;
}

void Swapchain::destroy() noexcept
{
    if (swapchain) {
        for (UInt i = 0; i < imageCount; i++) {
            device.destroy(views[i]);
            if (framebuffers)
                device.destroy(framebuffers[i]);
        }
        delete[] images;
        delete[] views;
        delete[] framebuffers;
        device.destroy(swapchain);
        swapchain = nullptr;
    }
}

vk::SurfaceFormatKHR getSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
    UInt count;
    vk::SurfaceFormatKHR* formats = nullptr;
    vk::resultCheck(physicalDevice.getSurfaceFormatsKHR(surface, &count, formats), "Failed to get surface formats");
    if (count == 0)
        throw std::runtime_error("No surface formats found");
    formats = (vk::SurfaceFormatKHR*) alloca(sizeof(vk::SurfaceFormatKHR) * count);
    vk::resultCheck(physicalDevice.getSurfaceFormatsKHR(surface, &count, formats), "Failed to get surface formats");

    for (UInt i = 0; i < count; i++) {
        if (formats[i].format == vk::Format::eB8G8R8A8Srgb && formats[i].colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return formats[i];
    }

    spdlog::get("Rendering")->warn("Preferred surface format not found");
    return formats[0];
}

vk::PresentModeKHR getPresentMode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool vsync)
{
    vk::PresentModeKHR target = vsync ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate;
    UInt count;
    vk::PresentModeKHR* modes = nullptr;
    vk::resultCheck(physicalDevice.getSurfacePresentModesKHR(surface, &count, modes), "Failed to get surface present modes");
    modes = (vk::PresentModeKHR*) alloca(sizeof(vk::PresentModeKHR) * count);
    vk::resultCheck(physicalDevice.getSurfacePresentModesKHR(surface, &count, modes), "Failed to get surface present modes");

    for (UInt i = 0; i < count; i++) {
        if (modes[i] == target)
            return target;
    }

    spdlog::get("Rendering")->warn("Requested presentation mode not available, falling back to FIFO");
    return vk::PresentModeKHR::eFifo;
}

Swapchain::Swapchain(Swapchain&& other) noexcept
{
    if (this != &other) {
        destroy();
        swapchain = other.swapchain;
        images = other.images;
        views = other.views;
        imageCount = other.imageCount;
        currentImageIndex = other.currentImageIndex;
        format = other.format;
        extent = other.extent;
        device = other.device;
        other.swapchain = nullptr;
    }
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
{
    if (this != &other) {
        destroy();
        swapchain = other.swapchain;
        images = other.images;
        views = other.views;
        imageCount = other.imageCount;
        currentImageIndex = other.currentImageIndex;
        format = other.format;
        extent = other.extent;
        device = other.device;
        other.swapchain = nullptr;
    }
    return *this;
}

Swapchain::~Swapchain() noexcept
{
    if (swapchain) {
        for (UInt i = 0; i < imageCount; i++)
            device.destroy(views[i]);
        delete[] images;
        delete[] views;
        device.destroy(swapchain);
        swapchain = nullptr;
    }
}
}   // namespace df