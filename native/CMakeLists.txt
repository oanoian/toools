cmake_minimum_required(VERSION 3.20)
project(GameStudioNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Produces GameStudioNative.dll (Windows) / libGameStudioNative.so (Linux/macOS).
add_library(GameStudioNative SHARED
    src/gsd_algorithms.cpp)

target_include_directories(GameStudioNative PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Keep the output name stable across platforms so the C# DllImport("GameStudioNative")
# resolves consistently.
set_target_properties(GameStudioNative PROPERTIES
    OUTPUT_NAME "GameStudioNative"
    PREFIX ""            # avoid the "lib" prefix on *nix so P/Invoke name matches
    WINDOWS_EXPORT_ALL_SYMBOLS OFF)

if (MSVC)
    target_compile_options(GameStudioNative PRIVATE /O2 /W4)
else()
    target_compile_options(GameStudioNative PRIVATE -O2 -Wall -Wextra)
endif()
