
add_library(Graphics STATIC src/renderer.cpp include/renderer.h src/material.cpp include/material.h src/model.cpp include/model.h)
target_link_libraries(Graphics PUBLIC Core)
target_include_directories(Graphics PUBLIC include)
add_subdirectory(vulkan)