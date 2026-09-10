#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace GameTools {
namespace Vulkan {

// GPU Vendor identification
enum class GPUVendor {
    NVIDIA,
    AMD,
    INTEL,
    UNKNOWN
};

// Memory heap types
enum class MemoryType {
    DEVICE_LOCAL,
    HOST_VISIBLE,
    HOST_COHERENT,
    LAZILY_ALLOCATED
};

// Resource states for barriers
enum class ResourceState {
    UNDEFINED,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    TRANSFER_SRC,
    TRANSFER_DST,
    SHADER_READ,
    SHADER_WRITE,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    PRESENT_SRC,
    GENERAL
};

// Pipeline stage flags wrapper
struct PipelineStage {
    VkPipelineStageFlags flags;
    static PipelineStage TopOfPipe();
    static PipelineStage BottomOfPipe();
    static PipelineStage VertexInput();
    static PipelineStage VertexShader();
    static PipelineStage FragmentShader();
    static PipelineStage ComputeShader();
    static PipelineStage Transfer();
    static PipelineStage ColorAttachment();
    static PipelineStage Host();
};

// Access flags wrapper
struct AccessMask {
    VkAccessFlags flags;
    static AccessMask None();
    static AccessMask VertexRead();
    static AccessMask IndexRead();
    static AccessMask ShaderRead();
    static AccessMask ShaderWrite();
    static AccessMask TransferRead();
    static AccessMask TransferWrite();
    static AccessMask ColorAttachmentRead();
    static AccessMask ColorAttachmentWrite();
    static AccessMask DepthStencilAttachmentRead();
    static AccessMask DepthStencilAttachmentWrite();
};

// Vector math structures
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

struct Mat4 {
    float data[16];
    Mat4();
    static Mat4 Identity();
    static Mat4 Perspective(float fov, float aspect, float near, float far);
    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
};

// Vertex structure for rendering
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
    Vec4 color;
};

// Instance data for batching
struct InstanceData {
    Mat4 model;
    Vec4 color;
};

// Collision primitives
struct AABB {
    Vec3 min;
    Vec3 max;
    bool intersects(const AABB& other) const;
    bool contains(const Vec3& point) const;
};

struct Sphere {
    Vec3 center;
    float radius;
    bool intersects(const Sphere& other) const;
    bool contains(const Vec3& point) const;
};

// Utility functions
GPUVendor DetectVendor(uint32_t vendorId);
std::string VendorToString(GPUVendor vendor);

} // namespace Vulkan
} // namespace GameTools

#endif // VULKAN_TYPES_H
