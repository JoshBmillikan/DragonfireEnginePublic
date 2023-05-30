
add_library(App src/app.cpp src/app.h)
add_executable(${APP_ID} src/main.cpp)
target_link_libraries(${APP_ID} App)
target_link_libraries(App PUBLIC Core VulkanRenderer)
target_precompile_headers(App REUSE_FROM Core)