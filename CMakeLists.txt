cmake_minimum_required(VERSION 3.24)
project(DragonfireEngine)

set(CMAKE_CXX_STANDARD 20)
include(cmake/download_dependencies.CMake)
FetchContent_MakeAvailable(sdl2 spdlog glm physfs json unordered_dense nameof tinyobjloader concurrentQueue entt libPng)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

add_subdirectory(core)
add_subdirectory(client)
