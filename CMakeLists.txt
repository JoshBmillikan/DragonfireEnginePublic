cmake_minimum_required(VERSION 3.25)
project(DragonfireEngine2)

set(CMAKE_CXX_STANDARD 20)

#Game name
set(APP_NAME "Dragonfire Engine")
#Game executable/folder name
set(APP_ID "dragonfire_engine")

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif (CCACHE_FOUND)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /std:c++20 /permissive-)
    elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fstrict-overflow -fno-rtti -fno-exceptions")
        # Tell the linker to search for shared libs in the same directory as the executable
        # This is the default behavior for Windows, but must be set explicitly for other platforms
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fstrict-overflow -fno-rtti -fno-exceptions")
    # Tell the linker to search for shared libs in the same directory as the executable
    # This is the default behavior for Windows, but must be set explicitly for other platforms
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /std:c++20 /permissive- /GR- /EHs-c- /D_HAS_EXCEPTIONS=0)
endif ()

option(SANITIZE "Enable Sanitizers" OFF)

if (SANITIZE)
    message("Address sanitizer enabled")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fno-omit-frame-pointer)
    else ()
        message("Address sanitizer is only supported on gcc")
    endif ()
endif ()

set(ASSET_DIR ${CMAKE_SOURCE_DIR}/assets)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(cmake/CPM.cmake)

add_subdirectory(engine)