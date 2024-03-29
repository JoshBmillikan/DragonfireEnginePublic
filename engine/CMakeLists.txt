CPMAddPackage("gh:skypjack/entt@3.11.1")
CPMFindPackage(
        NAME SDL2
        VERSION 2.26.3
        URL https://libsdl.org/release/SDL2-2.26.3.zip
)
if (SDL2_ADDED)
    add_library(SDL2::SDL2 ALIAS SDL2)
endif ()
CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        VERSION 1.11.0
        OPTIONS
        "SPDLOG_BUILD_PIC ON"
)
CPMAddPackage("gh:g-truc/glm@0.9.9.8#0.9.9.8")
CPMAddPackage("gh:icculus/physfs@3.2.0#release-3.2.0")
CPMAddPackage("gh:martinus/unordered_dense@3.1.0")
CPMAddPackage(
        NAME tinygltf
        GITHUB_REPOSITORY syoyo/tinygltf
        VERSION 2.8.9
        OPTIONS
        "TINYGLTF_HEADER_ONLY ON"
        "TINYGLTF_BUILD_LOADER_EXAMPLE OFF"
)
CPMAddPackage("gh:zeux/meshoptimizer@0.19")
CPMAddPackage(
        NAME nlohmann_json
        VERSION 3.11.2
        # the git repo is incredibly large, so we download the archived include directory
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/include.zip
        URL_HASH SHA256=e5c7a9f49a16814be27e4ed0ee900ecd0092bfb7dbfca65b5a421b774dccaaed
)
if (nlohmann_json_ADDED)
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif ()
CPMAddPackage(
        NAME sqlite
        VERSION 3.42.0
        URL https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip
)
if (sqlite_ADDED)
    add_library(sqlite STATIC)
    target_sources(sqlite PRIVATE ${sqlite_SOURCE_DIR}/sqlite3.c ${sqlite_SOURCE_DIR}/sqlite3.h)
    target_include_directories(sqlite PUBLIC ${sqlite_SOURCE_DIR})
endif ()
CPMAddPackage(
        NAME BSThreadPool
        GITHUB_REPOSITORY bshoshany/thread-pool
        VERSION 3.3.0
        DOWNLOAD_ONLY True
)

if (BSThreadPool_ADDED)
    add_library(BSThreadPool INTERFACE)
    target_include_directories(BSThreadPool INTERFACE ${BSThreadPool_SOURCE_DIR})
endif ()
CPMAddPackage(
        NAME imgui
        GITHUB_REPOSITORY ocornut/imgui
        VERSION 1.89.5
        DOWNLOAD_ONLY True
)

if (imgui_ADDED)
    add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp  ${imgui_SOURCE_DIR}/imgui_draw.cpp  ${imgui_SOURCE_DIR}/imgui_tables.cpp  ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    find_package(Vulkan REQUIRED)
    add_library(imgui_vulkan STATIC ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h)
    target_include_directories(imgui_vulkan PUBLIC ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui_vulkan PUBLIC imgui SDL2::SDL2 Vulkan::Headers)
    target_include_directories(imgui_vulkan PRIVATE ${SDL_INCLUDE_DIRS})
    target_compile_definitions(imgui_vulkan PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
endif ()
add_subdirectory(core)
add_subdirectory(graphics)
add_subdirectory(application)