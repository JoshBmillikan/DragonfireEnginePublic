cmake_minimum_required(VERSION 3.24)
project(DragonfireEngine)

set(CMAKE_CXX_STANDARD 20)
include(cmake/download_dependencies.CMake)
FetchContent_MakeAvailable(sdl2 glm unordered_dense tinyobjloader concurrentQueue sol2 Catch2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

if (NOT WIN32)
    # Tell the linker to search for shared libs in the same directory as the executable
    # This is the default behavior for Windows, but must be set explicitly for other platforms
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /std:c++20 /permissive-)
endif ()
if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
else()
    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

add_subdirectory(third_party)
add_subdirectory(core)
add_subdirectory(server)
add_subdirectory(client)
