cmake_minimum_required(VERSION 3.16)

option(WITH_TLS "Compile with TLS support" OFF)

if(WITH_TLS)
	add_definitions(-DWITH_TLS)
endif(WITH_TLS)


# minimum component require
set(COMPONENTS "main freertos esp32-camera esp_psram esp_wifi mqtt")
message(STATUS "Compiling with tls: ${WITH_TLS}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32_camera)

if(WITH_TLS)
	target_add_binary_data(esp32_camera.elf "main/ca.pem" TEXT)
endif(WITH_TLS)