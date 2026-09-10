#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include "vulkan_types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace GameTools {
namespace Vulkan {

// Memory pool configuration per vendor
struct MemoryPoolConfig {
    size_t poolSize;          // Size of each pool block
    size_t alignment;         // Memory alignment
    bool preferDedicated;     // Prefer dedicated allocations
    bool useDefragmentation;  // Enable defragmentation
    uint32_t maxPools;        // Maximum number of pools
    
    static MemoryPoolConfig GetNvidiaConfig();
    static MemoryPoolConfig GetAmdConfig();
    static MemoryPoolConfig GetIntelConfig();
    static MemoryPoolConfig GetGenericConfig();
};

// Allocation result
struct AllocationResult {
    VkDeviceMemory memory;
    void* mappedData;
    size_t offset;
    size_t size;
    bool success;
};

// Memory allocation statistics
struct MemoryStats {
    size_t totalAllocated;
    size_t totalMapped;
    size_t fragmentationRatio;
    uint32_t activeAllocations;
    uint32_t poolCount;
};

// Memory allocator with vendor-specific optimizations
class MemoryAllocator {
public:
    MemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice, 
                   GPUVendor vendor, const MemoryPoolConfig& config);
    ~MemoryAllocator();
    
    // Allocate memory for a resource
    AllocationResult Allocate(size_t size, size_t alignment, 
                             MemoryType type, VkBuffer buffer = VK_NULL_HANDLE);
    
    // Free allocated memory
    void Free(VkDeviceMemory memory, size_t offset, size_t size);
    
    // Map memory for CPU access
    void* Map(VkDeviceMemory memory, size_t offset, size_t size);
    
    // Unmap memory
    void Unmap(VkDeviceMemory memory);
    
    // Defragment memory (vendor-optimized)
    void Defragment();
    
    // Get statistics
    MemoryStats GetStats() const;
    
    // Bind buffer to memory
    bool BindBuffer(VkBuffer buffer, VkDeviceMemory memory, size_t offset);
    
    // Bind image to memory
    bool BindImage(VkImage image, VkDeviceMemory memory, size_t offset);

private:
    struct PoolBlock {
        VkDeviceMemory memory;
        size_t size;
        size_t used;
        std::vector<std::pair<size_t, size_t>> allocations; // offset, size
    };
    
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    GPUVendor m_vendor;
    MemoryPoolConfig m_config;
    
    std::unordered_map<MemoryType, std::vector<PoolBlock>> m_pools;
    std::unordered_map<VkDeviceMemory, size_t> m_memoryMap; // memory -> pool index
    mutable std::mutex m_mutex;
    
    // Vendor-specific optimization methods
    uint32_t FindOptimalMemoryType(VkMemoryRequirements requirements, 
                                   MemoryType preferredType);
    void OptimizeAllocationNvidia(PoolBlock& block);
    void OptimizeAllocationAmd(PoolBlock& block);
    void DefragmentNvidia();
    void DefragmentAmd();
};

} // namespace Vulkan
} // namespace GameTools

#endif // MEMORY_ALLOCATOR_H
