cmake_minimum_required(VERSION 3.24)
project(DragonfireEngine)

set(CMAKE_CXX_STANDARD 20)
include(cmake/download_dependencies.CMake)
FetchContent_MakeAvailable(sdl2 spdlog glm physfs json unordered_dense nameof tinyobjloader concurrentQueue entt)

add_subdirectory(core)
add_subdirectory(client)
