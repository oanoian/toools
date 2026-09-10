cmake_minimum_required(VERSION 3.16)
project(GameTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_VULKAN "Enable Vulkan support" ON)
option(ENABLE_DLL_OPTIMIZATION "Enable DLL optimization features" ON)

# Find packages
if(ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

# Source files
set(GAME_TOOLS_SOURCES
    src/dll_optimization/dll_manager.cpp
)

set(GAME_TOOLS_HEADERS
    include/dll_optimization/dll_manager.hpp
)

# If Vulkan enabled, add Vulkan sources
if(ENABLE_VULKAN)
    list(APPEND GAME_TOOLS_SOURCES
        src/vulkan_engine/vulkan_device.cpp
        src/vulkan_engine/vulkan_memory.cpp
        src/vulkan_engine/vulkan_command.cpp
        src/vulkan_engine/vulkan_renderer.cpp
        src/vulkan_engine/nvidia_optimizations.cpp
        src/vulkan_engine/amd_optimizations.cpp
        src/vulkan_engine/vulkan_optimization_manager.cpp
    )
    
    list(APPEND GAME_TOOLS_HEADERS
        include/vulkan_engine/vulkan_device.hpp
        include/vulkan_engine/vulkan_memory.hpp
        include/vulkan_engine/vulkan_command.hpp
        include/vulkan_engine/vulkan_renderer.hpp
        include/vulkan_engine/nvidia_optimizations.hpp
        include/vulkan_engine/amd_optimizations.hpp
        include/vulkan_engine/vulkan_optimization_manager.hpp
    )
endif()

# Create library
add_library(gametools ${GAME_TOOLS_SOURCES} ${GAME_TOOLS_HEADERS})

target_include_directories(gametools PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(gametools PRIVATE
        psapi
        imagehlp
        dbghelp
    )
    
    target_compile_definitions(gametools PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Vulkan linkage
if(ENABLE_VULKAN AND Vulkan_FOUND)
    target_link_libraries(gametools PUBLIC Vulkan::Vulkan)
    target_compile_definitions(gametools PUBLIC GAMETOOLS_VULKAN_ENABLED)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(gametools PRIVATE /W4 /WX-)
else()
    target_compile_options(gametools PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Optimization flags for different vendors
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC optimizations
    target_compile_options(gametools PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/arch:AVX2>
    )
    
    # Link time optimization for Release
    target_link_options(gametools PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang optimizations
    target_compile_options(gametools PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Release>:-flto>
    )
endif()

# Build tests if enabled
if(BUILD_TESTS)
    enable_testing()
    
    # DLL Optimization Tests
    if(ENABLE_DLL_OPTIMIZATION)
        add_executable(test_dll_opt tests/test_dll_optimization.cpp)
        target_link_libraries(test_dll_opt PRIVATE gametools)
        target_include_directories(test_dll_opt PRIVATE include)
        
        add_test(NAME DLL_Optimization_Tests COMMAND test_dll_opt)
    endif()
    
    # Vulkan Tests
    if(ENABLE_VULKAN)
        add_executable(test_vulkan tests/test_vulkan_engine.cpp)
        target_link_libraries(test_vulkan PRIVATE gametools Vulkan::Vulkan)
        target_include_directories(test_vulkan PRIVATE include)
        
        add_test(NAME Vulkan_Engine_Tests COMMAND test_vulkan)
    endif()
endif()

# Installation
install(TARGETS gametools
    EXPORT GameToolsTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT GameToolsTargets
    FILE GameToolsTargets.cmake
    NAMESPACE GameTools::
    DESTINATION lib/cmake/GameTools
)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
