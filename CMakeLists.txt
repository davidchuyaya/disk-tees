cmake_minimum_required(VERSION 3.13)  # CMake version check
project(disk_tees)
set(CMAKE_CXX_STANDARD 20)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp tls.cpp tls.hpp network_config.cpp network_config.hpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(disk_tees ${SOURCE_FILES})

# Use OpenSSL for hashing, encryption, random number generation, and TLS networking
find_package(OpenSSL REQUIRED)

# Use pthreads for multithreading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# JSON library for figuring out IP addresses of peers. FetchContent downloads the library for us.
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(disk_tees OpenSSL::Crypto OpenSSL::SSL Threads::Threads)