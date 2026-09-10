#ifndef VULKAN_OPTIMIZATION_MANAGER_H
#define VULKAN_OPTIMIZATION_MANAGER_H

#include "vulkan_types.h"
#include "memory_allocator.h"
#include "command_buffer_manager.h"
#include "render_batcher.h"
#include "tiled_renderer.h"
#include "nvidia_optimizations.h"
#include "amd_optimizations.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <string>

namespace GameTools {
namespace Vulkan {

// Overall optimization settings
struct OptimizationSettings {
    bool enableBatching;
    bool enableTiling;
    bool enableAsyncCompute;
    bool enableUpscaling;
    bool enableDefragmentation;
    uint32_t maxFramesInFlight;
    
    static OptimizationSettings GetDefault();
    static OptimizationSettings GetPerformance();
    static OptimizationSettings GetQuality();
};

// Upscaling configuration (DLSS/FSR)
struct UpscaleConfig {
    enum class Type {
        NONE,
        DLSS,      // NVIDIA
        FSR,       // AMD
        FSR2,      // AMD FSR 2.x
        XeSS       // Intel (future)
    };
    Type type;
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    float quality;
};

// Performance metrics
struct PerformanceMetrics {
    float fps;
    float frameTimeMs;
    float gpuUtilization;
    float memoryUsageMB;
    uint32_t drawCalls;
    uint32_t batches;
    float batchingEfficiency;
    float overdrawReduction;
};

// Main optimization manager - cross-vendor unified API
class VulkanOptimizationManager {
public:
    VulkanOptimizationManager(VkInstance instance, VkDevice device,
                             VkPhysicalDevice physicalDevice);
    ~VulkanOptimizationManager();
    
    // Initialize the optimization manager
    void Initialize(const OptimizationSettings& settings);
    
    // Detect GPU vendor and get info
    GPUVendor DetectGPU() const { return m_vendor; }
    std::string GetGPUName() const { return m_gpuName; }
    uint32_t GetVendorId() const { return m_vendorId; }
    
    // Get vendor-specific optimizations
    NvidiaOptimizations* GetNvidiaOptimizations() { return m_nvidiaOpt.get(); }
    AmdOptimizations* GetAmdOptimizations() { return m_amdOpt.get(); }
    
    // Memory management
    MemoryAllocator* GetMemoryAllocator() { return m_memoryAlloc.get(); }
    
    // Command buffer management
    CommandBufferManager* GetCommandBufferManager() { return m_cmdManager.get(); }
    
    // Render batching
    RenderBatcher* GetRenderBatcher() { return m_renderBatcher.get(); }
    
    // Tiled rendering
    TiledRenderer* GetTiledRenderer() { return m_tiledRenderer.get(); }
    
    // Configure upscaling (DLSS for NVIDIA, FSR for AMD)
    UpscaleConfig ConfigureUpscaling(UpscaleConfig::Type type,
                                    uint32_t renderWidth, uint32_t renderHeight,
                                    uint32_t displayWidth, uint32_t displayHeight);
    
    // Optimize the entire render pipeline
    void OptimizeRenderPipeline();
    
    // Get optimal settings for current hardware
    OptimizationSettings GetOptimalSettings() const;
    
    // Apply recommended optimizations
    void ApplyRecommendations();
    
    // Get performance metrics
    PerformanceMetrics GetMetrics() const;
    
    // Print optimization summary
    void PrintOptimizationSummary() const;
    
    // Get recommended compute dispatch dimensions
    void GetOptimalDispatch(uint32_t totalThreads,
                           uint32_t& groupX, uint32_t& groupY, uint32_t& groupZ);

private:
    VkInstance m_instance;
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    
    GPUVendor m_vendor;
    std::string m_gpuName;
    uint32_t m_vendorId;
    uint32_t m_deviceId;
    
    OptimizationSettings m_settings;
    
    std::unique_ptr<NvidiaOptimizations> m_nvidiaOpt;
    std::unique_ptr<AmdOptimizations> m_amdOpt;
    std::unique_ptr<MemoryAllocator> m_memoryAlloc;
    std::unique_ptr<CommandBufferManager> m_cmdManager;
    std::unique_ptr<RenderBatcher> m_renderBatcher;
    std::unique_ptr<TiledRenderer> m_tiledRenderer;
    
    void DetectHardware();
    void CreateVendorOptimizations();
    void CreateMemoryAllocator();
    void CreateCommandBufferManager();
    void CreateRenderBatcher();
    void CreateTiledRenderer();
};

} // namespace Vulkan
} // namespace GameTools

#endif // VULKAN_OPTIMIZATION_MANAGER_H
