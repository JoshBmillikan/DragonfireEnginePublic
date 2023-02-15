
if (WIN32)
    set(PLATFORM_SRC src/platform/windows.cpp)
elseif(UNIX)
    set(PLATFORM_SRC src/platform/linux.cpp)
endif()

add_library(Core include/crash.h include/core.h include/dynamic_array.h src/game.cpp include/game.h src/file.cpp include/file.h src/config.cpp
        include/config.h include/asset.h include/util.h src/util.cpp src/asset.cpp include/transform.h src/thread_pool.cpp include/thread_pool.h
        include/math.h src/event.cpp include/event.h src/physics.cpp include/physics.h src/world/world.cpp include/world/world.h src/voxel/chunk.cpp
        include/voxel/chunk.h src/rng.cpp include/rng.h src/voxel/voxel.cpp include/voxel/voxel.h src/noise.cpp include/noise.h src/controller.cpp
        include/controller.h include/platform.h ${PLATFORM_SRC} src/net.cpp include/net.h)
target_link_libraries(Core PUBLIC CONAN_PKG::spdlog glm CONAN_PKG::physfs CONAN_PKG::nlohmann_json unordered_dense::unordered_dense concurrentqueue CONAN_PKG::entt CONAN_PKG::bullet3 CONAN_PKG::luajit sol2::sol2)
set(DEV_ASSET_DIR ${CMAKE_SOURCE_DIR}/assets)
target_compile_definitions(Core PUBLIC ASSET_PATH="${DEV_ASSET_DIR}" GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
target_include_directories(Core PUBLIC include)
target_precompile_headers(Core PUBLIC <spdlog/spdlog.h> <memory> <vector> <string> <glm/glm.hpp> <glm/gtx/quaternion.hpp> include/core.h include/crash.h include/math.h)