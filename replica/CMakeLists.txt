cmake_minimum_required(VERSION 3.14)  # CMake version check
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++2a")
project(disk_tees)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp fuse.cpp fuse.hpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(disk_tees ${SOURCE_FILES})

target_link_libraries(disk_tees LINK_PUBLIC shared)
