// include/core/types.h
#pragma once
#include <cstdint>
#include <cstddef>

namespace game_tools {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

using EntityID = u32;
constexpr EntityID INVALID_ENTITY = 0xFFFFFFFF;

// Result type for error handling without exceptions in hot paths
template<typename T>
struct Result {
    T value;
    bool success;
    const char* error_msg;

    static Result Ok(T val) { return {val, true, nullptr}; }
    static Result Err(const char* msg) { return {{}, false, msg}; }
};

} // namespace game_tools
