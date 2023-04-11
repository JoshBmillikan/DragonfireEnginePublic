find_package(Vulkan REQUIRED)
FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG a6bfc237255a6bac1513f7c1ebde6d8aed6b5191
)
FetchContent_MakeAvailable(vma)
add_library(Renderer STATIC src/renderer.cpp src/renderer.h src/allocation.cpp src/allocation.h src/vulkan_includes.h src/swapchain.cpp src/swapchain.h src/pipeline.cpp
        src/pipeline.h include/camera.h src/render_context.cpp include/render_context.h src/mesh.cpp src/mesh.h include/vertex.h
        src/model.cpp include/model.h src/material.cpp src/material.h src/render_asset_loaders.cpp src/render_asset_loaders.h src/texture.cpp src/texture.h src/vk_init.cpp)
target_link_libraries(Renderer PUBLIC Core Client Vulkan::Headers SDL2::SDL2 VulkanMemoryAllocator tinyobjloader StbImage imgui)
target_include_directories(Renderer PUBLIC include)
add_subdirectory(shaders)