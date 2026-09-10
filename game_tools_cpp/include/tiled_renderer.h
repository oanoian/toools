#ifndef TILED_RENDERER_H
#define TILED_RENDERER_H

#include "vulkan_types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace GameTools {
namespace Vulkan {

// Tile configuration
struct TileConfig {
    uint32_t tileWidth;
    uint32_t tileHeight;
    uint32_t maxPrimitivesPerTile;
    bool useDepthPrepass;
    bool useEarlyZ;
    
    static TileConfig GetNvidiaConfig();
    static TileConfig GetAmdConfig();
    static TileConfig GetGenericConfig();
};

// Tile statistics
struct TileStats {
    uint32_t totalTiles;
    uint32_t activeTiles;
    float averagePrimitivesPerTile;
    float depthComplexity;
    float overdrawReduction;
};

// Tiled renderer for bandwidth optimization
class TiledRenderer {
public:
    TiledRenderer(VkDevice device, GPUVendor vendor);
    ~TiledRenderer();
    
    // Initialize tiled rendering resources
    void Initialize(uint32_t screenWidth, uint32_t screenHeight,
                   const TileConfig& config);
    
    // Partition scene into tiles
    void PartitionScene(const std::vector<Vertex>& vertices,
                       const std::vector<uint32_t>& indices,
                       const Mat4& viewProj);
    
    // Sort primitives by tile (binning)
    void BinPrimitives();
    
    // Record tile-based render commands
    void RecordTileCommands(VkCommandBuffer cmd, VkRenderPass renderPass,
                           VkFramebuffer framebuffer, uint32_t tileIndex);
    
    // Record all tile commands
    void RecordAllTiles(VkCommandBuffer cmd, VkRenderPass renderPass,
                       VkFramebuffer framebuffer);
    
    // Get tile count
    uint32_t GetTileCountX() const { return m_tileCountX; }
    uint32_t GetTileCountY() const { return m_tileCountY; }
    uint32_t GetTotalTiles() const { return m_tileCountX * m_tileCountY; }
    
    // Get statistics
    TileStats GetStats() const;
    
    // Optimize tile processing (vendor-specific)
    void OptimizeTiles();

private:
    struct Primitive {
        Vec3 v0, v1, v2; // triangle vertices in screen space
        uint32_t indexOffset;
        uint32_t materialId;
    };
    
    struct Tile {
        std::vector<uint32_t> primitiveIndices;
        VkRect2D bounds;
        bool hasContent;
    };
    
    VkDevice m_device;
    GPUVendor m_vendor;
    TileConfig m_config;
    
    uint32_t m_screenWidth;
    uint32_t m_screenHeight;
    uint32_t m_tileCountX;
    uint32_t m_tileCountY;
    
    std::vector<Tile> m_tiles;
    std::vector<Primitive> m_primitives;
    
    // Vendor-specific optimizations
    void BinPrimitivesNvidia();
    void BinPrimitivesAmd();
    void OptimizeTileOrderNvidia();
    void OptimizeTileOrderAmd();
    
    // Helper functions
    bool TriangleIntersectsTile(const Primitive& prim, const Tile& tile) const;
    Vec3 TransformToScreenSpace(const Vertex& v, const Mat4& viewProj) const;
};

} // namespace Vulkan
} // namespace GameTools

#endif // TILED_RENDERER_H
