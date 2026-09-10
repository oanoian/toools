/**
 * Alpha1 Memory Management System
 * 
 * High-performance memory allocators optimized for game development:
 * - Arena allocator for temporary allocations
 * - Pool allocator for fixed-size objects
 * - SIMD-aligned math types
 * - Cache-line optimized data structures
 * 
 * @file memory.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <cstring>
#include <new>
#include <array>

#ifdef A1_PLATFORM_WINDOWS
    #include <intrin.h>
#endif

#if defined(A1_COMPILER_GCC) || defined(A1_COMPILER_CLANG)
    #include <x86intrin.h>
#endif

namespace Alpha1::Core {

// Cache line size for target architecture
constexpr size_t CACHE_LINE_SIZE = 64;

/**
 * SIMD-aligned 4D vector for high-performance math
 */
struct alignas(16) Vec4 {
    float x, y, z, w;
    
    A1_FORCE_INLINE Vec4() : x(0), y(0), z(0), w(0) {}
    A1_FORCE_INLINE Vec4(float _x, float _y, float _z, float _w) 
        : x(_x), y(_y), z(_z), w(_w) {}
    
    // SIMD operations would be implemented here
    A1_FORCE_INLINE Vec4 operator+(const Vec4& other) const {
        return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    
    A1_FORCE_INLINE Vec4 operator-(const Vec4& other) const {
        return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    
    A1_FORCE_INLINE Vec4 operator*(float scalar) const {
        return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
    }
};

/**
 * SIMD-aligned 4x4 matrix for transformations
 */
struct alignas(16) Mat4 {
    std::array<Vec4, 4> rows;
    
    A1_FORCE_INLINE Mat4() {
        // Identity matrix
        rows[0] = Vec4(1, 0, 0, 0);
        rows[1] = Vec4(0, 1, 0, 0);
        rows[2] = Vec4(0, 0, 1, 0);
        rows[3] = Vec4(0, 0, 0, 1);
    }
};

/**
 * Arena Allocator - Linear bump allocator for temporary allocations
 * 
 * Extremely fast O(1) allocation, ideal for frame-temporary data.
 * Memory is freed all at once when the arena is reset.
 */
class ArenaAllocator {
public:
    explicit ArenaAllocator(MemorySize capacity) 
        : m_capacity(capacity), m_offset(0) {
        m_buffer = static_cast<uint8_t*>(aligned_alloc(CACHE_LINE_SIZE, capacity));
        if (!m_buffer) {
            throw std::bad_alloc();
        }
    }
    
    ~ArenaAllocator() {
        free(m_buffer);
    }
    
    // Non-copyable
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    
    /**
     * Allocate memory from the arena
     */
    void* Allocate(MemorySize size, MemorySize alignment = alignof(std::max_align_t)) {
        // Align the current offset
        MemorySize aligned_offset = (m_offset + alignment - 1) & ~(alignment - 1);
        
        if (aligned_offset + size > m_capacity) {
            return nullptr; // Out of memory
        }
        
        void* ptr = m_buffer + aligned_offset;
        m_offset = aligned_offset + size;
        return ptr;
    }
    
    /**
     * Reset the arena (free all allocations)
     */
    void Reset() {
        m_offset = 0;
    }
    
    /**
     * Get remaining capacity
     */
    MemorySize Remaining() const {
        return m_capacity - m_offset;
    }
    
    /**
     * Get current usage
     */
    MemorySize Used() const {
        return m_offset;
    }
    
private:
    uint8_t* m_buffer;
    MemorySize m_capacity;
    MemorySize m_offset;
};

/**
 * Pool Allocator - Fixed-size block allocator
 * 
 * O(1) allocation and deallocation, zero fragmentation.
 * Ideal for frequently allocated/deallocated objects of the same size.
 */
template<typename T, size_t BlockSize = 256>
class PoolAllocator {
public:
    using ValueType = T;
    using Pointer = T*;
    using SizeType = size_t;
    
    PoolAllocator() : m_currentBlock(nullptr), m_freeList(nullptr) {
        AllocateBlock();
    }
    
    ~PoolAllocator() {
        // Free all blocks
        Block* next = m_firstBlock;
        while (next) {
            Block* current = next;
            next = current->next;
            free(current);
        }
    }
    
    // Non-copyable
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    
    /**
     * Allocate an object from the pool
     */
    T* Allocate() {
        if (!m_freeList) {
            AllocateBlock();
        }
        
        Slot* slot = m_freeList;
        m_freeList = slot->next;
        
        // Construct object in place
        return new (&slot->data) T();
    }
    
    /**
     * Deallocate an object back to the pool
     */
    void Deallocate(T* ptr) {
        // Call destructor
        ptr->~T();
        
        // Return slot to free list
        Slot* slot = reinterpret_cast<Slot*>(ptr);
        slot->next = m_freeList;
        m_freeList = slot;
    }
    
    /**
     * Get total number of allocated objects
     */
    SizeType Size() const {
        return m_size;
    }
    
    /**
     * Get number of free slots
     */
    SizeType FreeCount() const {
        SizeType count = 0;
        Slot* slot = m_freeList;
        while (slot) {
            ++count;
            slot = slot->next;
        }
        return count;
    }
    
private:
    struct Slot {
        union {
            Slot* next;
            alignas(T) uint8_t data[sizeof(T)];
        };
    };
    
    struct Block {
        Block* next;
        std::array<Slot, BlockSize> slots;
    };
    
    void AllocateBlock() {
        Block* block = static_cast<Block*>(
            aligned_alloc(CACHE_LINE_SIZE, sizeof(Block))
        );
        
        if (!block) {
            throw std::bad_alloc();
        }
        
        block->next = nullptr;
        
        // Link all slots in the block to the free list
        for (size_t i = 0; i < BlockSize - 1; ++i) {
            block->slots[i].next = &block->slots[i + 1];
        }
        block->slots[BlockSize - 1].next = m_freeList;
        m_freeList = &block->slots[0];
        
        // Add block to the block list
        if (!m_firstBlock) {
            m_firstBlock = m_currentBlock = block;
        } else {
            m_currentBlock->next = block;
            m_currentBlock = block;
        }
        
        m_size += BlockSize;
    }
    
    Block* m_firstBlock;
    Block* m_currentBlock;
    Slot* m_freeList;
    SizeType m_size;
};

/**
 * Cache-line padded wrapper to prevent false sharing
 */
template<typename T>
struct alignas(CACHE_LINE_SIZE) CachePadded {
    T value;
    char padding[CACHE_LINE_SIZE - sizeof(T) % CACHE_LINE_SIZE];
    
    CachePadded() : value() {}
    CachePadded(const T& v) : value(v) {}
    
    operator T&() { return value; }
    operator const T&() const { return value; }
};

/**
 * Aligned allocation helper
 */
inline void* AlignedAlloc(MemorySize size, MemorySize alignment) {
    return aligned_alloc(alignment, size);
}

/**
 * Aligned deallocation helper
 */
inline void AlignedFree(void* ptr) {
    free(ptr);
}

} // namespace Alpha1::Core
