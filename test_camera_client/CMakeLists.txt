project(test_camera_client)
cmake_minimum_required(VERSION 3.16)

find_library(mosquitto REQUIRED)

add_executable(emu emu.c)
add_executable(recv recv.c)

target_link_libraries(emu PRIVATE mosquitto)
target_link_libraries(recv PRIVATE mosquitto)
