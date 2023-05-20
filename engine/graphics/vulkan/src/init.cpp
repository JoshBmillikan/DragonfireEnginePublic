//
// Created by josh on 5/17/23.
//
#include "vk_renderer.h"
#include <SDL2/SDL_vulkan.h>
#include <allocators.h>
#if defined(_MSC_VER) || defined(__MINGW32__)
    #include <malloc.h>
#else
    #include <alloca.h>
#endif
namespace dragonfire {

static void initVulkan(spdlog::logger* logger) noexcept
{
    void* proc = SDL_Vulkan_GetVkGetInstanceProcAddr();
    if (proc == nullptr)
        crash("Failed to get vulkan instance proc address: {}", SDL_GetError());
    VULKAN_HPP_DEFAULT_DISPATCHER.init((PFN_vkGetInstanceProcAddr) proc);
    try {
        UInt32 version = vk::enumerateInstanceVersion();
        logger->info(
                "Vulkan version {:d}.{:d}.{:d} loaded",
                VK_API_VERSION_MAJOR(version),
                VK_API_VERSION_MINOR(version),
                VK_API_VERSION_PATCH(version)
        );
    }
    catch (...) {
        logger->error("Unknown vulkan version loaded");
    }
}

static vk::SurfaceKHR createSurface(SDL_Window* window, vk::Instance instance) noexcept
{
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface(window, instance, &surface) == SDL_FALSE)
        crash("SDL Failed to create surface");
    return surface;
}

static vk::DebugUtilsMessengerCreateInfoEXT getDebugCreateInfo(
        PFN_vkDebugUtilsMessengerCallbackEXT callback,
        spdlog::logger* logger
)
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                             | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                             | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    createInfo.pfnUserCallback = callback;
    createInfo.pUserData = logger;
    return createInfo;
}

void VkRenderer::init()
{
    logger = spdlog::default_logger()->clone("Rendering");
    spdlog::register_logger(logger);
    createWindow(SDL_WINDOW_VULKAN);
    initVulkan(logger.get());
    const bool validation = std::getenv("VALIDATION_LAYERS") != nullptr;
    try {
        createInstance(validation);
        if (validation) {
            auto createInfo = getDebugCreateInfo(&VkRenderer::debugCallback, logger.get());
            debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
        }
        surface = createSurface(window, instance);
        getPhysicalDevice();
        createDevice();
        swapchain = Swapchain(physicalDevice, device, window, surface, queues.graphicsFamily, queues.presentFamily);
        createGpuAllocator();
    }
    catch (const std::exception& e) {
        crash("Vulkan initialization failed: {}", e.what());
    }
}

void VkRenderer::createInstance(bool validation)
{
    UInt32 extensionCount, windowExtensionCount;
    const char** extensions = nullptr;
    if (SDL_Vulkan_GetInstanceExtensions(window, &windowExtensionCount, extensions) == SDL_FALSE)
        crash("SDL failed to get instance extensions: {}", SDL_GetError());

    extensionCount = windowExtensionCount;
    if (validation)
        extensionCount++;

    extensions = (const char**) alloca(extensionCount * sizeof(const char*));

    if (SDL_Vulkan_GetInstanceExtensions(window, &windowExtensionCount, extensions) == SDL_FALSE)
        crash("SDL failed to get instance extensions: {}", SDL_GetError());

    if (validation)
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    for (UInt32 i = 0; i < extensionCount; i++)
        logger->info("Loaded instance extension \"{}\"", extensions[i]);

    UInt32 layerCount = 0;
    const char* layers[] = {
            "VK_LAYER_KHRONOS_validation",
    };
    if (validation)
        layerCount++;

    for (UInt32 i = 0; i < layerCount; i++)
        logger->info("Loaded layer \"{}\"", layers[i]);

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = APP_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "Dragonfire Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(2, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = extensionCount;

    vk::DebugUtilsMessengerCreateInfoEXT debug;
    if (validation) {
        debug = getDebugCreateInfo(&VkRenderer::debugCallback, logger.get());
        createInfo.pNext = &debug;
    }

    instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

/**
 * @brief List of device extensions to enable
 *  Some extension are now core and don't need to be enabled,
 *  but are still listed here for backwards compatability/completeness
 */
static std::array<const char*, 6> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

static bool checkFeatures(vk::PhysicalDevice device) noexcept
{
    auto chain = device.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceDescriptorIndexingFeatures,
            vk::PhysicalDeviceBufferDeviceAddressFeatures>();
    auto& features = chain.get<vk::PhysicalDeviceFeatures2>();
    auto& indexFeatures = chain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
    auto& bufferFeatures = chain.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
    return features.features.sparseBinding && features.features.samplerAnisotropy && features.features.sampleRateShading
           && features.features.multiDrawIndirect && indexFeatures.descriptorBindingPartiallyBound
           && indexFeatures.runtimeDescriptorArray && indexFeatures.descriptorBindingSampledImageUpdateAfterBind
           && indexFeatures.descriptorBindingVariableDescriptorCount && bufferFeatures.bufferDeviceAddress;
}

static bool isValidDevice(vk::PhysicalDevice device) noexcept
{
    if (!checkFeatures(device))
        return false;

    vk::ExtensionProperties* properties = nullptr;
    UInt32 count = 0;
    if (device.enumerateDeviceExtensionProperties(nullptr, &count, properties) != vk::Result::eSuccess)
        return false;

    properties = (vk::ExtensionProperties*) alloca(sizeof(vk::ExtensionProperties) * count);
    if (device.enumerateDeviceExtensionProperties(nullptr, &count, properties) != vk::Result::eSuccess)
        return false;

    for (const char* extension : DEVICE_EXTENSIONS) {
        bool found = false;
        for (UInt32 i = 0; i < count; i++) {
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

/***
 * @brief Attempt to log the GPU driver version
 *  The version exposed by vulkan is a packed 32 bit integer, in most cases
 *  this can be decoded using VK_VERSION macros but some vendors use a different
 * packing. We make a best effort attempt to accommodate this, but it may not
 * always be accurate.
 * @param properties physical device properties to get the driver version/vendor id from
 * @param logger logger to log to
 */
static void logDriverVersion(const vk::PhysicalDeviceProperties& properties, spdlog::logger* logger)
{
    UInt32 version = properties.driverVersion;
    switch (properties.vendorID) {
        case 4318:   // NVIDIA
            logger->info(
                    "GPU driver version {}.{}.{}.{}",
                    (version >> 22) & 0x3ff,
                    (version >> 14) & 0x0ff,
                    (version >> 6) & 0x0ff,
                    version & 0x003f
            );
            break;
        case 0x8086:   // INTEL
#ifdef _WIN32          // Intel only uses a different scheme on windows
            logger->info("GPU driver version {}.{}", version >> 14, version & 0x3fff);
            break;
#endif
        default:
            logger->info(
                    "GPU driver version {}.{}.{}",
                    VK_VERSION_MAJOR(version),
                    VK_VERSION_MINOR(version),
                    VK_VERSION_PATCH(version)
            );
    }
}

void VkRenderer::getPhysicalDevice()
{
    auto devices = instance.enumeratePhysicalDevices<FrameAllocator<vk::PhysicalDevice>>();
    vk::PhysicalDevice found = nullptr;
    vk::PhysicalDeviceProperties properties;
    for (vk::PhysicalDevice current : devices) {
        if (isValidDevice(current) && getQueueFamilies(current)) {
            found = current;
            properties = current.getProperties();
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                break;
        }
    }
    if (!found)
        crash("No valid gpu found");
    physicalDevice = found;
    limits = properties.limits;
    if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
        logger->warn("No dedicated gpu available");
    logger->info("Using gpu: {}", properties.deviceName.data());
    logDriverVersion(properties, logger.get());
    logger->info(
            "Using queue families: graphics={}, presentation={}, transfer={}",
            queues.graphicsFamily,
            queues.presentFamily,
            queues.transferFamily
    );
}

bool VkRenderer::getQueueFamilies(vk::PhysicalDevice pDevice) noexcept
{
    UInt32 count;
    vk::QueueFamilyProperties* properties = nullptr;
    pDevice.getQueueFamilyProperties(&count, properties);
    properties = (vk::QueueFamilyProperties*) alloca(sizeof(vk::QueueFamilyProperties) * count);
    pDevice.getQueueFamilyProperties(&count, properties);
    bool foundGraphics, foundPresent, foundTransfer;
    foundGraphics = foundPresent = foundTransfer = false;
    for (UInt32 i = 0; i < count; i++) {
        auto& prop = properties[i];
        bool surfaceSupport;
        try {
            surfaceSupport = pDevice.getSurfaceSupportKHR(i, surface);
        }
        catch (...) {
            surfaceSupport = false;
        }
        if (!foundGraphics && prop.queueFlags & vk::QueueFlagBits::eGraphics) {
            foundGraphics = true;
            queues.graphicsFamily = i;
        }
        else if (!foundPresent && surfaceSupport) {
            foundPresent = true;
            queues.presentFamily = i;
        }
        else if (!foundTransfer && prop.queueFlags & vk::QueueFlagBits::eTransfer) {
            foundTransfer = true;
            queues.transferFamily = i;
        }

        if (foundGraphics && foundPresent && foundTransfer)
            break;
    }

    try {
        if (!foundPresent && foundGraphics && pDevice.getSurfaceSupportKHR(queues.graphicsFamily, surface)) {
            foundPresent = true;
            queues.presentFamily = queues.graphicsFamily;
        }
    }
    catch (...) {
        return false;
    }

    if (!foundTransfer && foundGraphics) {
        queues.transferFamily = queues.graphicsFamily;
        foundTransfer = true;
    }

    return foundGraphics && foundPresent && foundTransfer;
}

void VkRenderer::createDevice()
{
    std::array<vk::DeviceQueueCreateInfo, 3> queueCreateInfos;
    UInt32 queueCount = 1;
    const float priority = 1.0;
    queueCreateInfos[0].queueFamilyIndex = queues.graphicsFamily;
    queueCreateInfos[0].queueCount = 1;
    queueCreateInfos[0].pQueuePriorities = &priority;
    if (queues.graphicsFamily != queues.presentFamily) {
        queueCreateInfos[queueCount].queueFamilyIndex = queues.presentFamily;
        queueCreateInfos[queueCount].queueCount = 1;
        queueCreateInfos[queueCount].pQueuePriorities = &priority;
        queueCount++;
    }
    if (queues.graphicsFamily != queues.transferFamily && queues.presentFamily != queues.transferFamily) {
        queueCreateInfos[queueCount].queueFamilyIndex = queues.transferFamily;
        queueCreateInfos[queueCount].queueCount = 1;
        queueCreateInfos[queueCount].pQueuePriorities = &priority;
        queueCount++;
    }

    vk::PhysicalDeviceFeatures2 features{};
    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferFeatures{};
    indexingFeatures.pNext = &bufferFeatures;
    features.pNext = &indexingFeatures;

    features.features.sparseBinding = true;
    features.features.samplerAnisotropy = true;
    features.features.sampleRateShading = true;
    features.features.multiDrawIndirect = true;

    indexingFeatures.descriptorBindingPartiallyBound = true;
    indexingFeatures.runtimeDescriptorArray = true;
    indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
    indexingFeatures.descriptorBindingVariableDescriptorCount = true;

    bufferFeatures.bufferDeviceAddress = true;

    for (const char* ext : DEVICE_EXTENSIONS)
        logger->info("Loaded device extension: {}", ext);

    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = DEVICE_EXTENSIONS.size();
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
    createInfo.pNext = &features;

    device = physicalDevice.createDevice(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
    queues.graphics = device.getQueue(queues.graphicsFamily, 0);
    queues.present = device.getQueue(queues.presentFamily, 0);
    queues.transfer = device.getQueue(queues.transferFamily, 0);
}

void VkRenderer::createGpuAllocator()
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

    vk::resultCheck(
            static_cast<vk::Result>(vmaCreateAllocator(&createInfo, &allocator)),
            "Failed to create GPU allocator"
    );
}

}   // namespace dragonfire