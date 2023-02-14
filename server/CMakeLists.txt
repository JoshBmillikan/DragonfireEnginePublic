
add_executable(Server src/server.cpp src/server.h src/main.cpp)
target_link_libraries(Server PUBLIC Core)