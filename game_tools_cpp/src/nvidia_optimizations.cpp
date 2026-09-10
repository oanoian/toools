#include "nvidia_optimizations.h"
#include <algorithm>
#include <cmath>

namespace GameTools {
namespace Vulkan {

// DLSSConfig implementations
DLSSConfig DLSSConfig::GetDefault() {
    DLSSConfig config;
    config.enabled = false;
    config.quality = Quality::Quality;
    config.autoExposure = true;
    config.motionVectors = true;
    return config;
}

// MeshShaderConfig implementations
MeshShaderConfig MeshShaderConfig::GetDefault() {
    MeshShaderConfig config;
    config.enabled = false;
    config.maxPrimitivesPerMesh = 512;
    config.maxVerticesPerMesh = 256;
    config.useAmplificationShaders = true;
    return config;
}

// SERConfig implementations
SERConfig SERConfig::GetDefault() {
    SERConfig config;
    config.enabled = false;
    config.reorderAlgorithm = 0;
    config.complexityThreshold = 0.5f;
    return config;
}

// NvidiaOptimizations implementation
NvidiaOptimizations::NvidiaOptimizations(VkDevice device, VkPhysicalDevice physicalDevice)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_hasMeshShaders(false)
    , m_hasSER(false)
    , m_hasTensorCores(false)
    , m_supportsDLSS(false)
    , m_smVersion(0)
    , m_warpSize(32)
    , m_maxThreadsPerBlock(1024)
    , m_maxSharedMemory(49152)
    , m_registersPerBlock(65536) {
    
    QueryDeviceProperties();
}

NvidiaOptimizations::~NvidiaOptimizations() {
}

void NvidiaOptimizations::QueryDeviceProperties() {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    
    m_vendorId = props.vendorID;
    m_deviceId = props.deviceID;
    
    // Detect NVIDIA features based on device properties
    if (m_vendorId == 0x10DE) {
        // Check for Tensor Cores (RTX series)
        // RTX 20xx and newer have tensor cores
        if (m_deviceId >= 0x1E00 && m_deviceId <= 0x2200) {
            // Turing (RTX 20xx)
            m_smVersion = 75;
            m_hasTensorCores = true;
            m_supportsDLSS = true;
        } else if (m_deviceId >= 0x2200 && m_deviceId <= 0x2600) {
            // Ampere (RTX 30xx)
            m_smVersion = 86;
            m_hasTensorCores = true;
            m_supportsDLSS = true;
            m_hasSER = true; // SER available on Ada Lovelace and newer
        } else if (m_deviceId >= 0x2600) {
            // Ada Lovelace (RTX 40xx) and newer
            m_smVersion = 89;
            m_hasTensorCores = true;
            m_supportsDLSS = true;
            m_hasSER = true;
            m_hasMeshShaders = true;
        } else if (m_deviceId >= 0x1B00 && m_deviceId < 0x1E00) {
            // Pascal (GTX 10xx) - no tensor cores
            m_smVersion = 61;
            m_hasTensorCores = false;
            m_supportsDLSS = false;
        }
        
        // Check extension support
        // In real implementation, would enumerate device extensions
    }
}

void NvidiaOptimizations::GetOptimalDispatch(uint32_t totalThreads,
                                            uint32_t& groupX, uint32_t& groupY, uint32_t& groupZ) {
    // NVIDIA optimal dispatch: maximize warp occupancy
    // Warp size is 32, so align to multiples of 32
    
    const uint32_t threadsPerBlock = m_maxThreadsPerBlock; // 1024 for most NVIDIA GPUs
    const uint32_t warpsPerBlock = threadsPerBlock / m_warpSize; // 32 warps
    
    // Calculate total blocks needed
    uint32_t totalBlocks = (totalThreads + threadsPerBlock - 1) / threadsPerBlock;
    
    // Optimal 2D/3D distribution for better occupancy
    // Prefer square-ish distributions for 2D, cubic for 3D
    groupZ = std::min(totalBlocks, 64u);
    uint32_t remaining = (totalBlocks + groupZ - 1) / groupZ;
    
    groupY = static_cast<uint32_t>(std::sqrt(remaining));
    groupX = (remaining + groupY - 1) / groupY;
    
    // Ensure we don't exceed hardware limits
    groupX = std::min(groupX, 65535u);
    groupY = std::min(groupY, 65535u);
    groupZ = std::min(groupZ, 65535u);
}

WarpAnalysis NvidiaOptimizations::AnalyzeWarpOccupancy(uint32_t threadsPerBlock,
                                                      uint32_t registersPerThread,
                                                      uint32_t sharedMemoryPerBlock) {
    WarpAnalysis analysis;
    analysis.warpSize = m_warpSize;
    
    // Calculate warps per block
    uint32_t warpsPerBlock = (threadsPerBlock + m_warpSize - 1) / m_warpSize;
    
    // Calculate register usage per block
    uint32_t registersPerBlock = registersPerThread * threadsPerBlock;
    analysis.registerPressure = registersPerBlock;
    
    // Calculate shared memory usage
    analysis.sharedMemoryUsage = sharedMemoryPerBlock;
    
    // Determine limiting factors
    uint32_t maxWarpsByRegisters = m_registersPerBlock / registersPerBlock;
    uint32_t maxWarpsBySharedMem = (sharedMemoryPerBlock > 0) ? 
                                   (m_maxSharedMemory / sharedMemoryPerBlock) : 64;
    uint32_t maxWarpsByThreads = m_maxThreadsPerBlock / threadsPerBlock;
    
    uint32_t maxActiveWarps = std::min({maxWarpsByRegisters, maxWarpsBySharedMem, maxWarpsByThreads});
    
    // Calculate occupancy
    const uint32_t maxWarpsPerSM = 64; // Typical for modern NVIDIA GPUs
    analysis.occupancy = static_cast<float>(maxActiveWarps) / maxWarpsPerSM;
    
    // Check for register spill
    analysis.hasDivergence = (registersPerThread > 32); // High register pressure
    
    // Calculate efficiency score (0-1)
    analysis.efficiencyScore = analysis.occupancy;
    if (analysis.hasDivergence) {
        analysis.efficiencyScore *= 0.8f; // Penalty for divergence
    }
    if (analysis.sharedMemoryUsage > m_maxSharedMemory * 0.8f) {
        analysis.efficiencyScore *= 0.9f; // Penalty for high shared memory usage
    }
    
    return analysis;
}

std::vector<ComputeRecommendation> NvidiaOptimizations::GetComputeRecommendations(
    const WarpAnalysis& analysis) {
    
    std::vector<ComputeRecommendation> recommendations;
    
    if (analysis.occupancy < 0.5f) {
        ComputeRecommendation rec;
        rec.recommendation = "Low occupancy detected. Consider reducing registers per thread or increasing threads per block.";
        rec.expectedSpeedup = 1.5f;
        rec.priority = 1;
        recommendations.push_back(rec);
    }
    
    if (analysis.registerPressure > m_registersPerBlock * 0.8f) {
        ComputeRecommendation rec;
        rec.recommendation = "High register pressure. Consider using fewer local variables or splitting kernels.";
        rec.expectedSpeedup = 1.3f;
        rec.priority = 2;
        recommendations.push_back(rec);
    }
    
    if (analysis.sharedMemoryUsage > m_maxSharedMemory * 0.7f) {
        ComputeRecommendation rec;
        rec.recommendation = "High shared memory usage. Consider tiling or reducing shared memory footprint.";
        rec.expectedSpeedup = 1.2f;
        rec.priority = 3;
        recommendations.push_back(rec);
    }
    
    if (analysis.hasDivergence) {
        ComputeRecommendation rec;
        rec.recommendation = "Potential warp divergence. Consider restructuring conditionals for coalesced execution.";
        rec.expectedSpeedup = 1.4f;
        rec.priority = 2;
        recommendations.push_back(rec);
    }
    
    // Sort by priority
    std::sort(recommendations.begin(), recommendations.end(),
              [](const ComputeRecommendation& a, const ComputeRecommendation& b) {
                  return a.priority < b.priority;
              });
    
    return recommendations;
}

DLSSConfig NvidiaOptimizations::ConfigureDLSS(DLSSConfig::Quality quality,
                                             uint32_t renderWidth, uint32_t renderHeight,
                                             uint32_t displayWidth, uint32_t displayHeight) {
    DLSSConfig config = DLSSConfig::GetDefault();
    config.enabled = m_supportsDLSS;
    config.quality = quality;
    
    // Adjust render resolution based on quality setting
    float scale = 1.0f;
    switch (quality) {
        case DLSSConfig::Quality::Performance:
            scale = 0.5f;
            break;
        case DLSSConfig::Quality::Balanced:
            scale = 0.67f;
            break;
        case DLSSConfig::Quality::Quality:
            scale = 0.75f;
            break;
        case DLSSConfig::Quality::UltraQuality:
            scale = 0.85f;
            break;
    }
    
    config.autoExposure = true;
    config.motionVectors = true;
    
    return config;
}

MeshShaderConfig NvidiaOptimizations::ConfigureMeshShaders(uint32_t targetPrimitives,
                                                          uint32_t targetVertices) {
    MeshShaderConfig config = MeshShaderConfig::GetDefault();
    config.enabled = m_hasMeshShaders;
    
    // Configure based on target geometry
    if (targetPrimitives > 256) {
        config.maxPrimitivesPerMesh = 512;
    } else {
        config.maxPrimitivesPerMesh = 256;
    }
    
    if (targetVertices > 128) {
        config.maxVerticesPerMesh = 256;
    } else {
        config.maxVerticesPerMesh = 128;
    }
    
    config.useAmplificationShaders = true;
    
    return config;
}

SERConfig NvidiaOptimizations::ConfigureSER(bool enableRayTracing) {
    SERConfig config = SERConfig::GetDefault();
    config.enabled = m_hasSER && enableRayTracing;
    config.reorderAlgorithm = enableRayTracing ? 1 : 0; // 1 = ray tracing optimized
    config.complexityThreshold = 0.5f;
    
    return config;
}

void NvidiaOptimizations::OptimizeBarriers(std::vector<VkBufferMemoryBarrier>& barriers) {
    // NVIDIA optimization: aggregate barriers where possible
    // This reduces driver overhead and improves performance
    
    if (barriers.size() <= 1) return;
    
    // Sort barriers by buffer handle to group same-buffer operations
    std::sort(barriers.begin(), barriers.end(),
              [](const VkBufferMemoryBarrier& a, const VkBufferMemoryBarrier& b) {
                  return a.buffer < b.buffer;
              });
    
    // Merge adjacent barriers with compatible stages/accesses
    std::vector<VkBufferMemoryBarrier> merged;
    merged.reserve(barriers.size());
    
    for (size_t i = 0; i < barriers.size(); ) {
        VkBufferMemoryBarrier current = barriers[i];
        size_t j = i + 1;
        
        // Try to merge with subsequent barriers for the same buffer
        while (j < barriers.size() && barriers[j].buffer == current.buffer) {
            current.dstStageMask |= barriers[j].srcStageMask;
            current.dstAccessMask |= barriers[j].srcAccessMask;
            j++;
        }
        
        merged.push_back(current);
        i = j;
    }
    
    barriers = std::move(merged);
}

} // namespace Vulkan
} // namespace GameTools
