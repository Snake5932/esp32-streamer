project(test_camera_client)
cmake_minimum_required(VERSION 3.16)

find_library(mosquitto REQUIRED)

add_executable(test test.c)
add_executable(recv recv.c)

target_link_libraries(test PRIVATE mosquitto)
target_link_libraries(recv PRIVATE mosquitto)
