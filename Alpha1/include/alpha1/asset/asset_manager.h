/**
 * Alpha1 Asset Manager
 * 
 * Manages asset loading, caching, and lifecycle.
 * 
 * @file asset_manager.h
 */

#pragma once

#include "alpha1/core/types.h"
#include "alpha1/core/memory.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Alpha1::Asset {

using namespace Core;

/**
 * Asset base class
 */
struct IAsset {
    virtual ~IAsset() = default;
    virtual void Unload() = 0;
    virtual bool IsLoaded() const = 0;
};

/**
 * Asset handle with reference counting
 */
class AssetManager {
public:
    static AssetManager& Instance() {
        static AssetManager instance;
        return instance;
    }
    
    /**
     * Load an asset from path
     */
    template<typename T>
    Result<std::shared_ptr<T>> Load(const std::string& path) {
        // Stub implementation
        return Result<std::shared_ptr<T>>("Not implemented");
    }
    
    /**
     * Unload an asset
     */
    void Unload(AssetHandle handle);
    
    /**
     * Unload all assets
     */
    void UnloadAll();
    
    /**
     * Get memory usage
     */
    MemorySize GetMemoryUsage() const;
    
private:
    AssetManager() = default;
    ~AssetManager() = default;
    
    std::unordered_map<AssetHandle, std::shared_ptr<IAsset>> m_assets;
};

} // namespace Alpha1::Asset
