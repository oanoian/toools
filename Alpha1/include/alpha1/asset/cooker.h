/**
 * Alpha1 Asset Cooker
 * 
 * Compiles raw assets into platform-optimized binary formats.
 * 
 * @file cooker.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <vector>

namespace Alpha1::Asset {

using namespace Core;

/**
 * Asset cooking configuration
 */
struct CookConfig {
    bool compressTextures = true;
    bool optimizeMeshes = true;
    bool generateLODs = false;
    int textureQuality = 2; // 0=low, 1=medium, 2=high
};

/**
 * Asset Cooker - converts source assets to runtime formats
 */
class AssetCooker {
public:
    /**
     * Cook a single asset
     */
    Result<void> CookAsset(const std::string& sourcePath, const std::string& outputPath, const CookConfig& config = CookConfig());
    
    /**
     * Cook multiple assets in parallel
     */
    Result<void> CookBatch(const std::vector<std::pair<std::string, std::string>>& assets, const CookConfig& config = CookConfig());
    
    /**
     * Get cooking progress (0.0 to 1.0)
     */
    float GetProgress() const;
    
    /**
     * Cancel current cooking operation
     */
    void Cancel();
    
private:
    std::atomic<bool> m_cancelled{false};
    std::atomic<float> m_progress{0.0f};
};

} // namespace Alpha1::Asset
