#ifndef AMD_OPTIMIZATIONS_H
#define AMD_OPTIMIZATIONS_H

#include "vulkan_types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace GameTools {
namespace Vulkan {

// FSR (FidelityFX Super Resolution) configuration
struct FSRConfig {
    bool enabled;
    enum class Quality {
        UltraPerformance,
        Performance,
        Balanced,
        Quality,
        UltraQuality
    };
    Quality quality;
    bool sharpeningEnabled;
    float sharpeningStrength; // 0.0 to 1.0
    bool autoExposure;
    
    static FSRConfig GetDefault();
};

// Wave operation configuration
struct WaveConfig {
    uint32_t waveSize; // AMD: 64 (wave64) or 32 (wave32)
    bool useWaveOps;
    bool preferWave64;
    uint32_t maxWavesPerCU;
    
    static WaveConfig GetDefault();
};

// Async compute configuration
struct AsyncComputeConfig {
    bool enabled;
    uint32_t maxConcurrentQueues;
    bool useGfxQueue;
    bool useComputeQueue;
    float workloadBalance; // 0.0-1.0, ratio of compute vs graphics
    
    static AsyncComputeConfig GetDefault();
};

// Resource pressure analysis
struct ResourcePressure {
    uint32_t vgprUsage;      // Vector General Purpose Registers
    uint32_t sgprUsage;      // Scalar General Purpose Registers
    uint32_t ldsUsage;       // Local Data Share
    uint32_t occupancy;      // Theoretical occupancy percentage
    bool hasRegisterSpill;
    float efficiencyScore;
};

// RDNA architecture tuning
struct RDNATuning {
    enum class Architecture {
        RDNA1,
        RDNA2,
        RDNA3,
        UNKNOWN
    };
    Architecture arch;
    uint32_t computeUnits;
    uint32_t rayAccelerators;
    bool supportsMeshShaders;
    bool supportsVariableRateShading;
};

// Compute shader optimization recommendations
struct ComputeRecommendation {
    std::string recommendation;
    float expectedSpeedup;
    int priority; // 1 = highest
};

// AMD-specific optimizations
class AmdOptimizations {
public:
    AmdOptimizations(VkDevice device, VkPhysicalDevice physicalDevice);
    ~AmdOptimizations();
    
    // Check for AMD-specific extensions
    bool HasWaveOps() const { return m_hasWaveOps; }
    bool HasAsyncCompute() const { return m_hasAsyncCompute; }
    bool SupportsFSR() const { return true; } // FSR is software-based
    bool HasMeshShaders() const { return m_hasMeshShaders; }
    
    // Get optimal compute dispatch dimensions
    void GetOptimalDispatch(uint32_t totalThreads,
                           uint32_t& groupX, uint32_t& groupY, uint32_t& groupZ);
    
    // Analyze resource pressure for a compute shader
    ResourcePressure AnalyzeResourcePressure(uint32_t threadsPerBlock,
                                            uint32_t vgprsPerThread,
                                            uint32_t sgprsPerThread,
                                            uint32_t ldsPerBlock);
    
    // Get compute shader optimization recommendations
    std::vector<ComputeRecommendation> GetComputeRecommendations(
        const ResourcePressure& pressure);
    
    // Configure FSR
    FSRConfig ConfigureFSR(FSRConfig::Quality quality,
                          uint32_t renderWidth, uint32_t renderHeight,
                          uint32_t displayWidth, uint32_t displayHeight,
                          float sharpeningStrength = 0.5f);
    
    // Configure wave operations
    WaveConfig ConfigureWaveOps(bool preferLargeWaves);
    
    // Configure async compute
    AsyncComputeConfig ConfigureAsyncCompute(float graphicsToComputeRatio);
    
    // Optimize barrier placement for AMD GPUs
    void OptimizeBarriers(std::vector<VkBufferMemoryBarrier>& barriers);
    
    // Get recommended pool size for memory allocation
    size_t GetRecommendedPoolSize() const { return 128 * 1024 * 1024; } // 128MB
    
    // Get optimal batch size for command buffers
    uint32_t GetOptimalBatchSize() const { return 256; }
    
    // Get wave size
    uint32_t GetWaveSize() const { return m_waveSize; }
    
    // Get RDNA architecture info
    RDNATuning GetRDNATuning() const { return m_rdnaTuning; }
    
    // Estimate async compute speedup
    float EstimateAsyncComputeSpeedup(uint32_t computeWorkloads,
                                     uint32_t graphicsWorkloads) const;

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    
    bool m_hasWaveOps;
    bool m_hasAsyncCompute;
    bool m_hasMeshShaders;
    
    uint32_t m_waveSize;
    uint32_t m_maxWavesPerCU;
    uint32_t m_computeUnits;
    
    RDNATuning m_rdnaTuning;
    
    void QueryDeviceProperties();
    RDNATuning::Architecture DetectRDNAVersion(uint32_t deviceId) const;
};

} // namespace Vulkan
} // namespace GameTools

#endif // AMD_OPTIMIZATIONS_H
