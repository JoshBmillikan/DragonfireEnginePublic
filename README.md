# DragonfireEngine

Work in progress c++ vulkan game/rendering engine

### Compiling ###
1. Check out the repository
   
    Note: use ``git clone --recursive`` to ensure git submodules are also cloned

2. install the [Vulkan sdk](https://vulkan.lunarg.com/) 

    Note: it must be installed in a way that is visible to cmake's find_package command, this may require setting the VULKAN_SDK environment variable if it is installed in a non-default location.

3. configure and build with cmake

(The windows build has currently not been tested and may or may not work correctly)