/**
 * Alpha1 Vulkan Engine
 * 
 * Cross-platform Vulkan renderer with vendor-specific optimizations.
 * 
 * @file vulkan_engine.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <vector>

namespace Alpha1::Renderer {

using namespace Core;

/**
 * Vulkan engine configuration
 */
struct VulkanConfig {
    bool enableValidationLayers = true;
    bool enableDebugUtils = true;
    uint32_t maxFramesInFlight = 2;
    bool useTiledRendering = false;
};

/**
 * GPU information
 */
struct GPUInfo {
    std::string name;
    uint32_t vendorID;
    uint32_t deviceID;
    size_t dedicatedMemoryMB;
    bool isNVIDIA;
    bool isAMD;
    bool isIntel;
};

/**
 * Vulkan Renderer Engine
 */
class VulkanEngine {
public:
    /**
     * Initialize Vulkan engine
     */
    Result<void> Initialize(const VulkanConfig& config = VulkanConfig());
    
    /**
     * Shutdown Vulkan engine
     */
    void Shutdown();
    
    /**
     * Get GPU information
     */
    const GPUInfo& GetGPUInfo() const;
    
    /**
     * Begin frame rendering
     */
    Result<void> BeginFrame();
    
    /**
     * End frame rendering
     */
    Result<void> EndFrame();
    
    /**
     * Wait for GPU to finish
     */
    void WaitForGPU();
    
private:
    bool m_initialized = false;
    GPUInfo m_gpuInfo;
};

} // namespace Alpha1::Renderer
