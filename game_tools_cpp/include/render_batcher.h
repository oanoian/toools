#ifndef RENDER_BATCHER_H
#define RENDER_BATCHER_H

#include "vulkan_types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace GameTools {
namespace Vulkan {

// Batch entry for rendering
struct BatchEntry {
    Vertex vertex;
    InstanceData instance;
    uint32_t materialId;
    uint32_t textureId;
};

// Batch statistics
struct BatchStats {
    uint32_t totalDrawCalls;
    uint32_t batchedDrawCalls;
    uint32_t verticesProcessed;
    float batchingEfficiency; // ratio: batched/total
};

// Render batcher with instancing support
class RenderBatcher {
public:
    RenderBatcher(VkDevice device, GPUVendor vendor);
    ~RenderBatcher();
    
    // Initialize resources
    void Initialize(uint32_t maxInstances, uint32_t maxVertices);
    
    // Add a mesh to be rendered
    void AddMesh(const std::vector<Vertex>& vertices,
                const std::vector<uint32_t>& indices,
                const Mat4& modelMatrix,
                const Vec4& color,
                uint32_t materialId,
                uint32_t textureId);
    
    // Flush batches and prepare for rendering
    void FlushBatches();
    
    // Get batch data for rendering
    const std::vector<Vertex>& GetBatchVertices() const { return m_batchVertices; }
    const std::vector<InstanceData>& GetBatchInstances() const { return m_batchInstances; }
    const std::vector<uint32_t>& GetBatchIndices() const { return m_batchIndices; }
    const std::vector<uint32_t>& GetBatchCounts() const { return m_batchCounts; }
    
    // Record draw commands for a batch
    void RecordBatchCommands(VkCommandBuffer cmd, VkPipeline pipeline,
                            VkDescriptorSet descriptorSet,
                            uint32_t firstVertex, uint32_t firstInstance);
    
    // Get statistics
    BatchStats GetStats() const;
    
    // Reset batches
    void Reset();
    
    // Optimize batches (vendor-specific)
    void OptimizeBatches();

private:
    VkDevice m_device;
    GPUVendor m_vendor;
    
    struct MeshKey {
        uint32_t materialId;
        uint32_t textureId;
        size_t vertexHash;
        
        bool operator==(const MeshKey& other) const {
            return materialId == other.materialId && 
                   textureId == other.textureId &&
                   vertexHash == other.vertexHash;
        }
    };
    
    struct MeshKeyHash {
        size_t operator()(const MeshKey& key) const {
            return std::hash<uint32_t>()(key.materialId) ^
                   (std::hash<uint32_t>()(key.textureId) << 1) ^
                   (std::hash<size_t>()(key.vertexHash) << 2);
        }
    };
    
    std::unordered_map<MeshKey, std::vector<InstanceData>, MeshKeyHash> m_batches;
    std::vector<Vertex> m_batchVertices;
    std::vector<InstanceData> m_batchInstances;
    std::vector<uint32_t> m_batchIndices;
    std::vector<uint32_t> m_batchCounts; // instances per batch
    
    uint32_t m_totalDrawCalls;
    uint32_t m_currentBatchSize;
    uint32_t m_maxInstancesPerBatch;
    
    // Vendor-specific optimization
    void SortBatchesNvidia();
    void SortBatchesAmd();
    size_t HashVertices(const std::vector<Vertex>& vertices) const;
};

} // namespace Vulkan
} // namespace GameTools

#endif // RENDER_BATCHER_H
