//
// Created by josh on 5/17/23.
//

#include "vk_renderer.h"
#include "mesh.h"
#include "renderer.h"
#include <ankerl/unordered_dense.h>
#include <transform.h>
#include <world.h>

namespace dragonfire {

void VkRenderer::render(World& world, const Camera& camera)
{
    startFrame();
    beginRenderingCommands(camera);
    Frame& frame = getCurrentFrame();
    DrawData* drawData = static_cast<DrawData*>(frame.drawData.getInfo().pMappedData);

    pipelineMap.clear();
    entt::registry& registry = world.getRegistry();
    using namespace entt::literals;

    UInt32 pipelineCount = 0, drawCount = 0;
    auto group = registry.group<Model*, Transform>({}, entt::exclude<entt::tag<"invisible"_hs>>);
    for (auto&& [entity, model, transform] : group.each()) {
        for (auto& primitive : model->getPrimitives()) {
            if (drawCount >= maxDrawCount) {
                logger->error("Max draw count exceeded, some models may not be drawn");
                break;
            }
            VkMaterial* material = static_cast<VkMaterial*>(primitive.material);
            vk::Pipeline pipeline = material->getPipeline();
            UInt32 pipelineIndex;
            if (pipelineMap.contains(pipeline)) {
                auto& info = pipelineMap[pipeline];
                pipelineIndex = info.index;
                info.drawCount++;
            }
            else {
                auto& info = pipelineMap[pipeline];
                info.index = pipelineIndex = pipelineCount++;
                info.layout = material->getPipelineLayout();
                info.drawCount = 1;
            }
            drawData[drawCount].transform = transform.toMatrix() * primitive.transform;
            Mesh* mesh = reinterpret_cast<Mesh*>(primitive.mesh);
            drawData[drawCount].vertexOffset = mesh->getVertexOffset();
            drawData[drawCount].vertexCount = mesh->vertexCount;
            drawData[drawCount].indexOffset = mesh->getIndexOffset();
            drawData[drawCount].indexCount = mesh->indexCount;
            drawData++;
            drawCount++;
        }
    }

    computePrePass(drawCount);
    renderMainPass();

    endFrame();
}

void VkRenderer::beginRenderingCommands(const Camera& camera)
{
    char* ptr = static_cast<char*>(globalUBO.getInfo().pMappedData);
    ptr += uboOffset * (frameCount % FRAMES_IN_FLIGHT);
    UBOData* data = reinterpret_cast<UBOData*>(ptr);
    glm::mat4 view = camera.getViewMatrix();
    data->orthographic = camera.ortho * view;
    data->perspective = camera.perspective * view;

    Frame& frame = getCurrentFrame();
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
                      | vk::CommandBufferUsageFlagBits::eRenderPassContinue;
    frame.cmd.begin(beginInfo);
}

void VkRenderer::computePrePass(UInt32 drawCount)
{
    Frame& frame = getCurrentFrame();
    vk::CommandBuffer cmd = frame.cmd;
    cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            cullComputeLayout,
            0,
            {frame.globalDescriptorSet, frame.computeSet},
            {}
    );
    UInt32 baseIndex = 0;
    for (auto& [pipeline, info] : pipelineMap) {
        if (info.drawCount == 0)
            continue;
        UInt32 pushConstants[] = {baseIndex, info.index, drawCount};
        cmd.pushConstants(cullComputeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(UInt32) * 2, pushConstants);
        cmd.dispatch(std::max(drawCount / 256u, 1u), 1, 1);
        baseIndex += info.drawCount;
    }

    vk::BufferMemoryBarrier commands{}, count{};
    commands.buffer = frame.commandBuffer;
    commands.size = frame.commandBuffer.getInfo().size;
    count.buffer = frame.countBuffer;
    count.size = frame.countBuffer.getInfo().size;
    commands.offset = count.offset = 0;
    commands.srcAccessMask = count.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    commands.dstAccessMask = count.dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead;
    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eDrawIndirect,
            {},
            {},
            {commands, count},
            {}
    );
}

void VkRenderer::renderMainPass()
{
    Frame& frame = getCurrentFrame();
    vk::ClearValue clearValues[2];
    clearValues[0].color = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    startRenderPass(mainRenderPass, clearValues);
    vk::CommandBuffer cmd = frame.cmd;

    vk::Viewport viewport{};
    viewport.x = viewport.y = 0.0f;
    viewport.width = float(swapchain.getExtent().width);
    viewport.height = float(swapchain.getExtent().height);
    viewport.maxDepth = 1.0f;
    viewport.minDepth = 0.0f;
    cmd.setViewport(0, viewport);

    vk::Rect2D scissor{};
    scissor.extent = swapchain.getExtent();
    cmd.setScissor(0, scissor);

    meshRegistry.bindBuffers(cmd);
    vk::DeviceSize drawOffset = 0;
    for (auto& [pipeline, info] : pipelineMap) {
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, info.layout, 0, {frame.globalDescriptorSet}, {});
        cmd.drawIndexedIndirectCount(
                frame.commandBuffer,
                drawOffset,
                frame.countBuffer,
                info.index * sizeof(UInt32),
                maxDrawCount,
                0
        );
        drawOffset += info.drawCount * sizeof(vk::DrawIndexedIndirectCommand);
    }
    cmd.endRenderPass();
}

void VkRenderer::startRenderPass(vk::RenderPass pass, std::span<vk::ClearValue> clearValues)
{
    vk::RenderPassBeginInfo passBeginInfo{};
    passBeginInfo.framebuffer = swapchain.getCurrentFramebuffer();
    passBeginInfo.renderArea.extent = swapchain.getExtent();
    passBeginInfo.renderPass = pass;
    passBeginInfo.clearValueCount = clearValues.size();
    passBeginInfo.pClearValues = clearValues.data();
    getCurrentFrame().cmd.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
}

void VkRenderer::startFrame()
{
    std::unique_lock lock(presentData.mutex);
    if (presentingFrame)
        presentData.condVar.wait(lock, [&] { return presentingFrame == nullptr; });
    vk::Result presentResult = presentData.result;
    lock.unlock();
    Frame& frame = getCurrentFrame();
    if (device.waitForFences(frame.fence, true, UINT64_MAX) != vk::Result::eSuccess)
        logger->error("Fence wait failed, attempting to continue, but things may break");

    UInt retries = 0;
    do {
        if (retries > 10)
            crash("Swapchain recreation failed, maximum number of retries exceeded");
        switch (presentResult) {
            case vk::Result::eSuccess: break;
            case vk::Result::eSuboptimalKHR:
            case vk::Result::eErrorOutOfDateKHR:
                swapchain = Swapchain(
                        physicalDevice,
                        device,
                        window,
                        surface,
                        queues.graphicsFamily,
                        queues.presentFamily,
                        swapchain
                );
                break;
            default: crash("Failed to acquire next swapchain image");
        }
        presentResult = swapchain.next(frame.presentSemaphore);
        retries++;
    } while (presentResult != vk::Result::eSuccess);

    device.resetFences(frame.fence);
    device.resetCommandPool(frame.pool);
}

void VkRenderer::endFrame()
{
    getCurrentFrame().cmd.end();
    std::unique_lock lock(presentData.mutex);
    presentingFrame = &getCurrentFrame();
    lock.unlock();
    presentData.condVar.notify_one();
    frameCount++;
}

void VkRenderer::present(const std::stop_token& stopToken)
{
    while (!stopToken.stop_requested()) {
        std::unique_lock lock(presentData.mutex);
        presentData.condVar.wait(lock, stopToken, [&] { return presentingFrame != nullptr; });
        if (stopToken.stop_requested())
            break;
        vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo{};
        submitInfo.pCommandBuffers = &presentingFrame->cmd;
        submitInfo.commandBufferCount = 1;
        submitInfo.pWaitSemaphores = &presentingFrame->renderSemaphore;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &presentingFrame->presentSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pWaitDstStageMask = &mask;

        queues.graphics.submit(submitInfo, presentingFrame->fence);

        UInt32 index = swapchain.getCurrentImageIndex();
        vk::SwapchainKHR chain = swapchain;
        vk::PresentInfoKHR presentInfo{};
        presentInfo.pWaitSemaphores = &presentingFrame->presentSemaphore;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pSwapchains = &chain;
        presentInfo.swapchainCount = 1;
        presentInfo.pImageIndices = &index;

        presentData.result = queues.present.presentKHR(&presentInfo);
        presentingFrame = nullptr;
        lock.unlock();
        presentData.condVar.notify_one();
    }
    queues.present.waitIdle();
    logger->info("Presentation thread destroyed");
}

MeshHandle VkRenderer::createMesh(std::span<Model::Vertex> vertices, std::span<UInt32> indices)
{
    return meshRegistry.createMesh(vertices, indices);
}

void VkRenderer::freeMesh(MeshHandle mesh)
{
    // TODO
    meshRegistry.freeMesh(mesh);
}

Material::Library* VkRenderer::getMaterialLibrary()
{
    return &materialLibrary;
}

void VkRenderer::shutdown()
{
    if (!instance)
        return;
    presentData.thread.request_stop();
    presentData.thread.join();
    device.waitIdle();
    for (Frame& frame : frames) {
        frame.drawData.destroy();
        frame.culledMatrices.destroy();
        device.destroy(frame.pool);
    }

    materialLibrary.destroy();
    layoutManager.destroy();
    meshRegistry.destroy();

    globalUBO.destroy();
    device.destroy(cullComputePipeline);
    device.destroy(cullComputeLayout);
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