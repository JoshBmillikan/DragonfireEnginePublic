//
// Created by josh on 11/9/22.
//

#include "renderer.h"
#include "config.h"
#include <SDL_vulkan.h>
#include <alloca.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace df {

void Renderer::beginRendering(const Camera& camera)
{
    std::unique_lock lock(presentLock);
    vk::Result lastResult = presentResult;
    lock.unlock();
    Frame& frame = getCurrentFrame();

    if (device.waitForFences(frame.fence, true, UINT64_MAX) != vk::Result::eSuccess)
        logger->error("Render fence wait failed, things  may break");
    UInt retryCount = 0;
    do {
        if (lastResult == vk::Result::eErrorOutOfDateKHR || lastResult == vk::Result::eSuboptimalKHR)
            recreateSwapchain();
        else if (lastResult != vk::Result::eSuccess)
            vk::throwResultException(lastResult, "Swapchain presentation/acquire error");
        lastResult = swapchain.next(frame.presentSemaphore);
        retryCount++;
    } while (lastResult != vk::Result::eSuccess && retryCount < 10);
    vk::resultCheck(lastResult, "Failed to resize swapchain");

    char* ptr = static_cast<char*>(globalUniformBuffer.getInfo().pMappedData);
    ptr += globalUniformOffset * (frameCount % FRAMES_IN_FLIGHT);
    UboData* data = reinterpret_cast<UboData*>(ptr);
    glm::mat4 view = camera.getViewMatrix();
    data->viewPerspective = viewPerspective = camera.perspective * view;
    data->viewOrthographic = viewOrthographic = camera.orthographic * view;

    device.resetFences(frame.fence);
    beginCommandRecording();

    if (threadData == nullptr)
        crash("Renderer was not initialized");
    for (UInt i = 0; i < renderThreadCount; i++) {
        threadData[i].mutex.lock();
        threadData[i].command = RenderCommand::begin;
        threadData[i].mutex.unlock();
        threadData[i].cond.notify_one();
    }
}

void Renderer::render(Model* model, const std::vector<glm::mat4>& matrices)
{
    static UInt threadIndex = 0;
    auto& data = threadData[threadIndex];
    {
        std::unique_lock lock(data.mutex);
        data.matrices = matrices.data();
        data.matrixCount = matrices.size();
        data.command = RenderCommand::render;
        data.model = model;
    }
    data.cond.notify_one();
    threadIndex = (threadIndex + 1) % renderThreadCount;
}

void Renderer::endRendering()
{
    renderBarrier.arrive_and_wait();
    Frame& frame = getCurrentFrame();
    vk::CommandBuffer cmd = frame.buffer;
    cmd.executeCommands(renderThreadCount, secondaryBuffers);
    cmd.endRendering();
    imageToPresentSrc(cmd);
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
            case RenderCommand::begin:
                pool = pools[frameCount % FRAMES_IN_FLIGHT];
                cmd = buffers[frameCount % FRAMES_IN_FLIGHT];
                device.resetCommandPool(pool);
                bufferOffset = 0;
                break;
            case RenderCommand::render: {
                if ((bufferOffset + data.matrixCount) * sizeof(glm::mat4) >= vertexBuffer.getInfo().size) {
                    Buffer newBuffer = createInstanceVertexBuffer(
                            std::max(bufferOffset + data.matrixCount, vertexBuffer.getInfo().size * 2)
                    );
                    void* src = vertexBuffer.getInfo().pMappedData;
                    void* dst = newBuffer.getInfo().pMappedData;
                    memcpy(dst, src, vertexBuffer.getInfo().size);
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
                    // TODO pipeline/descriptor sets
                    vk::Buffer vertexBuffers[] = {model->getVertexBuffer().getBuffer(), vertexBuffer};
                    vk::DeviceSize offsets[] = {0, bufferOffset};
                    cmd.bindVertexBuffers(0, vertexBuffers, offsets);
                    cmd.bindIndexBuffer(model->getVertexBuffer().getBuffer(), model->getVertexBuffer().getIndexOffset(), vk::IndexType::eUint32);
                    cmd.drawIndexed(model->getVertexBuffer().getIndexCount(), drawCount, 0, 0, 0);
                }
                bufferOffset += drawCount;
            } break;
            case RenderCommand::end:
                cmd.end();
                secondaryBuffers[threadIndex] = cmd;
                renderBarrier.arrive_and_wait();
                break;
            default: break;
        }
        data.command = RenderCommand::waiting;
    }
    for (vk::CommandPool p : pools)
        device.destroy(p);
    logger->trace("Render thread {} destroyed", threadIndex);
    renderBarrier.arrive_and_drop();
}

bool Renderer::cullTest(Model* model, const glm::mat4& matrix)
{
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

        presentResult = queues.present.presentKHR(presentInfo);
        presentingFrame = nullptr;
    }
    logger->info("Presentation thread destroyed");
}

void Renderer::shutdown() noexcept
{
    if (instance) {
        threadStop.request_stop();
        presentCond.notify_one();
        renderBarrier.arrive_and_drop();
        for (UInt i = 0; threadData != nullptr && i < renderThreadCount; i++) {
            threadData[i].cond.notify_one();
            threadData[i].thread.join();
        }
        presentThreadHandle.join();
        device.waitIdle();
        delete[] threadData;
        delete[] secondaryBuffers;
        for (Frame& frame : frames) {
            device.destroy(frame.fence);
            device.destroy(frame.renderSemaphore);
            device.destroy(frame.presentSemaphore);
            device.destroy(frame.pool);
        }
        pipelineFactory.destroy();
        globalUniformBuffer.destroy();
        device.destroy(depthView);
        depthImage.destroy();
        swapchain.destroy();
        Allocation::shutdownAllocator();
        device.destroy();
        instance.destroy(surface);
        if (debugMessenger)
            instance.destroy(debugMessenger);
        instance.destroy();
        instance = nullptr;
        logger->info("Renderer shutdown successfully");
    }
}

void Renderer::recreateSwapchain()
{
    // todo
}

void Renderer::beginCommandRecording()
{
    Frame& frame = getCurrentFrame();
    device.resetCommandPool(frame.pool);
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    frame.buffer.begin(beginInfo);
    imageToColorWrite(frame.buffer);
    vk::RenderingAttachmentInfo color, depth;
    color.imageView = swapchain.getCurrentView();
    color.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    color.loadOp = vk::AttachmentLoadOp::eClear;
    color.storeOp = vk::AttachmentStoreOp::eStore;
    color.clearValue = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
    depth.imageView = depthView;
    depth.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depth.loadOp = vk::AttachmentLoadOp::eClear;
    depth.storeOp = vk::AttachmentStoreOp::eStore;
    depth.clearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0, 0.0));

    vk::RenderingInfo info;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = &color;
    info.pDepthAttachment = &depth;
    info.layerCount = 1;
    info.renderArea = vk::Rect2D(vk::Offset2D(), swapchain.getExtent());
    frame.buffer.beginRendering(info);
    vk::Viewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(swapchain.getExtent().width);
    viewport.height = static_cast<float>(swapchain.getExtent().height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    frame.buffer.setViewport(0, viewport);
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D();
    scissor.extent = swapchain.getExtent();
    frame.buffer.setScissor(0, scissor);
}

void Renderer::imageToColorWrite(vk::CommandBuffer cmd) const
{
    vk::ImageMemoryBarrier image, depth;
    image.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    image.oldLayout = vk::ImageLayout::eUndefined;
    image.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    image.image = swapchain.getCurrentImage();
    image.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    image.subresourceRange.baseMipLevel = 0;
    image.subresourceRange.levelCount = 1;
    image.subresourceRange.baseArrayLayer = 0;
    image.subresourceRange.layerCount = 1;
    depth.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    depth.oldLayout = vk::ImageLayout::eUndefined;
    depth.newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depth.image = depthImage;
    depth.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth.subresourceRange.baseMipLevel = 0;
    depth.subresourceRange.levelCount = 1;
    depth.subresourceRange.baseArrayLayer = 0;
    depth.subresourceRange.layerCount = 1;

    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            {},
            {},
            {},
            image
    );

    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            {},
            {},
            {},
            depth
    );
}

void Renderer::imageToPresentSrc(vk::CommandBuffer cmd) const
{
    vk::ImageMemoryBarrier barrier;
    barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrier.image = swapchain.getCurrentImage();
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {},
            {},
            {},
            barrier
    );
}

/// callback to log debug messages from the validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logger->info("Validation Layer: {}", pCallbackData->pMessage); break;
        default: logger->trace("Validation Layer: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

static vk::DebugUtilsMessengerCreateInfoEXT getDebugCreateInfo(spdlog::logger* logger)
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
                                 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                                 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                                 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                             | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    createInfo.pfnUserCallback = &debugCallback;
    createInfo.pUserData = logger;
    return createInfo;
}

Renderer::Renderer(SDL_Window* window)
{
    bool validation = std::getenv("VALIDATION_LAYERS") != nullptr;
    init(window, validation);
}

void Renderer::init(SDL_Window* window, bool validation)
{
    logger = spdlog::default_logger()->clone("Rendering");
    logger->info("Initializing rendering engine");
    auto& cfg = Config::get().graphics;
    spdlog::register_logger(logger);
    createInstance(window, validation);
    if (validation) {
        auto debugInfo = getDebugCreateInfo(logger.get());
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
    }
    if (SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)) != SDL_TRUE)
        crash("SDL failed to create vulkan surface: {}", SDL_GetError());
    getPhysicalDevice();
    createDevice();
    queues.graphics = device.getQueue(queues.graphicsFamily, 0);
    queues.present = device.getQueue(queues.presentFamily, 0);
    queues.transfer = device.getQueue(queues.transferFamily, 0);
    logger->info(
            "Using queue families: graphics={}, presentation={}, transfer={}",
            queues.graphicsFamily,
            queues.presentFamily,
            queues.transferFamily
    );
    initAllocator();
    swapchain = Swapchain(physicalDevice, device, window, surface, queues.graphicsFamily, queues.presentFamily, cfg.vsync);
    logger->info("Using swapchain extent {}x{}", swapchain.getExtent().width, swapchain.getExtent().height);
    createDepthImage();
    allocateUniformBuffer();

    pipelineFactory = PipelineFactory(device, swapchain.getFormat(), depthImage.getFormat(), logger.get());
    createFrames();
    logger->info("Using {} render threads", renderThreadCount);
    secondaryBuffers = new vk::CommandBuffer[renderThreadCount];
    threadData = new RenderThreadData[renderThreadCount]();
    for (UInt i = 0; i < renderThreadCount; i++)
        threadData[i].thread = std::thread(&Renderer::renderThread, this, threadStop.get_token(), i);

    presentThreadHandle = std::thread(&Renderer::presentThread, this, threadStop.get_token());
    AssetRegistry::getRegistry().addLoader(std::move(Model::createLoader(this)), "obj");
    logger->info("Vulkan initialization complete");
}

void Renderer::createInstance(SDL_Window* window, bool validation)
{
    void* proc = SDL_Vulkan_GetVkGetInstanceProcAddr();
    if (proc == nullptr)
        crash("Failed to get vulkan instance proc address: {}", SDL_GetError());
    VULKAN_HPP_DEFAULT_DISPATCHER.init((PFN_vkGetInstanceProcAddr) proc);
    auto vkVersion = vk::enumerateInstanceVersion();
    logger->info(
            "Vulkan version {:d}.{:d}.{:d} loaded",
            VK_API_VERSION_MAJOR(vkVersion),
            VK_API_VERSION_MINOR(vkVersion),
            VK_API_VERSION_PATCH(vkVersion)
    );

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = APP_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "Dragonfire Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    UInt extCount, totalExtensions;
    const char** extensions = nullptr;
    if (SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions) == SDL_FALSE)
        crash("Failed to get SDL vulkan extensions: {}", SDL_GetError());
    totalExtensions = validation ? extCount + 1 : extCount;
    extensions = (const char**) alloca(totalExtensions * sizeof(const char*));
    if (SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions) == SDL_FALSE)
        crash("Failed to get SDL vulkan extensions: {}", SDL_GetError());
    if (validation)
        extensions[totalExtensions - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    for (UInt i = 0; i < totalExtensions; i++)
        logger->info("Loaded instance extension: {}", extensions[i]);

    UInt layerCount = 0;
    const char* layers[] = {
            "VK_LAYER_KHRONOS_validation",
    };
    if (validation)
        layerCount++;

    for (UInt i = 0; i < layerCount; i++)
        logger->info("Loaded layer: {}", layers[i]);

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = totalExtensions;

    vk::DebugUtilsMessengerCreateInfoEXT debug;
    if (validation) {
        debug = getDebugCreateInfo(logger.get());
        createInfo.pNext = &debug;
    }

    instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

static std::array<const char*, 5> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
};

static bool isValidDevice(vk::PhysicalDevice device)
{
    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures;
    vk::PhysicalDeviceVulkan12Features features12;
    dynamicRenderingFeatures.pNext = &features12;
    vk::PhysicalDeviceFeatures2 features2;
    features2.pNext = &dynamicRenderingFeatures;
    features2.features.sparseBinding = true;
    features2.features.samplerAnisotropy = true;
    features2.features.sampleRateShading = true;
    device.getFeatures2(&features2);

    vk::ExtensionProperties* properties = nullptr;
    UInt count;
    vk::resultCheck(device.enumerateDeviceExtensionProperties(nullptr, &count, properties), "Failed to get device extensions");
    properties = (vk::ExtensionProperties*) alloca(sizeof(vk::ExtensionProperties) * count);
    vk::resultCheck(device.enumerateDeviceExtensionProperties(nullptr, &count, properties), "Failed to get device extensions");

    for (const char* extension : DEVICE_EXTENSIONS) {
        bool found = false;
        for (UInt i = 0; i < count; i++) {
            if (strcmp(extension, properties[i].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    return true;
}

void Renderer::getPhysicalDevice()
{
    UInt count;
    vk::PhysicalDevice* devices = nullptr;
    vk::resultCheck(instance.enumeratePhysicalDevices(&count, devices), "Failed to get physical devices");
    devices = (vk::PhysicalDevice*) alloca(sizeof(vk::PhysicalDevice) * count);
    vk::resultCheck(instance.enumeratePhysicalDevices(&count, devices), "Failed to get physical devices");

    vk::PhysicalDevice foundDevice = nullptr;
    vk::PhysicalDeviceProperties properties;
    for (UInt i = 0; i < count; i++) {
        try {
            if (isValidDevice(devices[i]) && queues.getFamilies(devices[i], surface)) {
                foundDevice = devices[i];
                properties = foundDevice.getProperties();
                if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                    break;
            }
        }
        catch (const vk::Error& err) {
            logger->error("GPU compatibility check failed: {}", err.what());
        }
    }

    if (!foundDevice)
        crash("No valid GPU available");
    physicalDevice = foundDevice;
    limits = properties.limits;
    if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
        logger->warn("No discrete gpu available");
    logger->info("Using gpu: {}", properties.deviceName.data());
    logger->info(
            "GPU driver version {}.{}.{}",
            VK_VERSION_MAJOR(properties.driverVersion),
            VK_VERSION_MINOR(properties.driverVersion),
            VK_VERSION_PATCH(properties.driverVersion)
    );
}

void Renderer::createDevice()
{
    vk::DeviceQueueCreateInfo queueCreateInfos[3]{};
    UByte queueCount = 1;
    const float priority = 1.0;
    queueCreateInfos[0].queueFamilyIndex = queues.graphicsFamily;
    queueCreateInfos[0].queueCount = 1;
    queueCreateInfos[0].pQueuePriorities = &priority;
    if (queues.graphicsFamily != queues.presentFamily) {
        queueCreateInfos[1].queueFamilyIndex = queues.presentFamily;
        queueCreateInfos[1].queueCount = 1;
        queueCreateInfos[1].pQueuePriorities = &priority;
        queueCount++;
    }
    if (queues.graphicsFamily != queues.transferFamily && queues.presentFamily != queues.transferFamily) {
        queueCreateInfos[2].queueFamilyIndex = queues.transferFamily;
        queueCreateInfos[2].queueCount = 1;
        queueCreateInfos[2].pQueuePriorities = &priority;
        queueCount++;
    }

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures;
    dynamicRenderingFeatures.dynamicRendering = true;
    vk::PhysicalDeviceFeatures features;
    features.sparseBinding = true;
    features.samplerAnisotropy = true;
    features.sampleRateShading = true;

    for (const char* ext : DEVICE_EXTENSIONS)
        logger->info("Loaded device extension: {}", ext);

    vk::DeviceCreateInfo createInfo;
    createInfo.pNext = &dynamicRenderingFeatures;
    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.enabledExtensionCount = DEVICE_EXTENSIONS.size();
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
    createInfo.pEnabledFeatures = &features;

    device = physicalDevice.createDevice(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
}

void Renderer::initAllocator() const
{
    auto& ld = VULKAN_HPP_DEFAULT_DISPATCHER;
    VmaVulkanFunctions functions;
    functions.vkGetInstanceProcAddr = ld.vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = ld.vkGetDeviceProcAddr;
    functions.vkGetPhysicalDeviceProperties = ld.vkGetPhysicalDeviceProperties;
    functions.vkGetPhysicalDeviceMemoryProperties = ld.vkGetPhysicalDeviceMemoryProperties;
    functions.vkAllocateMemory = ld.vkAllocateMemory;
    functions.vkFreeMemory = ld.vkFreeMemory;
    functions.vkMapMemory = ld.vkMapMemory;
    functions.vkUnmapMemory = ld.vkUnmapMemory;
    functions.vkFlushMappedMemoryRanges = ld.vkFlushMappedMemoryRanges;
    functions.vkInvalidateMappedMemoryRanges = ld.vkInvalidateMappedMemoryRanges;
    functions.vkBindBufferMemory = ld.vkBindBufferMemory;
    functions.vkBindImageMemory = ld.vkBindImageMemory;
    functions.vkGetBufferMemoryRequirements = ld.vkGetBufferMemoryRequirements;
    functions.vkGetImageMemoryRequirements = ld.vkGetImageMemoryRequirements;
    functions.vkCreateBuffer = ld.vkCreateBuffer;
    functions.vkDestroyBuffer = ld.vkDestroyBuffer;
    functions.vkCreateImage = ld.vkCreateImage;
    functions.vkDestroyImage = ld.vkDestroyImage;
    functions.vkCmdCopyBuffer = ld.vkCmdCopyBuffer;
    functions.vkGetBufferMemoryRequirements2KHR = ld.vkGetBufferMemoryRequirements2;
    functions.vkGetImageMemoryRequirements2KHR = ld.vkGetImageMemoryRequirements2;
    functions.vkBindBufferMemory2KHR = ld.vkBindBufferMemory2;
    functions.vkBindImageMemory2KHR = ld.vkBindImageMemory2;
    functions.vkGetPhysicalDeviceMemoryProperties2KHR = ld.vkGetPhysicalDeviceMemoryProperties2;
    functions.vkGetDeviceBufferMemoryRequirements = ld.vkGetDeviceBufferMemoryRequirements;
    functions.vkGetDeviceImageMemoryRequirements = ld.vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo createInfo{};
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = device;
    createInfo.instance = instance;
    createInfo.pVulkanFunctions = &functions;
    createInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    Allocation::initAllocator(&createInfo);
}

static vk::Format getDepthFormat(vk::PhysicalDevice physicalDevice)
{
    const std::array<vk::Format, 3> possibleFormats = {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint,
    };
    for (vk::Format fmt : possibleFormats) {
        auto props = physicalDevice.getFormatProperties(fmt);
        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            return fmt;
    }
    crash("No valid depth image format found");
}

void Renderer::createDepthImage()
{
    vk::ImageCreateInfo createInfo;
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.format = getDepthFormat(physicalDevice);
    createInfo.extent = vk::Extent3D(swapchain.getExtent(), 1);
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = vk::SampleCountFlagBits::e1;
    createInfo.tiling = vk::ImageTiling::eOptimal;
    createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    createInfo.initialLayout = vk::ImageLayout::eUndefined;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    depthImage = Image(createInfo, allocInfo);

    vk::ImageSubresourceRange depthSubRange;
    depthSubRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depthSubRange.layerCount = 1;
    depthSubRange.levelCount = 1;
    depthSubRange.baseArrayLayer = 0;
    depthSubRange.baseMipLevel = 0;
    depthView = depthImage.createView(device, depthSubRange);
}

void Renderer::allocateUniformBuffer()
{
    globalUniformOffset = (sizeof(UboData) + limits.minUniformBufferOffsetAlignment - 1)
                          & ~(limits.minUniformBufferOffsetAlignment - 1);
    vk::DeviceSize size = globalUniformOffset * FRAMES_IN_FLIGHT;
    logger->info("Allocating uniform buffer of size {}(minimum alignment of {})", size, limits.minUniformBufferOffsetAlignment);
    vk::BufferCreateInfo createInfo;
    createInfo.size = size;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    createInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    allocInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlagBits::eDeviceLocal);
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocInfo.priority = 1;

    globalUniformBuffer = Buffer(createInfo, allocInfo);
}

bool Renderer::depthHasStencil() const noexcept
{
    return depthImage.getFormat() == vk::Format::eD24UnormS8Uint || depthImage.getFormat() == vk::Format::eD32SfloatS8Uint;
}

void Renderer::createFrames()
{
    vk::CommandPoolCreateInfo poolCreateInfo;
    poolCreateInfo.queueFamilyIndex = queues.graphicsFamily;
    poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;

    for (Frame& frame : frames) {
        frame.pool = device.createCommandPool(poolCreateInfo);
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = frame.pool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        vk::resultCheck(device.allocateCommandBuffers(&allocInfo, &frame.buffer), "Failed to allocate command buffer");
        frame.fence = device.createFence(fenceCreateInfo);
        frame.presentSemaphore = device.createSemaphore(semaphoreCreateInfo);
        frame.renderSemaphore = device.createSemaphore(semaphoreCreateInfo);
    }
}

bool Renderer::Queues::getFamilies(vk::PhysicalDevice pDevice, vk::SurfaceKHR surfaceKhr)
{
    UInt count;
    vk::QueueFamilyProperties* properties = nullptr;
    pDevice.getQueueFamilyProperties(&count, properties);
    properties = (vk::QueueFamilyProperties*) alloca(sizeof(vk::QueueFamilyProperties) * count);
    pDevice.getQueueFamilyProperties(&count, properties);

    bool foundGraphics, foundPresent, foundTransfer;
    foundGraphics = foundPresent = foundTransfer = false;
    for (UInt i = 0; i < count; i++) {
        auto& prop = properties[i];
        if (!foundGraphics && prop.queueFlags & vk::QueueFlagBits::eGraphics) {
            foundGraphics = true;
            graphicsFamily = i;
        }
        else if (!foundPresent && pDevice.getSurfaceSupportKHR(i, surfaceKhr)) {
            foundPresent = true;
            presentFamily = i;
        }
        else if (!foundTransfer && prop.queueFlags & vk::QueueFlagBits::eTransfer) {
            foundTransfer = true;
            transferFamily = i;
        }

        if (foundGraphics && foundPresent && foundTransfer)
            break;
    }

    if (!foundPresent && foundGraphics && pDevice.getSurfaceSupportKHR(graphicsFamily, surfaceKhr)) {
        foundPresent = true;
        presentFamily = graphicsFamily;
    }

    if (!foundTransfer && foundGraphics) {
        transferFamily = graphicsFamily;
        foundTransfer = true;
    }

    return foundGraphics && foundPresent && foundTransfer;
}

}   // namespace df