/**
 * Alpha1 USD Pipeline
 * 
 * Universal Scene Description integration for DCC interoperability.
 * 
 * @file usd_pipeline.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <vector>

namespace Alpha1::DCC {

using namespace Core;

/**
 * USD Pipeline for scene import/export
 */
class USDPipeline {
public:
    /**
     * Import USD scene (.usda, .usdc, or .usdz)
     */
    Result<void> ImportScene(const std::string& usdPath);
    
    /**
     * Export current scene to USD
     */
    Result<void> ExportScene(const std::string& usdPath);
    
    /**
     * Apply USD layer on top of base scene
     */
    Result<void> ApplyLayer(const std::string& layerPath);
    
    /**
     * Get supported USD file extensions
     */
    static std::vector<std::string> GetSupportedExtensions();
    
private:
    bool m_sceneLoaded = false;
};

} // namespace Alpha1::DCC
