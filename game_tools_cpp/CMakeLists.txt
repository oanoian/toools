cmake_minimum_required(VERSION 3.16)
project(GameToolsCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/vulkan_types.cpp
    src/memory_allocator.cpp
    src/nvidia_optimizations.cpp
    src/amd_optimizations.cpp
    src/vulkan_optimization_manager.cpp
)

# Create library
add_library(gametools_cpp STATIC ${SOURCES})
target_link_libraries(gametools_cpp Vulkan::Vulkan)

# Set compiler warnings
if(MSVC)
    target_compile_options(gametools_cpp PRIVATE /W4)
else()
    target_compile_options(gametools_cpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Example executable
add_executable(vulkan_demo src/main.cpp)
target_link_libraries(vulkan_demo gametools_cpp Vulkan::Vulkan)

# Install targets
install(TARGETS gametools_cpp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
