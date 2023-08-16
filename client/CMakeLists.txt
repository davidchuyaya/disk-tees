# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp Fuse.hpp Fuse.cpp client_fuse.hpp client_fuse.cpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(tee_fuse ${SOURCE_FILES})

target_link_libraries(tee_fuse LINK_PUBLIC shared)