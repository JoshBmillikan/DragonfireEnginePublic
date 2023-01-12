add_library(StbImage stb/stb_image.h stb/impl.c)
target_include_directories(StbImage PUBLIC stb)
add_library(imgui imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp )
target_include_directories(imgui PUBLIC imgui)