# DragonfireEngine

Work in progress c++ vulkan game/rendering engine

### Compiling ###

1. #### Check out the repository

   Note: use ``git clone --recursive`` to ensure git submodules are also cloned

2. #### Install dependencies #### 
   These libraries must be installed and visible to cmake's find_package command, all others will be automatically downloaded by cmake
    * [The Vulkan sdk](https://vulkan.lunarg.com/)
    * [FreeType](https://freetype.org/index.html)

3. #### Configure and build with cmake

(The windows build has currently not been tested and may or may not work correctly)

### Libraries in use ###

| Name                        | Link                                                              | Usage                           | Licenses                                                                                                 |
|-----------------------------|-------------------------------------------------------------------|---------------------------------|----------------------------------------------------------------------------------------------------------|
| Vulkan                      | https://vulkan.lunarg.com                                         | Graphics API                    | [License Summary](https://vulkan.lunarg.com/software/license/vulkan-1.3.239.0-linux-license-summary.txt) |
| FreeType                    | https://freetype.org/index.html                                   | Font rendering                  | [FreeType License](https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/docs/FTL.TXT)          |
| SDL2                        | https://www.libsdl.org/                                           | Window creation/general utility | zlib                                                                                                     |
 | spdlog                      | https://github.com/gabime/spdlog                                  | Logging                         | MIT                                                                                                      |  
 | GLM                         | https://github.com/g-truc/glm                                     | Mathematics                     | MIT                                                                                                      |
 | PhysFS                      | https://icculus.org/physfs/                                       | Filesystem abstraction          | zlib                                                                                                     |
 | EnTT                        | https://github.com/skypjack/entt                                  | ECS                             | MIT                                                                                                      |
 | ankerl::unordered_dense     | https://github.com/martinus/unordered_dense                       | A better hash map               | MIT                                                                                                      |
 | Vulkan Memory Allocator     | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | Vulkan memory management        | MIT                                                                                                      |
 | nlohmann::json              | https://github.com/nlohmann/json                                  | Nice JSON parser                | MIT                                                                                                      |
 | tinyobjloader               | https://github.com/tinyobjloader/tinyobjloader                    | Wavefront OBJ file parser       | MIT                                                                                                      |
 | moodycamel::ConcurrentQueue | https://github.com/cameron314/concurrentqueue                     | Lock-Free thread safe queue     | Simplified BSD/Boost License                                                                             |
 | Dear ImGui                  | https://github.com/ocornut/imgui                                  | Basic UI                        | MIT                                                                                                      |                                                                                                     |
 | LuaJIT                      | https://luajit.org/index.html                                     | Lua runtime                     | MIT                                                                                                      |
 | sol2                        | https://github.com/ThePhD/sol2/tree/main                          | C++ Lua bindings                | MIT                                                                                                      |