cmake_minimum_required(VERSION 3.24)
project(DragonfireEngine)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

FetchContent_Declare(
        sdl2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG 55b03c7493a7abed33cf803d1380a40fa8af903f
        FIND_PACKAGE_ARGS NAMES SDL2
)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG ad0e89cbfb4d0c1ce4d097e134eb7be67baebb36
)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
)
FetchContent_Declare(
        physfs
        GIT_REPOSITORY https://github.com/icculus/physfs.git
        GIT_TAG eb3383b532c5f74bfeb42ec306ba2cf80eed988c
)
FetchContent_Declare(
        unordered_dense
        GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
        GIT_TAG a7ece061e831e0b9f954ded1b25e06a5f9e3287c
)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_Declare(
        nameof
        GIT_REPOSITORY https://github.com/Neargye/nameof.git
        GIT_TAG c04b72fc51e4ac75f55462632de000c79a7b65b5
)
FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG f48bd0bfb9b00ed1f5b13dbcdbd7909ca8bd49b5
)
FetchContent_MakeAvailable(sdl2 spdlog glm physfs json unordered_dense nameof tinyobjloader)
add_subdirectory(core)
add_subdirectory(client)
