cmake_minimum_required(VERSION 3.13)  # CMake version check
project(disk_tees)
set(CMAKE_CXX_STANDARD 17)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp tls.cpp tls.hpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(disk_tees ${SOURCE_FILES})

# Use OpenSSL for hashing, encryption, random number generation, and TLS networking
find_package(OpenSSL REQUIRED)
target_link_libraries(disk_tees OpenSSL::Crypto OpenSSL::SSL)