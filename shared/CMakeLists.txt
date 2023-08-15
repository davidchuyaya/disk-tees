cmake_minimum_required(VERSION 3.14)  # CMake version check
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++2a")

add_library(shared tls.cpp tls.hpp network_config.cpp network_config.hpp)

# Use OpenSSL for hashing, encryption, random number generation, and TLS networking
find_package(OpenSSL REQUIRED)

# Use pthreads for multithreading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# JSON library for figuring out IP addresses of peers. FetchContent downloads the library for us.
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
# ZPP library for fast serialization for networking
FetchContent_Declare(zpp URL https://github.com/eyalz800/zpp_bits/archive/refs/tags/v4.4.18.tar.gz)
FetchContent_MakeAvailable(json zpp)
# Based on how this repo imports zpp: https://github.com/AlpineMapsOrg/renderer/blob/87a51e590eddf2f3a9b0954789380eb116cea8df/CMakeLists.txt#L86
add_library(zpp INTERFACE)
target_include_directories(zpp INTERFACE ${zpp_SOURCE_DIR})

target_link_libraries(shared OpenSSL::Crypto OpenSSL::SSL Threads::Threads nlohmann_json::nlohmann_json zpp)