
add_subdirectory(rendering)

add_library(Client src/game_client.h src/game_client.cpp rendering/src/renderer.cpp rendering/src/renderer.h)
add_executable(Game src/main.cpp)
target_link_libraries(Game SDL2::SDL2main Client)
target_link_libraries(Client PUBLIC SDL2::SDL2 Core Renderer png_static)