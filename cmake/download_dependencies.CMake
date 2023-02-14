include(FetchContent)

FetchContent_Declare(
        sdl2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG 55b03c7493a7abed33cf803d1380a40fa8af903f
        FIND_PACKAGE_ARGS NAMES SDL2
)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
)
FetchContent_Declare(
        unordered_dense
        GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
        GIT_TAG a7ece061e831e0b9f954ded1b25e06a5f9e3287c
)
set(SOL2_LUA_VERSION "LuaJIT" CACHE STRING "The version of Lua needed. Can be 5.1, 5.2, 5.3, 5.4, LuaJIT, or a more specific 3-part version number for a specifc Lua (e.g., 5.4.4 or luajit-2.0.5)" )
FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG eba86625b707e3c8c99bbfc4624e51f42dc9e561
)
FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG f48bd0bfb9b00ed1f5b13dbcdbd7909ca8bd49b5
)
FetchContent_Declare(
        concurrentQueue
        GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
        GIT_TAG 3747268264d0fa113e981658a99ceeae4dad05b7
)