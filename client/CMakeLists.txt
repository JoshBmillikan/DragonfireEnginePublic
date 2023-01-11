
add_subdirectory(rendering)

add_library(Client src/local_game.h src/local_game.cpp rendering/src/renderer.cpp rendering/src/renderer.h src/input.cpp src/input.h src/world/local_world.cpp src/world/local_world.h)
add_executable(Game src/main.cpp)
target_link_libraries(Game SDL2::SDL2main Client)
target_include_directories(Client PUBLIC include)
target_link_libraries(Client PUBLIC SDL2::SDL2 Core Renderer)