//
// Created by josh on 11/9/22.
//

#include "renderer.h"
#include "config.h"
#include "material.h"
#include <SDL_vulkan.h>
#include <backends/imgui_impl_vulkan.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace df {

void Renderer::beginRendering(const Camera& camera)
{
    std::unique_lock lock(presentLock);
    presentCond.wait(lock, [&] { return presentingFrame == nullptr; });
    vk::Result lastResult = presentResult;
    lock.unlock();
    Frame& frame = getCurrentFrame();

    if (device.waitForFences(frame.fence, true, UINT64_MAX) != vk::Result::eSuccess)
        logger->critical("Render fence wait failed, things  may break");
    UInt retryCount = 0;
    do {
        if (lastResult == vk::Result::eErrorOutOfDateKHR || lastResult == vk::Result::eSuboptimalKHR)
            recreateSwapchain();
        else if (lastResult != vk::Result::eSuccess)
            vk::throwResultException(lastResult, "Swapchain presentation/acquire error");
        lastResult = swapchain.next(frame.presentSemaphore);
        retryCount++;
    } while (lastResult != vk::Result::eSuccess && retryCount < 3);
    if (lastResult != vk::Result::eSuccess)
        crash("Failed to resize swapchain");

    char* ptr = reinterpret_cast<char*>(globalUniformBuffer.getInfo().pMappedData);
    ptr += globalUniformOffset * (frameCount % FRAMES_IN_FLIGHT);
    UboData* ubo = reinterpret_cast<UboData*>(ptr);
    glm::mat4 view = camera.getViewMatrix();
    ubo->viewPerspective = viewPerspective = camera.perspective * view;
    ubo->viewOrthographic = viewOrthographic = camera.orthographic * view;
    ubo->sunAngle = glm::vec3(0, 0, 1);
    ubo->resolution = glm::vec2(swapchain.getExtent().width, swapchain.getExtent().height);

    device.resetFences(frame.fence);
    beginCommandRecording();

    if (threadData == nullptr)
        crash("Renderer was not initialized");
    for (UInt i = 0; i < renderThreadCount; i++) {
        std::unique_lock l(threadData[i].mutex);
        threadData[i].cond.wait(l, [&] { return threadData[i].command == RenderCommand::waiting; });
        threadData[i].command = RenderCommand::begin;
        l.unlock();
        threadData[i].cond.notify_one();
    }
}

void Renderer::render(Model* model, const std::vector<glm::mat4>& matrices)
{
    static UInt threadIndex = 0;
    auto& data = threadData[threadIndex];
    {
        std::unique_lock lock(data.mutex);
        data.cond.wait(lock, [&] { return data.command == RenderCommand::waiting; });
        data.matrices = matrices.data();
        data.matrixCount = matrices.size();
        data.command = RenderCommand::render;
        data.model = model;
        data.uboDescriptorSet = getCurrentFrame().globalDescriptorSet;
        data.bindlessSet = getCurrentFrame().bindlessSet;
    }
    data.cond.notify_one();
    threadIndex = (threadIndex + 1) % renderThreadCount;
}

void Renderer::renderUI()
{
    vk::CommandBuffer cmd = getCurrentFrame().uiBuffer;
    beginSecondaryBuffer(cmd);
    ImDrawData* data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(data, cmd);
    cmd.end();
}

void Renderer::endRendering()
{
    if (threadData == nullptr)
        crash("Renderer was not initialized");
    for (UInt i = 0; i < renderThreadCount; i++) {
        std::unique_lock l(threadData[i].mutex);
        threadData[i].cond.wait(l, [&] { return threadData[i].command == RenderCommand::waiting; });
        threadData[i].command = RenderCommand::end;
        l.unlock();
        threadData[i].cond.notify_one();
    }
    renderBarrier.arrive_and_wait();
    Frame& frame = getCurrentFrame();
    vk::CommandBuffer cmd = frame.buffer;
    cmd.executeCommands(renderThreadCount, secondaryBuffers);
    cmd.executeCommands(frame.uiBuffer);
    cmd.endRenderPass();
    cmd.end();

    std::unique_lock lock(presentLock);
    presentingFrame = &frame;
    lock.unlock();
    presentCond.notify_one();
    frameCount++;
}

static Buffer createInstanceVertexBuffer(UInt capacity);

void Renderer::renderThread(const std::stop_token& token, const UInt threadIndex)
{
    vk::CommandPool pool, pools[FRAMES_IN_FLIGHT];
    vk::CommandBuffer cmd, buffers[FRAMES_IN_FLIGHT];
    for (UInt i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vk::CommandPoolCreateInfo createInfo;
        createInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
        createInfo.queueFamilyIndex = queues.graphicsFamily;
        pools[i] = device.createCommandPool(createInfo);
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandBufferCount = 1;
        allocInfo.level = vk::CommandBufferLevel::eSecondary;
        allocInfo.commandPool = pools[i];
        vk::resultCheck(device.allocateCommandBuffers(&allocInfo, &buffers[i]), "Failed to allocate secondary command buffer");
    }
    Buffer vertexBuffer = createInstanceVertexBuffer(1024);
    vk::DeviceSize bufferOffset = 0;

    if (threadData == nullptr || secondaryBuffers == nullptr)
        crash("Thread data was not initialized before starting render threads");
    RenderThreadData& data = threadData[threadIndex];
    while (!token.stop_requested()) {
        std::unique_lock lock(data.mutex);
        data.cond.wait(lock, token, [&] { return data.command != RenderCommand::waiting; });
        if (token.stop_requested())
            break;
        switch (data.command) {
            case RenderCommand::begin: {
                pool = pools[frameCount % FRAMES_IN_FLIGHT];
                cmd = buffers[frameCount % FRAMES_IN_FLIGHT];
                device.resetCommandPool(pool);
                beginSecondaryBuffer(cmd);
                if (frameCount % FRAMES_IN_FLIGHT == 0)
                    bufferOffset = 0;
            } break;
            case RenderCommand::render: {
                if ((bufferOffset + data.matrixCount) * sizeof(glm::mat4) >= vertexBuffer.getInfo().size) {
                    Buffer newBuffer = createInstanceVertexBuffer(
                            std::max(bufferOffset + data.matrixCount, vertexBuffer.getInfo().size * 2)
                    );
                    void* src = vertexBuffer.getInfo().pMappedData;
                    void* dst = newBuffer.getInfo().pMappedData;
                    memcpy(dst, src, vertexBuffer.getInfo().size);   // TODO: use gpu copy commands instead
                    vertexBuffer = std::move(newBuffer);
                }
                UInt drawCount = 0;
                Model* model = data.model;
                for (UInt i = 0; i < data.matrixCount; i++) {
                    const glm::mat4& matrix = data.matrices[i];
                    if (cullTest(model, matrix)) {
                        glm::mat4* ptr = static_cast<glm::mat4*>(vertexBuffer.getInfo().pMappedData);
                        ptr[bufferOffset + drawCount] = matrix;
                        drawCount++;
                    }
                }
                if (drawCount > 0) {
                    vk::Buffer vertexBuffers[] = {model->getMesh().getBuffer(), vertexBuffer};
                    vk::DeviceSize offsets[] = {0, bufferOffset};
                    vk::Pipeline pipeline = model->getMaterial().getPipeline();
                    vk::PipelineLayout pipelineLayout = model->getMaterial().getLayout();

                    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
                    cmd.bindDescriptorSets(
                            vk::PipelineBindPoint::eGraphics,
                            pipelineLayout,
                            0,
                            {data.uboDescriptorSet, data.bindlessSet},
                            {}
                    );

                    model->getMaterial().pushTextureIndices(cmd);

                    cmd.bindVertexBuffers(0, vertexBuffers, offsets);
                    cmd.bindIndexBuffer(
                            model->getMesh().getBuffer(),
                            model->getMesh().getIndexOffset(),
                            vk::IndexType::eUint32
                    );
                    cmd.drawIndexed(model->getMesh().getIndexCount(), drawCount, 0, 0, 0);
                }
                bufferOffset += drawCount;
            } break;
            case RenderCommand::end:
                cmd.end();
                secondaryBuffers[threadIndex] = cmd;
                cmd = nullptr;
                renderBarrier.arrive_and_wait();
                break;
            default: break;
        }
        data.command = RenderCommand::waiting;
        lock.unlock();
        data.cond.notify_one();
    }
    renderBarrier.arrive_and_wait();
    for (vk::CommandPool p : pools) {
        device.resetCommandPool(p);
        device.destroy(p);
    }
    logger->trace("Render thread {} destroyed", threadIndex);
}

void Renderer::beginSecondaryBuffer(vk::CommandBuffer cmd)
{
    vk::CommandBufferInheritanceInfo inheritanceInfo;
    inheritanceInfo.renderPass = mainPass;
    inheritanceInfo.subpass = 0;
    inheritanceInfo.framebuffer = swapchain.getCurrentFramebuffer();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
                      | vk::CommandBufferUsageFlagBits::eRenderPassContinue;
    cmd.begin(beginInfo);
    vk::Viewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(swapchain.getExtent().width);
    viewport.height = static_cast<float>(swapchain.getExtent().height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    cmd.setViewport(0, viewport);
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D();
    scissor.extent = swapchain.getExtent();
    cmd.setScissor(0, scissor);
}

bool Renderer::cullTest(Model* model, const glm::mat4& matrix)
{
    if (!model->shouldCull())
        return true;

    return true;   // TODO
}

Buffer createInstanceVertexBuffer(UInt capacity)
{
    vk::DeviceSize size = capacity * sizeof(glm::mat4);
    vk::BufferCreateInfo createInfo;
    createInfo.size = size;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    createInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocInfo.priority = 1;
    return Buffer(createInfo, allocInfo);
}

void Renderer::presentThread(const std::stop_token& token)
{
    while (!token.stop_requested()) {
        std::unique_lock lock(presentLock);
        presentCond.wait(lock, token, [&] { return presentingFrame != nullptr; });
        if (token.stop_requested())
            break;
        vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &presentingFrame->presentSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &presentingFrame->renderSemaphore;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &presentingFrame->buffer;
        submitInfo.pWaitDstStageMask = &mask;

        queues.graphics.submit(submitInfo, presentingFrame->fence);

        UInt presentIndex = swapchain.getCurrentImageIndex();
        vk::SwapchainKHR chain = swapchain;
        vk::PresentInfoKHR presentInfo;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &presentingFrame->renderSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &chain;
        presentInfo.pImageIndices = &presentIndex;

        try {
            presentResult = queues.present.presentKHR(presentInfo);
        }
        catch (const vk::OutOfDateKHRError& err) {
            presentResult = vk::Result::eErrorOutOfDateKHR;
        }
        presentingFrame = nullptr;
        lock.unlock();
        presentCond.notify_one();
    }
    queues.present.waitIdle();
    logger->info("Presentation thread destroyed");
}

void Renderer::recreateSwapchain()
{
    device.waitIdle();
    swapchain = Swapchain(this, window);
    logger->info("Resized swapchain to extent {}x{}", swapchain.getExtent().width, swapchain.getExtent().height);
    device.destroy(depthView);
    createDepthImage();
    device.destroy(msaaView);
    createMsaaImage();
    swapchain.createFramebuffers(mainPass, depthView, msaaView);
}

void Renderer::stopRendering()
{
    logger->info("Shutting down render threads");
    threadStop.request_stop();
    presentCond.notify_one();
    presentThreadHandle.join();
    device.waitIdle();
    renderBarrier.arrive_and_drop();
    for (UInt i = 0; threadData != nullptr && i < renderThreadCount; i++) {
        threadData[i].cond.notify_one();
        threadData[i].thread.join();
    }
    delete[] threadData;
    delete[] secondaryBuffers;
    threadData = nullptr;
}

void Renderer::destroy() noexcept
{
    if (instance) {
        if (threadData)
            stopRendering();
        ImGui_ImplVulkan_Shutdown();
        for (Frame& frame : frames) {
            device.destroy(frame.fence);
            device.destroy(frame.renderSemaphore);
            device.destroy(frame.presentSemaphore);
            device.destroy(frame.pool);
        }
        device.destroy(defaultSampler);
        pipelineFactory.destroy();
        device.destroy(descriptorPool);
        device.destroy(globalDescriptorSetLayout);
        device.destroy(bindlessLayout);
        globalUniformBuffer.destroy();
        device.destroy(msaaView);
        msaaImage.destroy();
        device.destroy(depthView);
        depthImage.destroy();
        swapchain.destroy();
        Allocation::shutdownAllocator();
        device.destroy(mainPass);
        device.destroy();
        instance.destroy(surface);
        if (debugMessenger)
            instance.destroy(debugMessenger);
        instance.destroy();
        instance = nullptr;
        logger->info("Renderer destroy successfully");
    }
}

void Renderer::beginCommandRecording()
{
    Frame& frame = getCurrentFrame();
    device.resetCommandPool(frame.pool);
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    frame.buffer.begin(beginInfo);

    vk::RenderPassBeginInfo info;
    info.renderPass = mainPass;
    info.renderArea.extent = swapchain.getExtent();
    info.clearValueCount = 2;

    vk::ClearValue clears[] = {
            (vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})),
            vk::ClearDepthStencilValue(1, 0)};
    info.pClearValues = clears;
    info.framebuffer = swapchain.getCurrentFramebuffer();

    frame.buffer.beginRenderPass(info, vk::SubpassContents::eSecondaryCommandBuffers);
}

void Renderer::updateTextures(Texture** textures, Size count)
{
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(count * FRAMES_IN_FLIGHT);
    imageInfos.reserve(count * FRAMES_IN_FLIGHT);
    for (auto& frame : frames) {
        for (Size i = 0; i < count; i++) {
            Texture* texture = textures[i];
            vk::Sampler sampler = texture->getSampler();
            vk::DescriptorImageInfo& imageInfo = imageInfos.emplace_back();
            imageInfo.imageView = texture->getView();
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.sampler = sampler ? sampler : defaultSampler;
            vk::WriteDescriptorSet& write = writes.emplace_back();
            write.dstSet = frame.bindlessSet;
            write.dstArrayElement = texture->getIndex();
            write.descriptorCount = 1;
            write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            write.dstBinding = 0;
            write.pImageInfo = &imageInfo;
        }
    }
    device.updateDescriptorSets(writes, {});
}

}   // namespace df