#ifndef COMMAND_BUFFER_MANAGER_H
#define COMMAND_BUFFER_MANAGER_H

#include "vulkan_types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>

namespace GameTools {
namespace Vulkan {

// Command buffer batch configuration
struct BatchConfig {
    uint32_t maxCommandsPerBatch;
    bool useSecondaryBuffers;
    bool aggregateBarriers;
    uint32_t poolSize;
    
    static BatchConfig GetNvidiaConfig();
    static BatchConfig GetAmdConfig();
    static BatchConfig GetGenericConfig();
};

// Barrier description
struct BarrierDesc {
    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    VkAccessFlags srcAccess;
    VkAccessFlags dstAccess;
    VkImage image;
    VkBuffer buffer;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    VkImageSubresourceRange subresourceRange;
    VkBufferMemoryBarrier bufferBarrier;
};

// Command buffer recording context
struct RecordingContext {
    VkCommandBuffer primary;
    std::vector<VkCommandBuffer> secondary;
    uint32_t currentBatch;
    bool isRecording;
};

// Command buffer manager with vendor optimizations
class CommandBufferManager {
public:
    CommandBufferManager(VkDevice device, VkQueue graphicsQueue, 
                        GPUVendor vendor, const BatchConfig& config);
    ~CommandBufferManager();
    
    // Initialize command pools and buffers
    void Initialize(uint32_t frameCount);
    
    // Begin recording commands
    void BeginFrame(uint32_t frameIndex);
    
    // End recording commands
    void EndFrame(uint32_t frameIndex);
    
    // Submit command buffers
    void Submit(uint32_t frameIndex, VkFence fence, 
               VkSemaphore waitSemaphore, VkSemaphore signalSemaphore,
               VkPipelineStageFlags waitStage);
    
    // Record a barrier (vendor-optimized)
    void RecordBarrier(VkCommandBuffer cmd, const BarrierDesc& barrier);
    
    // Aggregate multiple barriers into one (NVIDIA optimization)
    void RecordAggregatedBarriers(VkCommandBuffer cmd, 
                                 const std::vector<BarrierDesc>& barriers);
    
    // Inline barriers (AMD optimization)
    void RecordInlineBarrier(VkCommandBuffer cmd, const BarrierDesc& barrier);
    
    // Begin/End render pass
    void BeginRenderPass(VkCommandBuffer cmd, VkRenderPass renderPass,
                        VkFramebuffer framebuffer, VkRect2D renderArea,
                        const std::vector<VkClearValue>& clearValues);
    void EndRenderPass(VkCommandBuffer cmd);
    
    // Bind pipeline
    void BindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint,
                     VkPipeline pipeline);
    
    // Bind vertex/index buffers
    void BindVertexBuffer(VkCommandBuffer cmd, VkBuffer buffer, 
                         VkDeviceSize offset, uint32_t binding);
    void BindIndexBuffer(VkCommandBuffer cmd, VkBuffer buffer,
                        VkDeviceSize offset, VkIndexType indexType);
    
    // Draw commands
    void Draw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount,
             uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(VkCommandBuffer cmd, uint32_t indexCount,
                    uint32_t instanceCount, uint32_t firstIndex,
                    int32_t vertexOffset, uint32_t firstInstance);
    void DrawIndirect(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset,
                     uint32_t drawCount, uint32_t stride);
    
    // Dispatch compute shader
    void DispatchCompute(VkCommandBuffer cmd, uint32_t groupCountX,
                        uint32_t groupCountY, uint32_t groupCountZ);
    
    // Copy commands
    void CopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst,
                   VkDeviceSize size);
    void CopyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst,
                          VkExtent3D extent);
    
    // Reset command pools
    void ResetPools();

private:
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    GPUVendor m_vendor;
    BatchConfig m_config;
    
    std::vector<VkCommandPool> m_commandPools;
    std::vector<std::vector<VkCommandBuffer>> m_commandBuffers;
    std::vector<std::vector<VkCommandBuffer>> m_secondaryBuffers;
    
    uint32_t m_frameCount;
    
    // Vendor-specific optimization methods
    void OptimizeBarriersNvidia(std::vector<BarrierDesc>& barriers);
    void OptimizeBarriersAmd(std::vector<BarrierDesc>& barriers);
    uint32_t GetOptimalBatchSize() const;
};

} // namespace Vulkan
} // namespace GameTools

#endif // COMMAND_BUFFER_MANAGER_H
