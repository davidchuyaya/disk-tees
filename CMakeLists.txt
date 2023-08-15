cmake_minimum_required(VERSION 3.14)  # CMake version check
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++2a")
project(rollbackcine)

add_subdirectory(shared)
add_subdirectory(replica)
add_subdirectory(client)