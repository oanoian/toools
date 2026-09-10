cmake_minimum_required(VERSION 3.16)
project(game_tools_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Vulkan
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# Compiler optimization flags for game development
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/O2 /GL /arch:AVX2)
    add_link_options(/LTCG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -march=native -flto)
    add_link_options(-flto)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

# Core library sources
set(CORE_SOURCES
    src/core/memory.cpp
)

# ECS sources
set(ECS_SOURCES
    src/ecs/entity.cpp
)

# Asset management sources
set(ASSET_SOURCES
    src/asset/asset_manager.cpp
)

# DCC/USD pipeline sources
set(DCC_SOURCES
    src/dcc/usd_pipeline.cpp
)

# VCS sources
set(VCS_SOURCES
    src/vcs/version_control.cpp
)

# Renderer with Vulkan optimization sources (existing)
set(RENDERER_SOURCES
    src/vulkan_types.cpp
    src/memory_allocator.cpp
    src/nvidia_optimizations.cpp
    src/amd_optimizations.cpp
    src/vulkan_optimization_manager.cpp
)

# Create main game tools library
add_library(gametools_cpp STATIC
    ${CORE_SOURCES}
    ${ECS_SOURCES}
    ${ASSET_SOURCES}
    ${DCC_SOURCES}
    ${VCS_SOURCES}
    ${RENDERER_SOURCES}
)

target_link_libraries(gametools_cpp
    Vulkan::Vulkan
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(gametools_cpp ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gametools_cpp rt)
endif()

# Set compiler warnings
if(MSVC)
    target_compile_options(gametools_cpp PRIVATE /W4)
else()
    target_compile_options(gametools_cpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test executable
add_executable(game_tools_test tests/main.cpp)
target_link_libraries(game_tools_test gametools_cpp Vulkan::Vulkan Threads::Threads)

# Demo executable (existing)
add_executable(vulkan_demo src/main.cpp)
target_link_libraries(vulkan_demo gametools_cpp Vulkan::Vulkan)

# Install targets
install(TARGETS gametools_cpp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Print configuration summary
message(STATUS "Game Tools C++ Configuration:")
message(STATUS "  C++ Standard: C++17")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Vulkan Found: ${Vulkan_FOUND}")
