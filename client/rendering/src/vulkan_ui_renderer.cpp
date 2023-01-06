//
// Created by josh on 1/3/23.
//

#include "vulkan_ui_renderer.h"
#include "renderer.h"

namespace df {
VulkanUIRenderer::VulkanUIRenderer(Renderer* renderer) : ui::UIRenderer()
{
    device = renderer->device;
    extent = renderer->swapchain.getExtent();

    vk::CommandPoolCreateInfo createInfo{};
    createInfo.queueFamilyIndex = renderer->queues.graphicsFamily;
    pool = device.createCommandPool(createInfo);

    vk::CommandBuffer buffers[2];
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandBufferCount = 2;
    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::resultCheck(device.allocateCommandBuffers(&allocInfo, buffers), "Failed to allocate command buffers");
    uiCmds = buffers[0];
    transferCmds = buffers[1];

    createImageBuffers();
}

VulkanUIRenderer::~VulkanUIRenderer()
{
    device.destroy(uiView);
    device.destroy(pool);
}

void VulkanUIRenderer::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    rect = CefRect(0, 0, (int) extent.width, (int) extent.height);
}

void VulkanUIRenderer::OnPaint(
        CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList& dirtyRects,
        const void* buffer,
        int width,
        int height
)
{
    memcpy(uiBuffer.getInfo().pMappedData, buffer, width * height * 4);
    needsUpdate = true;
}

void VulkanUIRenderer::createImageBuffers()
{
    vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.extent = vk::Extent3D(extent, 1);
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = vk::Format::eR8G8B8A8Srgb;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    uiImage = Image(imageCreateInfo, imageAllocInfo);
    vk::ImageSubresourceRange imageSubRange;
    imageSubRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageSubRange.layerCount = 1;
    imageSubRange.levelCount = 1;
    imageSubRange.baseArrayLayer = 0;
    imageSubRange.baseMipLevel = 0;
    uiView = uiImage.createView(device, imageSubRange);

    vk::DeviceSize size = extent.width * extent.height * 4;
    vk::BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

    VmaAllocationCreateInfo bufferAllocInfo{};
    bufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    bufferAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    uiBuffer = Buffer(bufferCreateInfo, bufferAllocInfo);
}
}   // namespace df