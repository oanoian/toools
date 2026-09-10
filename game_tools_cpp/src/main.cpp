#include <iostream>
#include <vulkan/vulkan.h>
#include "vulkan_optimization_manager.h"

using namespace GameTools::Vulkan;

int main() {
    std::cout << "=== Vulkan Graphics Optimization Demo ===" << std::endl;
    std::cout << "Game Tools C++ - NVIDIA & AMD Optimizations\n" << std::endl;
    
    // Initialize Vulkan (minimal setup for demo)
    VkInstance instance = VK_NULL_HANDLE;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GameTools Demo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GameTools Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cout << "Warning: Could not create Vulkan instance (no GPU?)" << std::endl;
        std::cout << "Running in demo mode with simulated hardware...\n" << std::endl;
        
        // Demo mode - show what the system would do
        std::cout << "=== NVIDIA RTX 4090 Simulation ===" << std::endl;
        std::cout << "Memory Pool Size: 256MB (NVIDIA optimized)" << std::endl;
        std::cout << "Barrier Strategy: Aggregated (3x reduction)" << std::endl;
        std::cout << "Batch Size: 1024 commands" << std::endl;
        std::cout << "Warp Size: 32 threads" << std::endl;
        std::cout << "DLSS: Supported" << std::endl;
        std::cout << "Mesh Shaders: Supported" << std::endl;
        std::cout << "SER: Supported\n" << std::endl;
        
        std::cout << "=== AMD RX 7900 XTX Simulation ===" << std::endl;
        std::cout << "Memory Pool Size: 128MB (AMD optimized)" << std::endl;
        std::cout << "Barrier Strategy: Inline (better scheduling)" << std::endl;
        std::cout << "Batch Size: 256 commands" << std::endl;
        std::cout << "Wave Size: 64 threads (wave64)" << std::endl;
        std::cout << "FSR: Supported" << std::endl;
        std::cout << "Async Compute: Up to 22.5% speedup" << std::endl;
        std::cout << "Mesh Shaders: Supported (RDNA3)\n" << std::endl;
        
        return 0;
    }
    
    // Enumerate physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        std::cout << "No Vulkan devices found!" << std::endl;
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    // Use first device
    VkPhysicalDevice physicalDevice = devices[0];
    
    // Get device properties
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    
    std::cout << "GPU: " << props.deviceName << std::endl;
    std::cout << "Vendor ID: 0x" << std::hex << props.vendorID << std::dec << std::endl;
    std::cout << "Device ID: 0x" << std::hex << props.deviceID << std::dec << std::endl;
    std::cout << std::endl;
    
    // Create logical device (simplified)
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    
    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        std::cout << "Failed to create logical device!" << std::endl;
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    // Create optimization manager
    VulkanOptimizationManager optimizer(instance, device, physicalDevice);
    
    // Initialize with default settings
    OptimizationSettings settings = OptimizationSettings::GetDefault();
    optimizer.Initialize(settings);
    
    // Print optimization summary
    optimizer.PrintOptimizationSummary();
    
    // Apply recommendations
    optimizer.ApplyRecommendations();
    
    // Configure upscaling
    UpscaleConfig upscaleConfig = optimizer.ConfigureUpscaling(
        UpscaleConfig::Type::NONE,  // Auto-detect
        1920, 1080,  // Render resolution
        3840, 2160   // Display resolution
    );
    
    std::cout << "\nUpscaling configured:" << std::endl;
    switch (upscaleConfig.type) {
        case UpscaleConfig::Type::DLSS:
            std::cout << "  Type: DLSS (NVIDIA)" << std::endl;
            break;
        case UpscaleConfig::Type::FSR:
            std::cout << "  Type: FSR (AMD)" << std::endl;
            break;
        default:
            std::cout << "  Type: None" << std::endl;
            break;
    }
    std::cout << "  Render: " << upscaleConfig.renderWidth << "x" << upscaleConfig.renderHeight << std::endl;
    std::cout << "  Display: " << upscaleConfig.displayWidth << "x" << upscaleConfig.displayHeight << std::endl;
    
    // Optimize render pipeline
    std::cout << std::endl;
    optimizer.OptimizeRenderPipeline();
    
    // Get optimal compute dispatch
    uint32_t groupX, groupY, groupZ;
    optimizer.GetOptimalDispatch(65536, groupX, groupY, groupZ);
    std::cout << "\nOptimal compute dispatch for 65536 threads:" << std::endl;
    std::cout << "  Group: (" << groupX << ", " << groupY << ", " << groupZ << ")" << std::endl;
    
    // Cleanup
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    return 0;
}
