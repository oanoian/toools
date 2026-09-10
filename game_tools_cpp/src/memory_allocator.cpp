#include "memory_allocator.h"
#include <algorithm>
#include <cstring>

namespace GameTools {
namespace Vulkan {

// MemoryPoolConfig implementations
MemoryPoolConfig MemoryPoolConfig::GetNvidiaConfig() {
    MemoryPoolConfig config;
    config.poolSize = 256 * 1024 * 1024; // 256MB pools
    config.alignment = 256;
    config.preferDedicated = true;
    config.useDefragmentation = true;
    config.maxPools = 32;
    return config;
}

MemoryPoolConfig MemoryPoolConfig::GetAmdConfig() {
    MemoryPoolConfig config;
    config.poolSize = 128 * 1024 * 1024; // 128MB pools
    config.alignment = 256;
    config.preferDedicated = false;
    config.useDefragmentation = true;
    config.maxPools = 64;
    return config;
}

MemoryPoolConfig MemoryPoolConfig::GetIntelConfig() {
    MemoryPoolConfig config;
    config.poolSize = 64 * 1024 * 1024; // 64MB pools
    config.alignment = 256;
    config.preferDedicated = false;
    config.useDefragmentation = false;
    config.maxPools = 16;
    return config;
}

MemoryPoolConfig MemoryPoolConfig::GetGenericConfig() {
    MemoryPoolConfig config;
    config.poolSize = 64 * 1024 * 1024; // 64MB pools
    config.alignment = 256;
    config.preferDedicated = false;
    config.useDefragmentation = false;
    config.maxPools = 16;
    return config;
}

// MemoryAllocator implementation
MemoryAllocator::MemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice,
                                GPUVendor vendor, const MemoryPoolConfig& config)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_vendor(vendor)
    , m_config(config) {
}

MemoryAllocator::~MemoryAllocator() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& poolPair : m_pools) {
        for (auto& block : poolPair.second) {
            vkFreeMemory(m_device, block.memory, nullptr);
        }
    }
    m_pools.clear();
    m_memoryMap.clear();
}

uint32_t MemoryAllocator::FindOptimalMemoryType(VkMemoryRequirements requirements,
                                                MemoryType preferredType) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);
    
    uint32_t memoryTypeBits = requirements.memoryTypeBits;
    uint32_t optimalType = UINT32_MAX;
    
    // Vendor-specific memory type selection heuristics
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) == 0) continue;
        
        VkMemoryPropertyFlags props = memProps.memoryTypes[i].propertyFlags;
        
        switch (preferredType) {
            case MemoryType::DEVICE_LOCAL:
                if (props & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    if (m_vendor == GPUVendor::NVIDIA && 
                        !(props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                        // NVIDIA prefers pure device-local for GPU-only resources
                        return i;
                    }
                    optimalType = i;
                }
                break;
                
            case MemoryType::HOST_VISIBLE:
                if ((props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    return i;
                }
                break;
                
            case MemoryType::HOST_COHERENT:
                if ((props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    return i;
                }
                break;
                
            case MemoryType::LAZILY_ALLOCATED:
                if (props & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                    return i;
                }
                break;
        }
    }
    
    // Fallback to first available type
    if (optimalType == UINT32_MAX) {
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if (memoryTypeBits & (1 << i)) {
                return i;
            }
        }
    }
    
    return optimalType;
}

AllocationResult MemoryAllocator::Allocate(size_t size, size_t alignment,
                                          MemoryType type, VkBuffer buffer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AllocationResult result;
    result.success = false;
    result.memory = VK_NULL_HANDLE;
    result.mappedData = nullptr;
    result.offset = 0;
    result.size = size;
    
    // Get memory requirements
    VkMemoryRequirements requirements = {};
    if (buffer != VK_NULL_HANDLE) {
        vkGetBufferMemoryRequirements(m_device, buffer, &requirements);
        size = std::max(size, requirements.size);
        alignment = std::max(alignment, requirements.alignment);
    } else {
        requirements.size = size;
        requirements.alignment = alignment;
        requirements.memoryTypeBits = 0xFFFFFFFF;
    }
    
    // Find optimal memory type
    uint32_t memoryTypeIndex = FindOptimalMemoryType(requirements, type);
    if (memoryTypeIndex == UINT32_MAX) {
        return result;
    }
    
    // Try to find space in existing pools
    auto& pools = m_pools[type];
    for (auto& pool : pools) {
        // Simple first-fit allocation
        for (size_t i = 0; i < pool.allocations.size(); ++i) {
            size_t gapStart = (i == 0) ? 0 : pool.allocations[i-1].first + pool.allocations[i-1].second;
            size_t gapEnd = pool.allocations[i].first;
            size_t gapSize = gapEnd - gapStart;
            
            if (gapSize >= size + alignment) {
                size_t alignedOffset = ((gapStart + alignment - 1) / alignment) * alignment;
                if (alignedOffset + size <= pool.size) {
                    result.memory = pool.memory;
                    result.offset = alignedOffset;
                    result.success = true;
                    
                    // Insert allocation
                    pool.allocations.insert(pool.allocations.begin() + i,
                                           {alignedOffset, size});
                    pool.used += size;
                    m_memoryMap[result.memory] = &pool - &pools[0];
                    
                    // Vendor-specific optimization
                    if (m_vendor == GPUVendor::NVIDIA) {
                        OptimizeAllocationNvidia(pool);
                    } else if (m_vendor == GPUVendor::AMD) {
                        OptimizeAllocationAmd(pool);
                    }
                    
                    return result;
                }
            }
        }
        
        // Try end of pool
        if (pool.size - pool.used >= size) {
            size_t alignedOffset = ((pool.used + alignment - 1) / alignment) * alignment;
            if (alignedOffset + size <= pool.size) {
                result.memory = pool.memory;
                result.offset = alignedOffset;
                result.success = true;
                
                pool.allocations.push_back({alignedOffset, size});
                pool.used += size;
                m_memoryMap[result.memory] = &pool - &pools[0];
                
                return result;
            }
        }
    }
    
    // Create new pool if needed
    if (pools.size() < m_config.maxPools) {
        PoolBlock newBlock;
        newBlock.size = m_config.poolSize;
        newBlock.used = size;
        newBlock.memory = VK_NULL_HANDLE;
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = newBlock.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        
        // Check for dedicated allocation (NVIDIA optimization)
        VkMemoryDedicatedAllocateInfo dedicatedInfo = {};
        if (m_config.preferDedicated && buffer != VK_NULL_HANDLE) {
            dedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
            dedicatedInfo.buffer = buffer;
            allocInfo.pNext = &dedicatedInfo;
        }
        
        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &newBlock.memory) == VK_SUCCESS) {
            newBlock.allocations.push_back({0, size});
            pools.push_back(newBlock);
            
            result.memory = newBlock.memory;
            result.offset = 0;
            result.success = true;
            m_memoryMap[result.memory] = pools.size() - 1;
            
            return result;
        }
    }
    
    return result;
}

void MemoryAllocator::Free(VkDeviceMemory memory, size_t offset, size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_memoryMap.find(memory);
    if (it == m_memoryMap.end()) return;
    
    size_t poolIndex = it->second;
    
    for (auto& typePair : m_pools) {
        if (poolIndex < typePair.second.size()) {
            auto& pool = typePair.second[poolIndex];
            for (auto allocIt = pool.allocations.begin(); 
                 allocIt != pool.allocations.end(); ++allocIt) {
                if (allocIt->first == offset && allocIt->second == size) {
                    pool.allocations.erase(allocIt);
                    pool.used -= size;
                    return;
                }
            }
        }
    }
}

void* MemoryAllocator::Map(VkDeviceMemory memory, size_t offset, size_t size) {
    void* data = nullptr;
    vkMapMemory(m_device, memory, offset, size, 0, &data);
    return data;
}

void MemoryAllocator::Unmap(VkDeviceMemory memory) {
    vkUnmapMemory(m_device, memory);
}

void MemoryAllocator::Defragment() {
    if (m_vendor == GPUVendor::NVIDIA) {
        DefragmentNvidia();
    } else if (m_vendor == GPUVendor::AMD) {
        DefragmentAmd();
    }
}

void MemoryAllocator::DefragmentNvidia() {
    // NVIDIA-optimized defragmentation
    // Consolidate small allocations and reduce fragmentation
    for (auto& poolPair : m_pools) {
        for (auto& pool : poolPair.second) {
            if (pool.allocations.empty()) continue;
            
            // Sort allocations by offset
            std::sort(pool.allocations.begin(), pool.allocations.end());
            
            // Compact allocations to the beginning
            size_t writeOffset = 0;
            for (auto& alloc : pool.allocations) {
                if (alloc.first != writeOffset) {
                    // Would need to move data here (application-specific)
                    // For now, just track the ideal layout
                }
                writeOffset += alloc.second;
            }
        }
    }
}

void MemoryAllocator::DefragmentAmd() {
    // AMD-optimized defragmentation
    // Focus on balancing across pools
    for (auto& poolPair : m_pools) {
        if (poolPair.second.size() <= 1) continue;
        
        // Calculate average utilization
        size_t totalUsed = 0;
        size_t totalSize = 0;
        for (const auto& pool : poolPair.second) {
            totalUsed += pool.used;
            totalSize += pool.size;
        }
        
        float avgUtilization = static_cast<float>(totalUsed) / totalSize;
        
        // Redistribute allocations if imbalance detected
        for (const auto& pool : poolPair.second) {
            float poolUtil = static_cast<float>(pool.used) / pool.size;
            if (std::abs(poolUtil - avgUtilization) > 0.3f) {
                // Mark for rebalancing (application would move data)
            }
        }
    }
}

void MemoryAllocator::OptimizeAllocationNvidia(PoolBlock& block) {
    // NVIDIA-specific: prefer larger contiguous blocks
    // Align to 256-byte boundaries for optimal access
    if (!block.allocations.empty()) {
        auto& last = block.allocations.back();
        size_t alignedEnd = ((last.first + last.second + 255) / 256) * 256;
        // Padding already accounted for in next allocation
    }
}

void MemoryAllocator::OptimizeAllocationAmd(PoolBlock& block) {
    // AMD-specific: balance between multiple smaller pools
    // No special alignment beyond standard 256 bytes
}

MemoryStats MemoryAllocator::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    MemoryStats stats = {};
    stats.totalAllocated = 0;
    stats.totalMapped = 0;
    stats.activeAllocations = 0;
    stats.poolCount = 0;
    
    for (const auto& poolPair : m_pools) {
        for (const auto& pool : poolPair.second) {
            stats.totalAllocated += pool.size;
            stats.totalMapped += pool.used;
            stats.activeAllocations += pool.allocations.size();
            stats.poolCount++;
        }
    }
    
    // Calculate fragmentation ratio
    if (stats.totalAllocated > 0) {
        stats.fragmentationRatio = 1.0f - (static_cast<float>(stats.totalMapped) / stats.totalAllocated);
    }
    
    return stats;
}

bool MemoryAllocator::BindBuffer(VkBuffer buffer, VkDeviceMemory memory, size_t offset) {
    return vkBindBufferMemory(m_device, buffer, memory, offset) == VK_SUCCESS;
}

bool MemoryAllocator::BindImage(VkImage image, VkDeviceMemory memory, size_t offset) {
    return vkBindImageMemory(m_device, image, memory, offset) == VK_SUCCESS;
}

} // namespace Vulkan
} // namespace GameTools
