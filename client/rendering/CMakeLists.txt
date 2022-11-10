find_package(Vulkan REQUIRED)
FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG a6bfc237255a6bac1513f7c1ebde6d8aed6b5191
)
FetchContent_MakeAvailable(vma)
add_library(Renderer src/renderer.cpp include/renderer.h src/allocation.cpp src/allocation.h src/vulkan_includes.h src/swapchain.cpp src/swapchain.h)
target_link_libraries(Renderer PUBLIC Core Vulkan::Headers SDL2::SDL2 VulkanMemoryAllocator)
target_include_directories(Renderer PUBLIC include)