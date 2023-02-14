add_library(StbImage stb/stb_image.h stb/impl.c)
find_package(Vulkan REQUIRED)
target_include_directories(StbImage PUBLIC stb)
add_library(imgui imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_sdl.cpp imgui/misc/freetype/imgui_freetype.cpp imgui/misc/freetype/imgui_freetype.h)
target_link_libraries(imgui SDL2::SDL2 CONAN_PKG::freetype Vulkan::Headers)
target_include_directories(imgui PUBLIC imgui)
target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)