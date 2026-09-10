cmake_minimum_required(VERSION 3.16)
project(Alpha1 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -march=native -flto -fvisibility=hidden)
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/O2 /GL /arch:AVX2 /LTCG /W4)
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Thread support
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

# Source files
set(ALPHA1_SOURCES
    src/core/logging.cpp
    src/core/threading.cpp
    src/ecs/world.cpp
    src/asset/asset_manager.cpp
    src/asset/vfs.cpp
    src/asset/cooker.cpp
    src/dcc/usd_pipeline.cpp
    src/dcc/live_link.cpp
    src/vcs/version_control.cpp
    src/renderer/vulkan_engine.cpp
    src/renderer/nvidia_optimizations.cpp
    src/renderer/amd_optimizations.cpp
    src/dll/dll_manager.cpp
    src/dll/dependency_tracker.cpp
    src/dll/hot_reloader.cpp
    src/alpha1.cpp
)

# Create library
add_library(alpha1 SHARED ${ALPHA1_SOURCES})
target_link_libraries(alpha1 
    ${Vulkan_LIBRARIES}
    Threads::Threads
)

# Set library properties
set_target_properties(alpha1 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/alpha1/alpha1.h
)

# Tests
enable_testing()

# Test framework (doctest - single header)
add_executable(alpha1_tests tests/main.cpp)
target_link_libraries(alpha1_tests alpha1)
target_include_directories(alpha1_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
add_test(NAME Alpha1Tests COMMAND alpha1_tests)

# Benchmark harness
add_executable(alpha1_bench bench/ecs_benchmark.cpp)
target_link_libraries(alpha1_bench alpha1)
target_include_directories(alpha1_bench PRIVATE ${CMAKE_SOURCE_DIR}/tests)

# Sanitizer support (Task 1.1)
option(ALPHA1_ENABLE_SANITIZERS "Enable ASan/UBSan in Debug builds" ON)
if(ALPHA1_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Installation
install(TARGETS alpha1
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/alpha1
)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Alpha1 Framework Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Vulkan: ${Vulkan_FOUND}")
