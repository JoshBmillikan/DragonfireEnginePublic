add_library(Core STATIC include/core.h src/core.cpp src/file.cpp include/file.h src/allocators.cpp include/allocators.h src/config.cpp include/config.h src/utility.cpp include/utility.h src/world.cpp include/world.h include/math.h src/rng.cpp include/rng.h)
target_include_directories(Core PUBLIC include)
target_link_libraries(Core PUBLIC spdlog::spdlog EnTT SDL2::SDL2 glm::glm PhysFS::PhysFS-static unordered_dense::unordered_dense nlohmann_json sqlite BSThreadPool)
target_compile_definitions(Core PUBLIC "APP_NAME=\"${APP_NAME}\"" "APP_ID=\"${APP_ID}\"" "ASSET_PATH=\"${ASSET_DIR}\"" GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
target_precompile_headers(Core PUBLIC <vector> <memory> <string> <core.h> <spdlog/spdlog.h>)