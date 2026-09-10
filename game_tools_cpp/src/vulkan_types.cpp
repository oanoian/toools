#include "vulkan_types.h"
#include <cmath>
#include <cstring>

namespace GameTools {
namespace Vulkan {

// PipelineStage implementations
PipelineStage PipelineStage::TopOfPipe() {
    return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
}

PipelineStage PipelineStage::BottomOfPipe() {
    return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
}

PipelineStage PipelineStage::VertexInput() {
    return {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
}

PipelineStage PipelineStage::VertexShader() {
    return {VK_PIPELINE_STAGE_VERTEX_SHADER_BIT};
}

PipelineStage PipelineStage::FragmentShader() {
    return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};
}

PipelineStage PipelineStage::ComputeShader() {
    return {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
}

PipelineStage PipelineStage::Transfer() {
    return {VK_PIPELINE_STAGE_TRANSFER_BIT};
}

PipelineStage PipelineStage::ColorAttachment() {
    return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
}

PipelineStage PipelineStage::Host() {
    return {VK_PIPELINE_STAGE_HOST_BIT};
}

// AccessMask implementations
AccessMask AccessMask::None() {
    return {0};
}

AccessMask AccessMask::VertexRead() {
    return {VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT};
}

AccessMask AccessMask::IndexRead() {
    return {VK_ACCESS_INDEX_READ_BIT};
}

AccessMask AccessMask::ShaderRead() {
    return {VK_ACCESS_SHADER_READ_BIT};
}

AccessMask AccessMask::ShaderWrite() {
    return {VK_ACCESS_SHADER_WRITE_BIT};
}

AccessMask AccessMask::TransferRead() {
    return {VK_ACCESS_TRANSFER_READ_BIT};
}

AccessMask AccessMask::TransferWrite() {
    return {VK_ACCESS_TRANSFER_WRITE_BIT};
}

AccessMask AccessMask::ColorAttachmentRead() {
    return {VK_ACCESS_COLOR_ATTACHMENT_READ_BIT};
}

AccessMask AccessMask::ColorAttachmentWrite() {
    return {VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
}

AccessMask AccessMask::DepthStencilAttachmentRead() {
    return {VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
}

AccessMask AccessMask::DepthStencilAttachmentWrite() {
    return {VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
}

// Mat4 implementations
Mat4::Mat4() {
    std::memset(data, 0, sizeof(data));
}

Mat4 Mat4::Identity() {
    Mat4 result;
    result.data[0] = 1.0f;
    result.data[5] = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

Mat4 Mat4::Perspective(float fov, float aspect, float near, float far) {
    Mat4 result;
    float tanHalfFov = std::tan(fov / 2.0f);
    
    result.data[0] = 1.0f / (aspect * tanHalfFov);
    result.data[5] = 1.0f / tanHalfFov;
    result.data[10] = -(far + near) / (far - near);
    result.data[11] = -1.0f;
    result.data[14] = -(2.0f * far * near) / (far - near);
    
    return result;
}

Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = Vec3(center.x - eye.x, center.y - eye.y, center.z - eye.z);
    float len = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
    f.x /= len; f.y /= len; f.z /= len;
    
    Vec3 r = Vec3(
        f.y * up.z - f.z * up.y,
        f.z * up.x - f.x * up.z,
        f.x * up.y - f.y * up.x
    );
    len = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
    r.x /= len; r.y /= len; r.z /= len;
    
    Vec3 u = Vec3(
        r.y * f.z - r.z * f.y,
        r.z * f.x - r.x * f.z,
        r.x * f.y - r.y * f.x
    );
    
    Mat4 result;
    result.data[0] = r.x;
    result.data[1] = u.x;
    result.data[2] = -f.x;
    result.data[3] = 0.0f;
    
    result.data[4] = r.y;
    result.data[5] = u.y;
    result.data[6] = -f.y;
    result.data[7] = 0.0f;
    
    result.data[8] = r.z;
    result.data[9] = u.z;
    result.data[10] = -f.z;
    result.data[11] = 0.0f;
    
    result.data[12] = -(r.x * eye.x + r.y * eye.y + r.z * eye.z);
    result.data[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    result.data[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    result.data[15] = 1.0f;
    
    return result;
}

// AABB implementations
bool AABB::intersects(const AABB& other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y) &&
           (min.z <= other.max.z && max.z >= other.min.z);
}

bool AABB::contains(const Vec3& point) const {
    return (point.x >= min.x && point.x <= max.x) &&
           (point.y >= min.y && point.y <= max.y) &&
           (point.z >= min.z && point.z <= max.z);
}

// Sphere implementations
bool Sphere::intersects(const Sphere& other) const {
    float dx = center.x - other.center.x;
    float dy = center.y - other.center.y;
    float dz = center.z - other.center.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float radiusSum = radius + other.radius;
    return distSq <= radiusSum * radiusSum;
}

bool Sphere::contains(const Vec3& point) const {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    float dz = point.z - center.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    return distSq <= radius * radius;
}

// Vendor detection
GPUVendor DetectVendor(uint32_t vendorId) {
    switch (vendorId) {
        case 0x10DE: return GPUVendor::NVIDIA;
        case 0x1002: return GPUVendor::AMD;
        case 0x8086: return GPUVendor::INTEL;
        default: return GPUVendor::UNKNOWN;
    }
}

std::string VendorToString(GPUVendor vendor) {
    switch (vendor) {
        case GPUVendor::NVIDIA: return "NVIDIA";
        case GPUVendor::AMD: return "AMD";
        case GPUVendor::INTEL: return "Intel";
        default: return "Unknown";
    }
}

} // namespace Vulkan
} // namespace GameTools
