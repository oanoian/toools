/**
 * Alpha1 Core Types
 * 
 * Fundamental type definitions, error handling, and platform abstractions.
 * 
 * @file types.h
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <functional>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define A1_PLATFORM_WINDOWS 1
    #define A1_API __declspec(dllexport)
#elif defined(__linux__)
    #define A1_PLATFORM_LINUX 1
    #define A1_API __attribute__((visibility("default")))
#elif defined(__APPLE__)
    #define A1_PLATFORM_MACOS 1
    #define A1_API __attribute__((visibility("default")))
#else
    #error "Unsupported platform"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define A1_COMPILER_MSVC 1
    #define A1_FORCE_INLINE __forceinline
#elif defined(__clang__)
    #define A1_COMPILER_CLANG 1
    #define A1_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(__GNUC__)
    #define A1_COMPILER_GCC 1
    #define A1_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define A1_FORCE_INLINE inline
#endif

namespace Alpha1::Core {

/**
 * Unique entity identifier
 */
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY_ID = UINT32_MAX;

/**
 * Component type identifier
 */
using ComponentTypeID = uint32_t;

/**
 * Asset handle
 */
using AssetHandle = uint64_t;
constexpr AssetHandle INVALID_ASSET_HANDLE = UINT64_MAX;

/**
 * Result type for error handling
 */
template<typename T>
class Result {
public:
    Result(T value) : m_value(std::move(value)), m_success(true) {}
    Result(const char* error) : m_error(error), m_success(false) {}
    
    bool IsSuccess() const { return m_success; }
    bool IsError() const { return !m_success; }
    
    const T& Value() const { return m_value; }
    T& Value() { return m_value; }
    
    const std::string& Error() const { return m_error; }
    
    explicit operator bool() const { return m_success; }
    
private:
    variant<T, string> m_data;
    T m_value;
    std::string m_error;
    bool m_success;
};

/**
 * Specialization for void results
 */
template<>
class Result<void> {
public:
    Result() : m_success(true) {}
    Result(const char* error) : m_error(error), m_success(false) {}
    
    bool IsSuccess() const { return m_success; }
    bool IsError() const { return !m_success; }
    
    const std::string& Error() const { return m_error; }
    
    explicit operator bool() const { return m_success; }
    
private:
    std::string m_error;
    bool m_success;
};

/**
 * High-resolution clock type
 */
using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

/**
 * Time delta in seconds
 */
using DeltaTime = float;

/**
 * Log level enumeration
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * Thread ID type
 */
using ThreadID = uint64_t;

/**
 * Memory size type
 */
using MemorySize = size_t;

/**
 * Index type for arrays
 */
using IndexType = size_t;

} // namespace Alpha1::Core

// Common type aliases
namespace Alpha1 {
    using Core::EntityID;
    using Core::ComponentTypeID;
    using Core::AssetHandle;
    using Core::Result;
    using Core::DeltaTime;
    using Core::LogLevel;
    using Core::ThreadID;
    using Core::MemorySize;
    using Core::IndexType;
    
    constexpr EntityID INVALID_ENTITY = Core::INVALID_ENTITY_ID;
    constexpr AssetHandle INVALID_ASSET = Core::INVALID_ASSET_HANDLE;
}
