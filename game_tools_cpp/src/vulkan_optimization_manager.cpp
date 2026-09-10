#include "vulkan_optimization_manager.h"
#include <iostream>
#include <algorithm>

namespace GameTools {
namespace Vulkan {

// OptimizationSettings implementations
OptimizationSettings OptimizationSettings::GetDefault() {
    OptimizationSettings settings;
    settings.enableBatching = true;
    settings.enableTiling = true;
    settings.enableAsyncCompute = true;
    settings.enableUpscaling = false;
    settings.enableDefragmentation = true;
    settings.maxFramesInFlight = 3;
    return settings;
}

OptimizationSettings OptimizationSettings::GetPerformance() {
    OptimizationSettings settings = GetDefault();
    settings.enableBatching = true;
    settings.enableTiling = false; // Disable for lower latency
    settings.enableAsyncCompute = true;
    settings.enableUpscaling = true;
    settings.enableDefragmentation = false;
    settings.maxFramesInFlight = 2;
    return settings;
}

OptimizationSettings OptimizationSettings::GetQuality() {
    OptimizationSettings settings = GetDefault();
    settings.enableBatching = true;
    settings.enableTiling = true;
    settings.enableAsyncCompute = true;
    settings.enableUpscaling = true;
    settings.enableDefragmentation = true;
    settings.maxFramesInFlight = 3;
    return settings;
}

// VulkanOptimizationManager implementation
VulkanOptimizationManager::VulkanOptimizationManager(VkInstance instance, VkDevice device,
                                                     VkPhysicalDevice physicalDevice)
    : m_instance(instance)
    , m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_vendor(GPUVendor::UNKNOWN)
    , m_vendorId(0)
    , m_deviceId(0) {
    
    DetectHardware();
}

VulkanOptimizationManager::~VulkanOptimizationManager() {
    // Smart pointers will clean up automatically
}

void VulkanOptimizationManager::DetectHardware() {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    
    m_gpuName = props.deviceName;
    m_vendorId = props.vendorID;
    m_deviceId = props.deviceID;
    m_vendor = DetectVendor(m_vendorId);
    
    std::cout << "Detected GPU: " << m_gpuName << std::endl;
    std::cout << "Vendor: " << VendorToString(m_vendor) << std::endl;
}

void VulkanOptimizationManager::CreateVendorOptimizations() {
    if (m_vendor == GPUVendor::NVIDIA) {
        m_nvidiaOpt = std::make_unique<NvidiaOptimizations>(m_device, m_physicalDevice);
        std::cout << "Initialized NVIDIA optimizations" << std::endl;
    } else if (m_vendor == GPUVendor::AMD) {
        m_amdOpt = std::make_unique<AmdOptimizations>(m_device, m_physicalDevice);
        std::cout << "Initialized AMD optimizations" << std::endl;
    }
}

void VulkanOptimizationManager::CreateMemoryAllocator() {
    MemoryPoolConfig config;
    
    switch (m_vendor) {
        case GPUVendor::NVIDIA:
            config = MemoryPoolConfig::GetNvidiaConfig();
            break;
        case GPUVendor::AMD:
            config = MemoryPoolConfig::GetAmdConfig();
            break;
        case GPUVendor::INTEL:
            config = MemoryPoolConfig::GetIntelConfig();
            break;
        default:
            config = MemoryPoolConfig::GetGenericConfig();
            break;
    }
    
    m_memoryAlloc = std::make_unique<MemoryAllocator>(m_device, m_physicalDevice, 
                                                      m_vendor, config);
}

void VulkanOptimizationManager::CreateCommandBufferManager() {
    // Get graphics queue (simplified - in real code would query queue family)
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device, 0, 0, &graphicsQueue);
    
    BatchConfig config;
    
    switch (m_vendor) {
        case GPUVendor::NVIDIA:
            config = BatchConfig::GetNvidiaConfig();
            break;
        case GPUVendor::AMD:
            config = BatchConfig::GetAmdConfig();
            break;
        default:
            config = BatchConfig::GetGenericConfig();
            break;
    }
    
    m_cmdManager = std::make_unique<CommandBufferManager>(m_device, graphicsQueue,
                                                          m_vendor, config);
}

void VulkanOptimizationManager::CreateRenderBatcher() {
    m_renderBatcher = std::make_unique<RenderBatcher>(m_device, m_vendor);
}

void VulkanOptimizationManager::CreateTiledRenderer() {
    m_tiledRenderer = std::make_unique<TiledRenderer>(m_device, m_vendor);
}

void VulkanOptimizationManager::Initialize(const OptimizationSettings& settings) {
    m_settings = settings;
    
    CreateVendorOptimizations();
    CreateMemoryAllocator();
    CreateCommandBufferManager();
    CreateRenderBatcher();
    CreateTiledRenderer();
    
    std::cout << "Vulkan Optimization Manager initialized successfully" << std::endl;
}

UpscaleConfig VulkanOptimizationManager::ConfigureUpscaling(UpscaleConfig::Type type,
                                                           uint32_t renderWidth, uint32_t renderHeight,
                                                           uint32_t displayWidth, uint32_t displayHeight) {
    UpscaleConfig config;
    config.type = type;
    config.renderWidth = renderWidth;
    config.renderHeight = renderHeight;
    config.displayWidth = displayWidth;
    config.displayHeight = displayHeight;
    config.quality = 1.0f;
    
    // Auto-detect best upscaling method based on vendor
    if (type == UpscaleConfig::Type::NONE) {
        if (m_vendor == GPUVendor::NVIDIA && m_nvidiaOpt && m_nvidiaOpt->SupportsDLSS()) {
            config.type = UpscaleConfig::Type::DLSS;
        } else if (m_vendor == GPUVendor::AMD && m_amdOpt) {
            config.type = UpscaleConfig::Type::FSR;
        }
    }
    
    // Configure vendor-specific upscaling
    if (config.type == UpscaleConfig::Type::DLSS && m_nvidiaOpt) {
        DLSSConfig::Quality dlssQuality = DLSSConfig::Quality::Quality;
        m_nvidiaOpt->ConfigureDLSS(dlssQuality, renderWidth, renderHeight, 
                                  displayWidth, displayHeight);
    } else if (config.type == UpscaleConfig::Type::FSR && m_amdOpt) {
        FSRConfig::Quality fsrQuality = FSRConfig::Quality::Quality;
        m_amdOpt->ConfigureFSR(fsrQuality, renderWidth, renderHeight,
                              displayWidth, displayHeight);
    }
    
    return config;
}

void VulkanOptimizationManager::OptimizeRenderPipeline() {
    std::cout << "Optimizing render pipeline for " << VendorToString(m_vendor) << " GPU..." << std::endl;
    
    if (m_settings.enableBatching && m_renderBatcher) {
        m_renderBatcher->OptimizeBatches();
        std::cout << "  - Render batching enabled" << std::endl;
    }
    
    if (m_settings.enableTiling && m_tiledRenderer) {
        m_tiledRenderer->OptimizeTiles();
        std::cout << "  - Tiled rendering enabled" << std::endl;
    }
    
    if (m_settings.enableDefragmentation && m_memoryAlloc) {
        m_memoryAlloc->Defragment();
        std::cout << "  - Memory defragmentation enabled" << std::endl;
    }
    
    std::cout << "Pipeline optimization complete" << std::endl;
}

OptimizationSettings VulkanOptimizationManager::GetOptimalSettings() const {
    OptimizationSettings settings = OptimizationSettings::GetDefault();
    
    // Adjust settings based on detected hardware
    if (m_vendor == GPUVendor::NVIDIA) {
        if (m_nvidiaOpt && m_nvidiaOpt->HasMeshShaders()) {
            // RTX 40 series - enable all features
            settings.enableBatching = true;
            settings.enableTiling = true;
            settings.enableAsyncCompute = true;
            settings.enableUpscaling = true;
        }
    } else if (m_vendor == GPUVendor::AMD) {
        if (m_amdOpt) {
            auto rdna = m_amdOpt->GetRDNATuning();
            if (rdna.arch == RDNATuning::Architecture::RDNA3) {
                // RDNA3 - enable all features
                settings.enableBatching = true;
                settings.enableTiling = true;
                settings.enableAsyncCompute = true;
                settings.enableUpscaling = true;
            }
        }
    }
    
    return settings;
}

void VulkanOptimizationManager::ApplyRecommendations() {
    std::cout << "Applying hardware-specific recommendations..." << std::endl;
    
    if (m_vendor == GPUVendor::NVIDIA && m_nvidiaOpt) {
        std::cout << "  NVIDIA Recommendations:" << std::endl;
        std::cout << "    - Use 256MB memory pools" << std::endl;
        std::cout << "    - Aggregate barriers for reduced overhead" << std::endl;
        std::cout << "    - Batch size: 1024 commands" << std::endl;
        std::cout << "    - Warp size: 32 threads" << std::endl;
        
        if (m_nvidiaOpt->SupportsDLSS()) {
            std::cout << "    - DLSS recommended for upscaling" << std::endl;
        }
        if (m_nvidiaOpt->HasMeshShaders()) {
            std::cout << "    - Mesh shaders available for geometry" << std::endl;
        }
    } else if (m_vendor == GPUVendor::AMD && m_amdOpt) {
        std::cout << "  AMD Recommendations:" << std::endl;
        std::cout << "    - Use 128MB memory pools" << std::endl;
        std::cout << "    - Inline barriers for better scheduling" << std::endl;
        std::cout << "    - Batch size: 256 commands" << std::endl;
        std::cout << "    - Wave size: " << m_amdOpt->GetWaveSize() << " threads" << std::endl;
        
        auto rdna = m_amdOpt->GetRDNATuning();
        float speedup = m_amdOpt->EstimateAsyncComputeSpeedup(10, 30);
        std::cout << "    - Async compute speedup: ~" << (speedup * 100 - 100) << "%" << std::endl;
        std::cout << "    - FSR recommended for upscaling" << std::endl;
        
        if (rdna.supportsMeshShaders) {
            std::cout << "    - Mesh shaders available (RDNA3)" << std::endl;
        }
    }
}

PerformanceMetrics VulkanOptimizationManager::GetMetrics() const {
    PerformanceMetrics metrics = {};
    metrics.fps = 0.0f;
    metrics.frameTimeMs = 0.0f;
    metrics.gpuUtilization = 0.0f;
    metrics.memoryUsageMB = 0.0f;
    metrics.drawCalls = 0;
    metrics.batches = 0;
    metrics.batchingEfficiency = 0.0f;
    metrics.overdrawReduction = 0.0f;
    
    // Get actual metrics from components (would be populated during rendering)
    if (m_renderBatcher) {
        auto batchStats = m_renderBatcher->GetStats();
        metrics.drawCalls = batchStats.totalDrawCalls;
        metrics.batches = batchStats.batchedDrawCalls;
        metrics.batchingEfficiency = batchStats.batchingEfficiency;
    }
    
    if (m_tiledRenderer) {
        auto tileStats = m_tiledRenderer->GetStats();
        metrics.overdrawReduction = tileStats.overdrawReduction;
    }
    
    if (m_memoryAlloc) {
        auto memStats = m_memoryAlloc->GetStats();
        metrics.memoryUsageMB = static_cast<float>(memStats.totalMapped) / (1024.0f * 1024.0f);
    }
    
    return metrics;
}

void VulkanOptimizationManager::PrintOptimizationSummary() const {
    std::cout << "\n=== Vulkan Optimization Summary ===" << std::endl;
    std::cout << "GPU: " << m_gpuName << std::endl;
    std::cout << "Vendor: " << VendorToString(m_vendor) << std::endl;
    std::cout << "\nEnabled Features:" << std::endl;
    std::cout << "  Batching: " << (m_settings.enableBatching ? "Yes" : "No") << std::endl;
    std::cout << "  Tiling: " << (m_settings.enableTiling ? "Yes" : "No") << std::endl;
    std::cout << "  Async Compute: " << (m_settings.enableAsyncCompute ? "Yes" : "No") << std::endl;
    std::cout << "  Upscaling: " << (m_settings.enableUpscaling ? "Yes" : "No") << std::endl;
    std::cout << "  Defragmentation: " << (m_settings.enableDefragmentation ? "Yes" : "No") << std::endl;
    std::cout << "  Max Frames in Flight: " << m_settings.maxFramesInFlight << std::endl;
    
    if (m_vendor == GPUVendor::NVIDIA && m_nvidiaOpt) {
        std::cout << "\nNVIDIA-Specific:" << std::endl;
        std::cout << "  Tensor Cores: " << (m_nvidiaOpt->HasTensorCores() ? "Yes" : "No") << std::endl;
        std::cout << "  DLSS Support: " << (m_nvidiaOpt->SupportsDLSS() ? "Yes" : "No") << std::endl;
        std::cout << "  Mesh Shaders: " << (m_nvidiaOpt->HasMeshShaders() ? "Yes" : "No") << std::endl;
        std::cout << "  SER: " << (m_nvidiaOpt->HasSER() ? "Yes" : "No") << std::endl;
    } else if (m_vendor == GPUVendor::AMD && m_amdOpt) {
        auto rdna = m_amdOpt->GetRDNATuning();
        std::cout << "\nAMD-Specific:" << std::endl;
        std::cout << "  Architecture: RDNA" << static_cast<int>(rdna.arch) << std::endl;
        std::cout << "  Compute Units: " << rdna.computeUnits << std::endl;
        std::cout << "  Ray Accelerators: " << rdna.rayAccelerators << std::endl;
        std::cout << "  Wave Size: " << m_amdOpt->GetWaveSize() << std::endl;
        std::cout << "  Async Compute: " << (m_amdOpt->HasAsyncCompute() ? "Yes" : "No") << std::endl;
    }
    
    std::cout << "====================================\n" << std::endl;
}

void VulkanOptimizationManager::GetOptimalDispatch(uint32_t totalThreads,
                                                  uint32_t& groupX, uint32_t& groupY, uint32_t& groupZ) {
    if (m_vendor == GPUVendor::NVIDIA && m_nvidiaOpt) {
        m_nvidiaOpt->GetOptimalDispatch(totalThreads, groupX, groupY, groupZ);
    } else if (m_vendor == GPUVendor::AMD && m_amdOpt) {
        m_amdOpt->GetOptimalDispatch(totalThreads, groupX, groupY, groupZ);
    } else {
        // Generic fallback
        const uint32_t threadsPerBlock = 256;
        uint32_t totalBlocks = (totalThreads + threadsPerBlock - 1) / threadsPerBlock;
        
        groupZ = std::min(totalBlocks, 32u);
        uint32_t remaining = (totalBlocks + groupZ - 1) / groupZ;
        groupY = static_cast<uint32_t>(std::sqrt(remaining));
        groupX = (remaining + groupY - 1) / groupY;
    }
}

} // namespace Vulkan
} // namespace GameTools
