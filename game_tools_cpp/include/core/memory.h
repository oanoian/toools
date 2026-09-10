// include/core/memory.h
#pragma once
#include "core/types.h"
#include <cstring>
#include <new>
#include <vector>
#include <array>

namespace game_tools {

// Cache line size for different architectures
constexpr size_t CACHE_LINE_SIZE = 64;

// Aligned memory allocation
inline void* aligned_alloc(size_t alignment, size_t size) {
    void* ptr = nullptr;
#if defined(_MSC_VER)
    ptr = _aligned_malloc(size, alignment);
#else
    ::posix_memalign(&ptr, alignment, size);
#endif
    return ptr;
}

inline void aligned_free(void* ptr) {
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// Arena allocator for fast, contiguous memory allocation
class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t capacity) 
        : buffer_(new u8[capacity]), capacity_(capacity), offset_(0) {}
    
    ~ArenaAllocator() {
        delete[] buffer_;
    }
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        // Align the current offset
        size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);
        
        if (aligned_offset + size > capacity_) {
            return nullptr; // Out of memory
        }
        
        void* ptr = buffer_ + aligned_offset;
        offset_ = aligned_offset + size;
        allocations_++;
        total_allocated_ += size;
        return ptr;
    }
    
    void reset() {
        offset_ = 0;
        allocations_ = 0;
        total_allocated_ = 0;
    }
    
    size_t get_capacity() const { return capacity_; }
    size_t get_used() const { return offset_; }
    size_t get_allocation_count() const { return allocations_; }
    size_t get_total_allocated() const { return total_allocated_; }
    
private:
    u8* buffer_;
    size_t capacity_;
    size_t offset_;
    size_t allocations_ = 0;
    size_t total_allocated_ = 0;
};

// Pool allocator for fixed-size objects
template<typename T, size_t BlockSize = 256>
class PoolAllocator {
public:
    PoolAllocator() {
        allocate_block();
    }
    
    ~PoolAllocator() {
        for (auto* block : blocks_) {
            aligned_free(block);
        }
    }
    
    T* allocate() {
        if (free_list_ == nullptr) {
            allocate_block();
        }
        
        T* ptr = free_list_;
        free_list_ = reinterpret_cast<T*>(*reinterpret_cast<u32**>(free_list_));
        construction_count_++;
        return ptr;
    }
    
    void deallocate(T* ptr) {
        *reinterpret_cast<u32**>(ptr) = free_list_;
        free_list_ = ptr;
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }
    
    void destroy(T* ptr) {
        ptr->~T();
        deallocate(ptr);
    }
    
    size_t get_block_count() const { return blocks_.size(); }
    size_t get_construction_count() const { return construction_count_; }
    
private:
    void allocate_block() {
        void* block = aligned_alloc(CACHE_LINE_SIZE, BlockSize * sizeof(T));
        blocks_.push_back(static_cast<u8*>(block));
        
        // Initialize free list
        for (size_t i = 0; i < BlockSize - 1; ++i) {
            T* obj = reinterpret_cast<T*>(static_cast<u8*>(block) + i * sizeof(T));
            *reinterpret_cast<u32**>(obj) = reinterpret_cast<T*>(
                static_cast<u8*>(block) + (i + 1) * sizeof(T)
            );
        }
        
        T* last_obj = reinterpret_cast<T*>(
            static_cast<u8*>(block) + (BlockSize - 1) * sizeof(T)
        );
        *reinterpret_cast<u32**>(last_obj) = free_list_;
        free_list_ = reinterpret_cast<T*>(block);
    }
    
    std::vector<u8*> blocks_;
    T* free_list_ = nullptr;
    size_t construction_count_ = 0;
};

// SIMD-aligned math types
struct alignas(16) Vec4 {
    f32 x, y, z, w;
    
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
};

struct alignas(16) Mat4 {
    f32 m[16];
    
    Mat4() {
        memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f; // Identity
    }
};

} // namespace game_tools
