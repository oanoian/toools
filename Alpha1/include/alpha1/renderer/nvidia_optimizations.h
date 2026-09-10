/**
 * Alpha1 NVIDIA Optimizations
 * 
 * GPU-specific optimizations for NVIDIA hardware.
 * 
 * @file nvidia_optimizations.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>

namespace Alpha1::Renderer {

using namespace Core;

/**
 * DLSS configuration
 */
struct DLSSConfig {
    bool enabled = false;
    int qualityLevel = 2; // 0=performance, 1=balanced, 2=quality
};

/**
 * NVIDIA-specific optimizations
 */
class NvidiaOptimizations {
public:
    /**
     * Detect if current GPU is NVIDIA
     */
    static bool IsNVIDIAGPU();
    
    /**
     * Enable Tensor Core acceleration (DLSS)
     */
    Result<void> EnableTensorCores(const DLSSConfig& config);
    
    /**
     * Enable Mesh Shaders for geometry
     */
    Result<void> EnableMeshShaders();
    
    /**
     * Enable Shader Execution Reordering (SER)
     */
    Result<void> EnableSER();
    
    /**
     * Get optimal memory pool size for NVIDIA (256MB default)
     */
    static size_t GetOptimalMemoryPoolSize();
    
    /**
     * Get warp size (always 32 for NVIDIA)
     */
    static constexpr int GetWarpSize() { return 32; }
    
private:
    bool m_tensorCoresEnabled = false;
    bool m_meshShadersEnabled = false;
    bool m_SEREnabled = false;
};

} // namespace Alpha1::Renderer
