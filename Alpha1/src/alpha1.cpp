/**
 * Alpha1 Framework - Main Implementation
 * Framework initialization, shutdown, and lifecycle management
 */

#include "alpha1/alpha1.h"
#include "alpha1/core/logging.h"
#include <atomic>
#include <string>

namespace Alpha1 {

// Static framework state
static std::atomic<bool> s_initialized{false};
static FrameworkConfig s_config;

Result<void> Framework::Initialize(const FrameworkConfig& config) {
    if (s_initialized.exchange(true)) {
        return Result<void>::Error("Framework already initialized");
    }
    
    s_config = config;
    
    // Initialize logging system
    Core::Logger::Instance().SetLogLevel(config.log_level);
    
    LOG_INFO("Alpha1 Framework initializing...");
    LOG_INFO("  Version: " + std::to_string(ALPHA1_VERSION_MAJOR) + "." + 
             std::to_string(ALPHA1_VERSION_MINOR) + "." + 
             std::to_string(ALPHA1_VERSION_PATCH));
    
    LOG_INFO("Framework initialization complete");
    
    return Result<void>::Ok();
}

Result<void> Framework::Shutdown() {
    if (!s_initialized.exchange(false)) {
        return Result<void>::Error("Framework not initialized");
    }
    
    LOG_INFO("Shutting down Alpha1 Framework...");
    
    // Cleanup logging
    Core::Logger::Instance().Log(Core::LogLevel::Info, "Framework shutdown complete");
    
    // Reset configuration
    s_config = FrameworkConfig{};
    
    return Result<void>::Ok();
}

bool Framework::IsInitialized() {
    return s_initialized.load();
}

const FrameworkConfig& Framework::GetConfig() {
    return s_config;
}

} // namespace Alpha1
