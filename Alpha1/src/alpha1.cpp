/**
 * Alpha1 Framework Implementation
 * 
 * Main initialization, shutdown, and runtime management.
 * 
 * @file alpha1.cpp
 */

#include "alpha1/alpha1.h"
#include "alpha1/core/logging.h"
#include <atomic>
#include <chrono>

namespace Alpha1 {

using namespace Core;

// Type aliases for time management
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Duration = std::chrono::duration<float>;

// Global state
static std::atomic<bool> s_initialized{false};
static std::atomic<float> s_frameTime{0.0f};
static std::atomic<float> s_fps{0.0f};
static TimePoint s_lastFrameTime;
static TimePoint s_startTime;

Result<void> Initialize(const char* config_path) {
    if (s_initialized.load()) {
        return Result<void>("Alpha1 framework already initialized");
    }
    
    // Record start time
    s_startTime = std::chrono::high_resolution_clock::now();
    s_lastFrameTime = s_startTime;
    
    // Initialize logging
    Logger::Instance().SetLogLevel(LogLevel::Info);
    
    if (config_path) {
        Logger::Instance().SetLogFile(std::string(config_path) + ".log");
    }
    
    LOG_INFO(std::string("Alpha1 Framework v") + VERSION_STRING + " initializing...");
    
    // TODO: Initialize subsystems in order:
    // 1. Memory managers
    // 2. Thread pools
    // 3. Asset manager
    // 4. Renderer (Vulkan)
    // 5. ECS world
    // 6. DCC pipeline
    // 7. Version control
    
    s_initialized.store(true);
    LOG_INFO("Alpha1 Framework initialized successfully");
    
    return Result<void>();
}

void Shutdown() {
    if (!s_initialized.load()) {
        return;
    }
    
    LOG_INFO("Shutting down Alpha1 Framework...");
    
    // TODO: Shutdown subsystems in reverse order:
    // 1. Version control
    // 2. DCC pipeline
    // 3. ECS world
    // 4. Renderer
    // 5. Asset manager
    // 6. Thread pools
    // 7. Memory managers
    
    s_initialized.store(false);
    LOG_INFO("Alpha1 Framework shutdown complete");
}

bool IsInitialized() {
    return s_initialized.load();
}

float GetFrameTime() {
    return s_frameTime.load();
}

float GetFPS() {
    return s_fps.load();
}

void Update() {
    if (!s_initialized.load()) {
        return;
    }
    
    // Calculate frame time
    TimePoint now = std::chrono::high_resolution_clock::now();
    Duration delta = now - s_lastFrameTime;
    s_frameTime.store(delta.count());
    s_lastFrameTime = now;
    
    // Calculate FPS
    static int frameCount = 0;
    static float fpsTimer = 0.0f;
    
    ++frameCount;
    fpsTimer += s_frameTime.load();
    
    if (fpsTimer >= 1.0f) {
        s_fps.store(static_cast<float>(frameCount) / fpsTimer);
        frameCount = 0;
        fpsTimer = 0.0f;
    }
}

} // namespace Alpha1
