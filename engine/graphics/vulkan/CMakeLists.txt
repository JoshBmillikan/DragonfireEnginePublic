find_package(Vulkan REQUIRED)
CPMAddPackage(
        NAME VulkanMemoryAllocator
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        VERSION 3.0.1
        DOWNLOAD_ONLY True
)

if (VulkanMemoryAllocator_ADDED)
    add_library(VulkanMemoryAllocator INTERFACE IMPORTED)
    target_include_directories(VulkanMemoryAllocator INTERFACE ${VulkanMemoryAllocator_SOURCE_DIR}/include)
endif ()

CPMAddPackage(
        NAME spirv-reflect
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Reflect
        GIT_TAG sdk-1.3.243.0
        OPTIONS
        "SPIRV_REFLECT_EXECUTABLE OFF"
        "SPIRV_REFLECT_EXAMPLES OFF"
        "SPIRV_REFLECT_STATIC_LIB ON"
)

add_library(VulkanRenderer STATIC src/vk_renderer.cpp src/vk_renderer.h src/vk_include.h src/init.cpp src/swapchain.cpp src/swapchain.h src/allocation.cpp src/allocation.h src/pipeline.cpp src/pipeline.h src/vk_material.cpp src/vk_material.h src/descriptor_set.cpp src/descriptor_set.h)
target_link_libraries(VulkanRenderer PUBLIC Core Graphics)
target_link_libraries(VulkanRenderer PRIVATE Vulkan::Headers VulkanMemoryAllocator spirv-reflect-static)
target_include_directories(VulkanRenderer PRIVATE src)
target_precompile_headers(VulkanRenderer PRIVATE <vector> <memory> <string> <core.h> <spdlog/spdlog.h> <vk_include.h>)
target_compile_definitions(VulkanRenderer PRIVATE "SHADER_OUTPUT_PATH=\"${SHADERS_OUT}\"")
add_dependencies(VulkanRenderer ShaderTarget)