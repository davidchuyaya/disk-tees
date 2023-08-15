cmake_minimum_required(VERSION 3.14)  # CMake version check
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++2a")
project(tee_fuse)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(tee_fuse ${SOURCE_FILES})

# FUSE library
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse3)
target_include_directories(tee_fuse PUBLIC ${FUSE_INCLUDE_DIRS})
target_link_directories(tee_fuse PUBLIC ${FUSE_LIBRARY_DIRS})

target_link_libraries(tee_fuse LINK_PUBLIC shared ${FUSE_LIBRARIES})


