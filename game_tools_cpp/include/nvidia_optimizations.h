#ifndef NVIDIA_OPTIMIZATIONS_H
#define NVIDIA_OPTIMIZATIONS_H

#include "vulkan_types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

// Forward declare NV extensions if available
#ifdef VK_NV_mesh_shader
#include <vulkan/vulkan_beta.h>
#endif

namespace GameTools {
namespace Vulkan {

// DLSS configuration
struct DLSSConfig {
    bool enabled;
    enum class Quality {
        Performance,
        Balanced,
        Quality,
        UltraQuality
    };
    Quality quality;
    bool autoExposure;
    bool motionVectors;
    
    static DLSSConfig GetDefault();
};

// Mesh shader configuration (NVIDIA RTX+)
struct MeshShaderConfig {
    bool enabled;
    uint32_t maxPrimitivesPerMesh;
    uint32_t maxVerticesPerMesh;
    bool useAmplificationShaders;
    
    static MeshShaderConfig GetDefault();
};

// Shader Execution Reordering (SER) configuration
struct SERConfig {
    bool enabled;
    uint32_t reorderAlgorithm;
    float complexityThreshold;
    
    static SERConfig GetDefault();
};

// Warp-level optimization analysis
struct WarpAnalysis {
    uint32_t warpSize; // NVIDIA: 32
    float occupancy;   // Active warps / Max warps
    uint32_t registerPressure;
    uint32_t sharedMemoryUsage;
    bool hasDivergence;
    float efficiencyScore;
};

// Compute shader optimization recommendations
struct ComputeRecommendation {
    std::string recommendation;
    float expectedSpeedup;
    int priority; // 1 = highest
};

// NVIDIA-specific optimizations
class NvidiaOptimizations {
public:
    NvidiaOptimizations(VkDevice device, VkPhysicalDevice physicalDevice);
    ~NvidiaOptimizations();
    
    // Check for NVIDIA-specific extensions
    bool HasMeshShaders() const { return m_hasMeshShaders; }
    bool HasSER() const { return m_hasSER; }
    bool HasTensorCores() const { return m_hasTensorCores; }
    bool SupportsDLSS() const { return m_supportsDLSS; }
    
    // Get optimal compute dispatch dimensions
    void GetOptimalDispatch(uint32_t totalThreads, 
                           uint32_t& groupX, uint32_t& groupY, uint32_t& groupZ);
    
    // Analyze warp occupancy for a compute shader
    WarpAnalysis AnalyzeWarpOccupancy(uint32_t threadsPerBlock,
                                     uint32_t registersPerThread,
                                     uint32_t sharedMemoryPerBlock);
    
    // Get compute shader optimization recommendations
    std::vector<ComputeRecommendation> GetComputeRecommendations(
        const WarpAnalysis& analysis);
    
    // Configure DLSS
    DLSSConfig ConfigureDLSS(DLSSConfig::Quality quality,
                            uint32_t renderWidth, uint32_t renderHeight,
                            uint32_t displayWidth, uint32_t displayHeight);
    
    // Configure mesh shaders
    MeshShaderConfig ConfigureMeshShaders(uint32_t targetPrimitives,
                                         uint32_t targetVertices);
    
    // Enable Shader Execution Reordering
    SERConfig ConfigureSER(bool enableRayTracing);
    
    // Optimize barrier placement for NVIDIA GPUs
    void OptimizeBarriers(std::vector<VkBufferMemoryBarrier>& barriers);
    
    // Get recommended pool size for memory allocation
    size_t GetRecommendedPoolSize() const { return 256 * 1024 * 1024; } // 256MB
    
    // Get optimal batch size for command buffers
    uint32_t GetOptimalBatchSize() const { return 1024; }
    
    // Get warp size
    uint32_t GetWarpSize() const { return 32; }

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    
    bool m_hasMeshShaders;
    bool m_hasSER;
    bool m_hasTensorCores;
    bool m_supportsDLSS;
    
    uint32_t m_smVersion; // SM version (e.g., 86 for Ampere)
    uint32_t m_warpSize;
    uint32_t m_maxThreadsPerBlock;
    uint32_t m_maxSharedMemory;
    uint32_t m_registersPerBlock;
    
    void QueryDeviceProperties();
};

} // namespace Vulkan
} // namespace GameTools

#endif // NVIDIA_OPTIMIZATIONS_H
