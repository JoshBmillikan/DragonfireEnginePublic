//
// Created by josh on 2/22/23.
//
#include "config.h"
#include "material.h"
#include "renderer.h"
#include <SDL_vulkan.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>
#if defined(_MSC_VER) || defined(__MINGW32__)
    #include <malloc.h>
#else
    #include <alloca.h>
#endif
namespace df {

Renderer::Renderer(SDL_Window* window) : window(window)
{
    bool validation = std::getenv("VALIDATION_LAYERS") != nullptr;
    init(validation);
}

/// List of device extensions to enable
/// Some extension are now core and don't need to be enabled,
/// but are still listed here for backwards compatability
static std::array<const char*, 6> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

static vk::DebugUtilsMessengerCreateInfoEXT getDebugCreateInfo(spdlog::logger* logger);

void Renderer::init(bool validation)
{
    logger = spdlog::default_logger()->clone("Rendering");
    logger->info("Initializing rendering engine");
    spdlog::register_logger(logger);
    createInstance(validation);
    if (validation) {
        auto debugInfo = getDebugCreateInfo(logger.get());
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
    }
    if (SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)) != SDL_TRUE)
        crash("SDL failed to create vulkan surface: {}", SDL_GetError());
    getPhysicalDevice();
    getSampleCount();
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
    swapchain = Swapchain(this, window);
    createRenderPass();
    logger->info("Using swapchain extent {}x{}", swapchain.getExtent().width, swapchain.getExtent().height);
    createDepthImage();
    createMsaaImage();
    swapchain.createFramebuffers(mainPass, depthView, msaaView);
    allocateUniformBuffer();
    createDescriptorPool();
    pipelineFactory = PipelineFactory(device, mainPass, logger.get(), rasterSamples);
    createFrames();
    createDefaultSampler();
    logger->info("Using {} render threads", renderThreadCount);
    secondaryBuffers = new vk::CommandBuffer[renderThreadCount];
    threadData = new RenderThreadData[renderThreadCount]();
    for (UInt i = 0; i < renderThreadCount; i++)
        threadData[i].thread = std::thread(&Renderer::renderThread, this, threadStop.get_token(), i);

    presentThreadHandle = std::thread(&Renderer::presentThread, this, threadStop.get_token());
    initImGui();
    logger->info("Vulkan initialization complete");
}

void Renderer::createInstance(bool validation)
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

vk::DebugUtilsMessengerCreateInfoEXT getDebugCreateInfo(spdlog::logger* logger)
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

static bool isValidDevice(vk::PhysicalDevice device)
{
    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferFeatures{};
    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.pNext = &bufferFeatures;
    vk::PhysicalDeviceFeatures2 features2;
    features2.pNext = &indexingFeatures;
    device.getFeatures2(&features2);
    bool supportsFeatures = features2.features.sparseBinding && features2.features.samplerAnisotropy
                            && features2.features.sampleRateShading && indexingFeatures.descriptorBindingPartiallyBound
                            && indexingFeatures.runtimeDescriptorArray
                            && indexingFeatures.descriptorBindingSampledImageUpdateAfterBind
                            && indexingFeatures.descriptorBindingVariableDescriptorCount && bufferFeatures.bufferDeviceAddress;

    if (!supportsFeatures)
        return false;

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

void Renderer::getSampleCount()
{
    GraphicsSettings::AntiAliasingMode aaMode = Config::get().graphics.antiAliasing;
    vk::SampleCountFlags validFlags = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
    switch (aaMode) {
        case GraphicsSettings::AntiAliasingMode::msaa64:
            if (validFlags & vk::SampleCountFlagBits::e64) {
                rasterSamples = vk::SampleCountFlagBits::e64;
                logger->info("Using Anti-Aliasing mode: MSAA 64x");
                break;
            }
            logger->warn("MSAA 64 not available, trying to fall back to MSAA 32");
        case GraphicsSettings::AntiAliasingMode::msaa32:
            if (validFlags & vk::SampleCountFlagBits::e32) {
                rasterSamples = vk::SampleCountFlagBits::e32;
                logger->info("Using Anti-Aliasing mode: MSAA 32x");
                break;
            }
            logger->warn("MSAA 32 not available, trying to fall back to MSAA 16");
        case GraphicsSettings::AntiAliasingMode::msaa16:
            if (validFlags & vk::SampleCountFlagBits::e32) {
                rasterSamples = vk::SampleCountFlagBits::e16;
                logger->info("Using Anti-Aliasing mode: MSAA 16x");
                break;
            }
            logger->warn("MSAA 16 not available, trying to fall back to MSAA 8");
        case GraphicsSettings::AntiAliasingMode::msaa8:
            if (validFlags & vk::SampleCountFlagBits::e8) {
                rasterSamples = vk::SampleCountFlagBits::e8;
                logger->info("Using Anti-Aliasing mode: MSAA 8x");
                break;
            }
            logger->warn("MSAA 8 not available, trying to fall back to MSAA 4");
        case GraphicsSettings::AntiAliasingMode::msaa4:
            if (validFlags & vk::SampleCountFlagBits::e4) {
                rasterSamples = vk::SampleCountFlagBits::e4;
                logger->info("Using Anti-Aliasing mode: MSAA 4x");
                break;
            }
            logger->warn("MSAA 4 not available, trying to fall back to MSAA 2");
        case GraphicsSettings::AntiAliasingMode::msaa2:
            if (validFlags & vk::SampleCountFlagBits::e2) {
                rasterSamples = vk::SampleCountFlagBits::e2;
                logger->info("Using Anti-Aliasing mode: MSAA 2x");
                break;
            }
            logger->warn("MSAA 2 not available, anti-aliasing will be disabled");
        default: rasterSamples = vk::SampleCountFlagBits::e1; break;
    }
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

    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferFeatures{};
    bufferFeatures.bufferDeviceAddress = true;

    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.descriptorBindingPartiallyBound = true;
    indexingFeatures.runtimeDescriptorArray = true;
    indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
    indexingFeatures.descriptorBindingVariableDescriptorCount = true;
    indexingFeatures.pNext = &bufferFeatures;

    vk::PhysicalDeviceFeatures2 features2{};
    features2.pNext = &indexingFeatures;
    features2.features.sparseBinding = true;
    features2.features.samplerAnisotropy = true;
    features2.features.sampleRateShading = true;

    for (const char* ext : DEVICE_EXTENSIONS)
        logger->info("Loaded device extension: {}", ext);

    vk::DeviceCreateInfo createInfo;
    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.enabledExtensionCount = DEVICE_EXTENSIONS.size();
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
    createInfo.pNext = &features2;

    device = physicalDevice.createDevice(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
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

void Renderer::createRenderPass()
{
    vk::AttachmentDescription attachments[3];
    attachments[0].format = swapchain.getFormat();
    attachments[0].samples = rasterSamples;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    attachments[1].format = getDepthFormat(physicalDevice);
    attachments[1].samples = rasterSamples;
    attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
    attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[1].initialLayout = vk::ImageLayout::eUndefined;
    attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    attachments[2].format = swapchain.getFormat();
    attachments[2].samples = vk::SampleCountFlagBits::e1;
    attachments[2].loadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[2].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[2].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[2].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[2].initialLayout = vk::ImageLayout::eUndefined;
    attachments[2].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorRef, depthRef, resolveRef;
    colorRef.attachment = 0;
    colorRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
    depthRef.attachment = 1;
    depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    resolveRef.attachment = 2;
    resolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;
    subpass.pResolveAttachments = &resolveRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput
                              | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput
                              | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    vk::RenderPassCreateInfo createInfo;
    createInfo.attachmentCount = 3;
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    mainPass = device.createRenderPass(createInfo);
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
    createInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    Allocation::initAllocator(&createInfo);
}

void Renderer::createDepthImage()
{
    vk::ImageCreateInfo createInfo;
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.format = getDepthFormat(physicalDevice);
    createInfo.extent = vk::Extent3D(swapchain.getExtent(), 1);
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = rasterSamples;
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

void Renderer::createMsaaImage()
{
    vk::ImageCreateInfo createInfo;
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.format = swapchain.getFormat();
    createInfo.extent = vk::Extent3D(swapchain.getExtent(), 1);
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = rasterSamples;
    createInfo.tiling = vk::ImageTiling::eOptimal;
    createInfo.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    createInfo.initialLayout = vk::ImageLayout::eUndefined;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    msaaImage = Image(createInfo, allocInfo);

    vk::ImageSubresourceRange imageSubRange;
    imageSubRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageSubRange.layerCount = 1;
    imageSubRange.levelCount = 1;
    imageSubRange.baseArrayLayer = 0;
    imageSubRange.baseMipLevel = 0;
    msaaView = msaaImage.createView(device, imageSubRange);
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

void Renderer::createDescriptorPool()
{
    vk::DescriptorPoolSize sizes[2];
    sizes[0].type = vk::DescriptorType::eUniformBuffer;
    sizes[0].descriptorCount = FRAMES_IN_FLIGHT;
    sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    sizes[1].descriptorCount = MAX_TEXTURE_DESCRIPTORS;
    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.maxSets = MAX_TEXTURE_DESCRIPTORS + 16;
    poolCreateInfo.pPoolSizes = sizes;
    poolCreateInfo.poolSizeCount = 2;
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    descriptorPool = device.createDescriptorPool(poolCreateInfo);

    vk::DescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    bindings[0].stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    bindings[1].binding = 0;
    bindings[1].descriptorCount = MAX_TEXTURE_DESCRIPTORS;
    bindings[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorBindingFlags flags = vk::DescriptorBindingFlagBits::ePartiallyBound
                                       | vk::DescriptorBindingFlagBits::eUpdateAfterBind
                                       | vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagInfo{};
    flagInfo.bindingCount = 1;
    flagInfo.pBindingFlags = &flags;
    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.pBindings = bindings;
    layoutCreateInfo.bindingCount = 1;
    globalDescriptorSetLayout = device.createDescriptorSetLayout(layoutCreateInfo);
    layoutCreateInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
    layoutCreateInfo.pBindings = &bindings[1];
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pNext = &flagInfo;
    bindlessLayout = device.createDescriptorSetLayout(layoutCreateInfo);
}

void Renderer::createFrames()
{
    vk::CommandPoolCreateInfo poolCreateInfo;
    poolCreateInfo.queueFamilyIndex = queues.graphicsFamily;
    poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;

    std::array<vk::DescriptorSetLayout, FRAMES_IN_FLIGHT> layouts;
    layouts.fill(globalDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo descriptorSetInfo;
    descriptorSetInfo.descriptorPool = descriptorPool;
    descriptorSetInfo.pSetLayouts = layouts.data();
    descriptorSetInfo.descriptorSetCount = FRAMES_IN_FLIGHT;
    vk::DescriptorSet allocatedSets[FRAMES_IN_FLIGHT];
    vk::resultCheck(device.allocateDescriptorSets(&descriptorSetInfo, allocatedSets), "Failed to allocate descriptor sets");

    std::array<UInt, FRAMES_IN_FLIGHT> count{};
    count.fill(MAX_TEXTURE_DESCRIPTORS - 1);
    std::array<vk::DescriptorSetLayout, FRAMES_IN_FLIGHT> bindlessLayouts;
    bindlessLayouts.fill(bindlessLayout);
    vk::DescriptorSetVariableDescriptorCountAllocateInfo countAllocateInfo{};
    countAllocateInfo.descriptorSetCount = FRAMES_IN_FLIGHT;
    countAllocateInfo.pDescriptorCounts = count.data();
    descriptorSetInfo.pNext = &countAllocateInfo;
    descriptorSetInfo.pSetLayouts = bindlessLayouts.data();
    vk::DescriptorSet bindlessSets[FRAMES_IN_FLIGHT];
    vk::resultCheck(device.allocateDescriptorSets(&descriptorSetInfo, bindlessSets), "Failed to allocate descriptor sets");

    std::array<vk::DescriptorBufferInfo, FRAMES_IN_FLIGHT> bufferInfos;
    std::array<vk::WriteDescriptorSet, FRAMES_IN_FLIGHT> writes;
    for (UInt i = 0; i < FRAMES_IN_FLIGHT; i++) {
        Frame& frame = frames[i];
        frame.pool = device.createCommandPool(poolCreateInfo);
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = frame.pool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        vk::resultCheck(device.allocateCommandBuffers(&allocInfo, &frame.buffer), "Failed to allocate command buffer");
        allocInfo.level = vk::CommandBufferLevel::eSecondary;
        vk::resultCheck(device.allocateCommandBuffers(&allocInfo, &frame.uiBuffer), "Failed to allocate command buffer");
        frame.fence = device.createFence(fenceCreateInfo);
        frame.presentSemaphore = device.createSemaphore(semaphoreCreateInfo);
        frame.renderSemaphore = device.createSemaphore(semaphoreCreateInfo);
        frame.globalDescriptorSet = allocatedSets[i];
        frame.bindlessSet = bindlessSets[i];
        bufferInfos[i].buffer = globalUniformBuffer;
        bufferInfos[i].offset = globalUniformOffset * i;
        bufferInfos[i].range = sizeof(UboData);
        writes[i].dstSet = frame.globalDescriptorSet;
        writes[i].descriptorType = vk::DescriptorType::eUniformBuffer;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &bufferInfos[i];
    }
    device.updateDescriptorSets(writes, {});
}

void Renderer::createDefaultSampler()
{
    vk::SamplerCreateInfo createInfo{};
    createInfo.magFilter = vk::Filter::eLinear;
    createInfo.minFilter = vk::Filter::eLinear;
    createInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    createInfo.anisotropyEnable = true;
    createInfo.maxAnisotropy = limits.maxSamplerAnisotropy;
    createInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    createInfo.unnormalizedCoordinates = false;
    createInfo.compareEnable = false;
    createInfo.compareOp = vk::CompareOp::eAlways;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    createInfo.mipLodBias = 0;
    createInfo.minLod = 0;
    createInfo.maxLod = 0;

    defaultSampler = device.createSampler(createInfo);
}

// Load the functions used by ImGui from the default dispatcher
static PFN_vkVoidFunction getFunction(const char* function_name, void*)
{
#define FN(name)                                                                                                               \
    if (strcmp(function_name, #name) == 0)                                                                                     \
        return reinterpret_cast<PFN_vkVoidFunction>(VULKAN_HPP_DEFAULT_DISPATCHER.name);
    FN(vkAllocateCommandBuffers)
    FN(vkAllocateDescriptorSets)
    FN(vkAllocateMemory)
    FN(vkBindBufferMemory)
    FN(vkBindImageMemory)
    FN(vkCmdBindDescriptorSets)
    FN(vkCmdBindIndexBuffer)
    FN(vkCmdBindPipeline)
    FN(vkCmdBindVertexBuffers)
    FN(vkCmdCopyBufferToImage)
    FN(vkCmdDrawIndexed)
    FN(vkCmdPipelineBarrier)
    FN(vkCmdPushConstants)
    FN(vkCmdSetScissor)
    FN(vkCmdSetViewport)
    FN(vkCreateBuffer)
    FN(vkCreateCommandPool)
    FN(vkCreateDescriptorSetLayout)
    FN(vkCreateFence)
    FN(vkCreateFramebuffer)
    FN(vkCreateGraphicsPipelines)
    FN(vkCreateImage)
    FN(vkCreateImageView)
    FN(vkCreatePipelineLayout)
    FN(vkCreateRenderPass)
    FN(vkCreateSampler)
    FN(vkCreateSemaphore)
    FN(vkCreateShaderModule)
    FN(vkCreateSwapchainKHR)
    FN(vkDestroyBuffer)
    FN(vkDestroyCommandPool)
    FN(vkDestroyDescriptorSetLayout)
    FN(vkDestroyFence)
    FN(vkDestroyFramebuffer)
    FN(vkDestroyImage)
    FN(vkDestroyImageView)
    FN(vkDestroyPipeline)
    FN(vkDestroyPipelineLayout)
    FN(vkDestroyRenderPass)
    FN(vkDestroySampler)
    FN(vkDestroySemaphore)
    FN(vkDestroyShaderModule)
    FN(vkDestroySurfaceKHR)
    FN(vkDestroySwapchainKHR)
    FN(vkDeviceWaitIdle)
    FN(vkFlushMappedMemoryRanges)
    FN(vkFreeCommandBuffers)
    FN(vkFreeDescriptorSets)
    FN(vkFreeMemory)
    FN(vkGetBufferMemoryRequirements)
    FN(vkGetImageMemoryRequirements)
    FN(vkGetPhysicalDeviceMemoryProperties)
    FN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    FN(vkGetPhysicalDeviceSurfaceFormatsKHR)
    FN(vkGetPhysicalDeviceSurfacePresentModesKHR)
    FN(vkGetSwapchainImagesKHR)
    FN(vkMapMemory)
    FN(vkUnmapMemory)
    FN(vkUpdateDescriptorSets)
#undef FN
    return nullptr;
}

void Renderer::initImGui()
{
    ImGui_ImplVulkan_LoadFunctions(&getFunction, nullptr);
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo info{};
    info.Instance = instance;
    info.PhysicalDevice = physicalDevice;
    info.Device = device;
    info.QueueFamily = queues.graphicsFamily;
    info.Queue = queues.graphics;
    info.PipelineCache = pipelineFactory.getCache();
    info.DescriptorPool = descriptorPool;
    info.Subpass = 0;
    info.MSAASamples = static_cast<VkSampleCountFlagBits>(rasterSamples);
    info.ImageCount = swapchain.getImageCount();
    info.MinImageCount = FRAMES_IN_FLIGHT;
    info.Allocator = nullptr;
    info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&info, mainPass);

    vk::CommandBuffer cmd = frames[0].buffer;
    vk::CommandBufferBeginInfo begin{};
    begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(begin);
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    cmd.end();
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    queues.graphics.submit(submitInfo);
    queues.graphics.waitIdle();
    ImGui_ImplVulkan_DestroyFontUploadObjects();
    logger->info("ImGui rendering initialized");
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