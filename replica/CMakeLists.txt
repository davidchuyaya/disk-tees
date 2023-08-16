# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp replica_fuse.cpp replica_fuse.hpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(disk_tees ${SOURCE_FILES})

target_link_libraries(disk_tees LINK_PUBLIC shared)
