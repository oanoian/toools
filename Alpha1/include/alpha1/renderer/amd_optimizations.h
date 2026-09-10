/**
 * Alpha1 AMD Optimizations
 * 
 * GPU-specific optimizations for AMD hardware.
 * 
 * @file amd_optimizations.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>

namespace Alpha1::Renderer {

using namespace Core;

/**
 * FSR configuration
 */
struct FSRConfig {
    bool enabled = false;
    int qualityLevel = 2; // 0=ultra-performance, 1=performance, 2=balanced, 3=quality
    float sharpness = 0.2f;
};

/**
 * AMD-specific optimizations
 */
class AmdOptimizations {
public:
    /**
     * Detect if current GPU is AMD
     */
    static bool IsAMDGPU();
    
    /**
     * Enable FSR (FidelityFX Super Resolution)
     */
    Result<void> EnableFSR(const FSRConfig& config);
    
    /**
     * Enable async compute optimization
     */
    Result<void> EnableAsyncCompute();
    
    /**
     * Get optimal wave size (64 for RDNA, 32 for older)
     */
    static int GetOptimalWaveSize();
    
    /**
     * Get optimal memory pool size for AMD (128MB default)
     */
    static size_t GetOptimalMemoryPoolSize();
    
private:
    bool m_FSR_Enabled = false;
    bool m_asyncComputeEnabled = false;
};

} // namespace Alpha1::Renderer
