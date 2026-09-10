#include "amd_optimizations.h"
#include <algorithm>
#include <cmath>

namespace GameTools {
namespace Vulkan {

// FSRConfig implementations
FSRConfig FSRConfig::GetDefault() {
    FSRConfig config;
    config.enabled = false;
    config.quality = Quality::Quality;
    config.sharpeningEnabled = true;
    config.sharpeningStrength = 0.5f;
    config.autoExposure = true;
    return config;
}

// WaveConfig implementations
WaveConfig WaveConfig::GetDefault() {
    WaveConfig config;
    config.waveSize = 64; // Default to wave64 for AMD
    config.useWaveOps = true;
    config.preferWave64 = true;
    config.maxWavesPerCU = 100;
    return config;
}

// AsyncComputeConfig implementations
AsyncComputeConfig AsyncComputeConfig::GetDefault() {
    AsyncComputeConfig config;
    config.enabled = true;
    config.maxConcurrentQueues = 2;
    config.useGfxQueue = true;
    config.useComputeQueue = true;
    config.workloadBalance = 0.3f; // 30% compute, 70% graphics
    return config;
}

// AmdOptimizations implementation
AmdOptimizations::AmdOptimizations(VkDevice device, VkPhysicalDevice physicalDevice)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_hasWaveOps(true)
    , m_hasAsyncCompute(true)
    , m_hasMeshShaders(false)
    , m_waveSize(64)
    , m_maxWavesPerCU(100)
    , m_computeUnits(0) {
    
    m_rdnaTuning.arch = RDNATuning::Architecture::UNKNOWN;
    m_rdnaTuning.computeUnits = 0;
    m_rdnaTuning.rayAccelerators = 0;
    m_rdnaTuning.supportsMeshShaders = false;
    m_rdnaTuning.supportsVariableRateShading = false;
    
    QueryDeviceProperties();
}

AmdOptimizations::~AmdOptimizations() {
}

void AmdOptimizations::QueryDeviceProperties() {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    
    m_vendorId = props.vendorID;
    m_deviceId = props.deviceID;
    
    if (m_vendorId == 0x1002) {
        // Detect RDNA architecture
        m_rdnaTuning.arch = DetectRDNAVersion(m_deviceId);
        
        // Set wave size based on architecture
        switch (m_rdnaTuning.arch) {
            case RDNATuning::Architecture::RDNA1:
                m_waveSize = 64;
                m_computeUnits = 40; // Max for RX 5700 XT
                break;
            case RDNATuning::Architecture::RDNA2:
                m_waveSize = 32; // RDNA2 supports both wave32 and wave64
                m_computeUnits = 80; // Max for RX 6900 XT
                m_rdnaTuning.supportsVariableRateShading = true;
                m_rdnaTuning.rayAccelerators = m_computeUnits;
                break;
            case RDNATuning::Architecture::RDNA3:
                m_waveSize = 64;
                m_computeUnits = 192; // Max for RX 7900 XTX (in WGP units)
                m_rdnaTuning.supportsVariableRateShading = true;
                m_rdnaTuning.supportsMeshShaders = true;
                m_rdnaTuning.rayAccelerators = m_computeUnits;
                m_hasMeshShaders = true;
                break;
            default:
                m_waveSize = 64;
                break;
        }
        
        m_rdnaTuning.computeUnits = m_computeUnits;
        m_rdnaTuning.supportsMeshShaders = m_hasMeshShaders;
    }
}

RDNATuning::Architecture AmdOptimizations::DetectRDNAVersion(uint32_t deviceId) const {
    // Detect RDNA generation based on device ID ranges
    // These are approximate ranges based on known AMD GPU IDs
    
    if (deviceId >= 0x7300 && deviceId <= 0x7400) {
        // Navi 21, 22, 23 - RDNA2
        return RDNATuning::Architecture::RDNA2;
    } else if (deviceId >= 0x7400 && deviceId <= 0x7500) {
        // Navi 31, 32, 33 - RDNA3
        return RDNATuning::Architecture::RDNA3;
    } else if (deviceId >= 0x6700 && deviceId <= 0x6800) {
        // Navi 10, 14 - RDNA1
        return RDNATuning::Architecture::RDNA1;
    }
    
    // Default to RDNA2 for unknown devices (most common)
    return RDNATuning::Architecture::RDNA2;
}

void AmdOptimizations::GetOptimalDispatch(uint32_t totalThreads,
                                         uint32_t& groupX, uint32_t& groupY, uint32_t& groupZ) {
    // AMD optimal dispatch: maximize wave occupancy
    // Wave size is typically 64 (wave64) or 32 (wave32) on newer architectures
    
    const uint32_t threadsPerBlock = 256; // Common workgroup size for AMD
    const uint32_t wavesPerBlock = (threadsPerBlock + m_waveSize - 1) / m_waveSize;
    
    // Calculate total blocks needed
    uint32_t totalBlocks = (totalThreads + threadsPerBlock - 1) / threadsPerBlock;
    
    // AMD prefers more balanced distributions across dimensions
    // This helps with async compute scheduling
    groupZ = std::min(totalBlocks, 32u);
    uint32_t remaining = (totalBlocks + groupZ - 1) / groupZ;
    
    groupY = static_cast<uint32_t>(std::sqrt(remaining));
    groupX = (remaining + groupY - 1) / groupY;
    
    // Ensure we don't exceed hardware limits
    groupX = std::min(groupX, 65535u);
    groupY = std::min(groupY, 65535u);
    groupZ = std::min(groupZ, 65535u);
}

ResourcePressure AmdOptimizations::AnalyzeResourcePressure(uint32_t threadsPerBlock,
                                                          uint32_t vgprsPerThread,
                                                          uint32_t sgprsPerThread,
                                                          uint32_t ldsPerBlock) {
    ResourcePressure pressure;
    
    // Calculate VGPR usage (AMD GPUs have limited VGPRs per CU)
    const uint32_t vgprsPerCU = 65536; // Typical for RDNA2/3
    pressure.vgprUsage = vgprsPerThread * threadsPerBlock;
    
    // Calculate SGPR usage
    const uint32_t sgprsPerCU = 8192;
    pressure.sgprUsage = sgprsPerThread * threadsPerBlock;
    
    // Calculate LDS usage
    const uint32_t ldsPerCU = 65536; // 64KB typical
    pressure.ldsUsage = ldsPerBlock;
    
    // Calculate occupancy based on limiting resource
    uint32_t maxBlocksByVGPR = vgprsPerCU / (vgprsPerThread * threadsPerBlock);
    uint32_t maxBlocksBySGPR = sgprsPerCU / (sgprsPerThread * threadsPerBlock);
    uint32_t maxBlocksByLDS = (ldsPerBlock > 0) ? (ldsPerCU / ldsPerBlock) : 16;
    uint32_t maxBlocksByThreads = 10; // Typical max workgroups per CU
    
    uint32_t maxActiveBlocks = std::min({maxBlocksByVGPR, maxBlocksBySGPR, maxBlocksByLDS, maxBlocksByThreads});
    
    // Calculate occupancy percentage
    pressure.occupancy = (maxActiveBlocks * 100) / 10;
    
    // Check for register spill (when VGPR usage is high)
    pressure.hasRegisterSpill = (pressure.vgprUsage > vgprsPerCU * 0.8f);
    
    // Calculate efficiency score
    pressure.efficiencyScore = static_cast<float>(pressure.occupancy) / 100.0f;
    if (pressure.hasRegisterSpill) {
        pressure.efficiencyScore *= 0.7f; // Penalty for register spill
    }
    if (pressure.ldsUsage > ldsPerCU * 0.8f) {
        pressure.efficiencyScore *= 0.85f; // Penalty for high LDS usage
    }
    
    return pressure;
}

std::vector<ComputeRecommendation> AmdOptimizations::GetComputeRecommendations(
    const ResourcePressure& pressure) {
    
    std::vector<ComputeRecommendation> recommendations;
    
    if (pressure.occupancy < 50) {
        ComputeRecommendation rec;
        rec.recommendation = "Low occupancy detected. Consider reducing VGPR usage or adjusting workgroup size.";
        rec.expectedSpeedup = 1.6f;
        rec.priority = 1;
        recommendations.push_back(rec);
    }
    
    if (pressure.hasRegisterSpill) {
        ComputeRecommendation rec;
        rec.recommendation = "VGPR pressure causing spills. Reduce local variables or split kernel.";
        rec.expectedSpeedup = 1.8f;
        rec.priority = 1;
        recommendations.push_back(rec);
    }
    
    if (pressure.ldsUsage > 49152) { // > 75% of 64KB
        ComputeRecommendation rec;
        rec.recommendation = "High LDS usage. Consider tiling strategy or reducing shared data.";
        rec.expectedSpeedup = 1.3f;
        rec.priority = 2;
        recommendations.push_back(rec);
    }
    
    if (pressure.sgprUsage > 6144) { // High SGPR usage
        ComputeRecommendation rec;
        rec.recommendation = "High SGPR pressure. Consider using uniform values or reducing branch complexity.";
        rec.expectedSpeedup = 1.2f;
        rec.priority = 3;
        recommendations.push_back(rec);
    }
    
    // Sort by priority
    std::sort(recommendations.begin(), recommendations.end(),
              [](const ComputeRecommendation& a, const ComputeRecommendation& b) {
                  return a.priority < b.priority;
              });
    
    return recommendations;
}

FSRConfig AmdOptimizations::ConfigureFSR(FSRConfig::Quality quality,
                                        uint32_t renderWidth, uint32_t renderHeight,
                                        uint32_t displayWidth, uint32_t displayHeight,
                                        float sharpeningStrength) {
    FSRConfig config = FSRConfig::GetDefault();
    config.enabled = true;
    config.quality = quality;
    config.sharpeningEnabled = (sharpeningStrength > 0.0f);
    config.sharpeningStrength = sharpeningStrength;
    config.autoExposure = true;
    
    return config;
}

WaveConfig AmdOptimizations::ConfigureWaveOps(bool preferLargeWaves) {
    WaveConfig config = WaveConfig::GetDefault();
    
    if (preferLargeWaves) {
        config.waveSize = 64;
        config.preferWave64 = true;
    } else {
        config.waveSize = 32;
        config.preferWave64 = false;
    }
    
    config.useWaveOps = true;
    
    return config;
}

AsyncComputeConfig AmdOptimizations::ConfigureAsyncCompute(float graphicsToComputeRatio) {
    AsyncComputeConfig config = AsyncComputeConfig::GetDefault();
    config.enabled = m_hasAsyncCompute;
    config.workloadBalance = 1.0f - graphicsToComputeRatio;
    
    // Adjust queue count based on architecture
    if (m_rdnaTuning.arch == RDNATuning::Architecture::RDNA3) {
        config.maxConcurrentQueues = 4; // RDNA3 has improved async compute
    } else if (m_rdnaTuning.arch == RDNATuning::Architecture::RDNA2) {
        config.maxConcurrentQueues = 2;
    } else {
        config.maxConcurrentQueues = 1;
    }
    
    return config;
}

void AmdOptimizations::OptimizeBarriers(std::vector<VkBufferMemoryBarrier>& barriers) {
    // AMD optimization: use inline barriers where possible
    // AMD performs better with more granular, inline barriers vs aggregated ones
    
    // For AMD, we keep barriers separate but ensure proper ordering
    // No merging like NVIDIA - AMD's driver handles this differently
    
    // Sort barriers by stage to improve scheduling
    std::sort(barriers.begin(), barriers.end(),
              [](const VkBufferMemoryBarrier& a, const VkBufferMemoryBarrier& b) {
                  // Sort by source stage first, then destination
                  if (a.srcStageMask != b.srcStageMask) {
                      return a.srcStageMask < b.srcStageMask;
                  }
                  return a.dstStageMask < b.dstStageMask;
              });
}

float AmdOptimizations::EstimateAsyncComputeSpeedup(uint32_t computeWorkloads,
                                                   uint32_t graphicsWorkloads) const {
    if (!m_hasAsyncCompute || computeWorkloads == 0) {
        return 1.0f; // No speedup without async compute
    }
    
    // Estimate speedup based on workload balance and architecture
    float baseSpeedup = 1.0f;
    
    switch (m_rdnaTuning.arch) {
        case RDNATuning::Architecture::RDNA3:
            baseSpeedup = 1.25f; // Up to 25% improvement on RDNA3
            break;
        case RDNATuning::Architecture::RDNA2:
            baseSpeedup = 1.15f; // Up to 15% improvement on RDNA2
            break;
        case RDNATuning::Architecture::RDNA1:
            baseSpeedup = 1.08f; // Modest improvement on RDNA1
            break;
        default:
            baseSpeedup = 1.05f;
            break;
    }
    
    // Adjust based on workload ratio
    float workloadRatio = static_cast<float>(computeWorkloads) / 
                         (computeWorkloads + graphicsWorkloads);
    
    // Optimal balance is around 20-30% compute workloads
    float optimalRatio = 0.25f;
    float ratioFactor = 1.0f - std::abs(workloadRatio - optimalRatio) * 2.0f;
    ratioFactor = std::max(0.5f, ratioFactor);
    
    return baseSpeedup * ratioFactor;
}

} // namespace Vulkan
} // namespace GameTools
