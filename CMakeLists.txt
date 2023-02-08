cmake_minimum_required(VERSION 3.24)
project(DragonfireEngine)

set(CMAKE_CXX_STANDARD 20)
include(cmake/download_dependencies.CMake)
FetchContent_MakeAvailable(sdl2 spdlog glm physfs json unordered_dense tinyobjloader concurrentQueue entt sol2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

if (NOT WIN32)
    # Tell the linker to search for shared libs in the same directory as the executable
    # This is the default behavior for Windows, but must be set explicitly for other platforms
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif ()

add_subdirectory(third_party)
add_subdirectory(core)
add_subdirectory(client)
