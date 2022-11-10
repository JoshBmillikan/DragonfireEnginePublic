//
// Created by josh on 11/9/22.
//

#include "renderer.h"
#include <SDL_vulkan.h>
#include <alloca.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace df {
Renderer Renderer::rendererInstance;

Renderer* Renderer::getOrInit(SDL_Window* window) noexcept
{
    if (!rendererInstance.instance) {
        try {
            bool validation = std::getenv("VALIDATION_LAYERS") != nullptr;
            rendererInstance.init(window, validation);
        }
        catch (const std::exception& e) {
            crash("Unhandled error during rendering engine initialization: {}", e.what());
        }
    }
    return &rendererInstance;
}

void Renderer::shutdown() noexcept
{
    if (instance) {
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
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            logger->info("Validation Layer: {}", pCallbackData->pMessage);
            break;
        default: logger->trace("Validation Layer: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

static vk::DebugUtilsMessengerCreateInfoEXT getDebugCreateInfo(spdlog::logger* logger)
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                             | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                             | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    createInfo.pfnUserCallback = &debugCallback;
    createInfo.pUserData = logger;
    return createInfo;
}

static vk::DebugUtilsMessengerEXT createDebugMessenger(vk::Instance instance, spdlog::logger* logger)
{
    auto createInfo = getDebugCreateInfo(logger);
    return instance.createDebugUtilsMessengerEXT(createInfo);
}

void Renderer::init(SDL_Window* window, bool validation)
{
    logger = spdlog::default_logger()->clone("Rendering");
    spdlog::register_logger(logger);
    createInstance(window, validation);
    if (validation)
        debugMessenger = createDebugMessenger(instance, logger.get());
    if (SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)) != SDL_TRUE)
        crash("SDL failed to create vulkan surface: {}", SDL_GetError());
    getPhysicalDevice();
    createDevice();
    queues.graphics = device.getQueue(queues.graphicsFamily, 0);
    queues.present = device.getQueue(queues.presentFamily, 0);
    queues.transfer = device.getQueue(queues.transferFamily, 0);
    initAllocator();
    swapchain = Swapchain(physicalDevice, device, window, surface, queues.graphicsFamily, queues.presentFamily);
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

static std::array<const char*, 6> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
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
    vk::resultCheck(
            device.enumerateDeviceExtensionProperties(nullptr, &count, properties),
            "Failed to get device extensions"
    );
    properties = (vk::ExtensionProperties*) alloca(sizeof(vk::ExtensionProperties) * count);
    vk::resultCheck(
            device.enumerateDeviceExtensionProperties(nullptr, &count, properties),
            "Failed to get device extensions"
    );

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
    logger->info("Using gpu \"{}\"", properties.deviceName.data());
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

bool Renderer::Queues::getFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    UInt count;
    vk::QueueFamilyProperties* properties = nullptr;
    device.getQueueFamilyProperties(&count, properties);
    properties = (vk::QueueFamilyProperties*) alloca(sizeof(vk::QueueFamilyProperties) * count);
    device.getQueueFamilyProperties(&count, properties);

    bool foundGraphics, foundPresent, foundTransfer;
    foundGraphics = foundPresent = foundTransfer = false;
    for (UInt i = 0; i < count; i++) {
        auto& prop = properties[i];
        if (!foundGraphics && prop.queueFlags & vk::QueueFlagBits::eGraphics) {
            foundGraphics = true;
            graphicsFamily = i;
        }
        else if (!foundPresent && device.getSurfaceSupportKHR(i, surface)) {
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

    if (!foundPresent && foundGraphics && device.getSurfaceSupportKHR(graphicsFamily, surface)) {
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