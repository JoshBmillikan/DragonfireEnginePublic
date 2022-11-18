
add_library(Core include/crash.h include/core.h include/dynamic_array.h src/game.cpp include/game.h src/file.cpp include/file.h src/config.cpp include/config.h include/asset.h include/util.h src/util.cpp src/asset.cpp)
target_link_libraries(Core PUBLIC spdlog glm PhysFS::PhysFS-static nlohmann_json::nlohmann_json unordered_dense::unordered_dense nameof)
set(DEV_ASSET_DIR ${CMAKE_SOURCE_DIR}/assets)
target_compile_definitions(Core PUBLIC ASSET_PATH="${DEV_ASSET_DIR}" GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
target_include_directories(Core PUBLIC include)
target_precompile_headers(Core PUBLIC <memory> <glm/glm.hpp> <glm/gtx/quaternion.hpp> include/core.h include/crash.h)