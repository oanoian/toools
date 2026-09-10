/**
 * Alpha1 Hot Reloader
 * 
 * Runtime DLL hot-reloading with state preservation.
 * 
 * @file hot_reloader.h
 */

#pragma once

#include "alpha1/core/types.h"
#include "alpha1/core/threading.h"
#include <string>
#include <functional>
#include <atomic>

namespace Alpha1::DLL {

using namespace Core;

/**
 * Hot Reload state
 */
enum class ReloadState {
    Idle,
    Monitoring,
    Reloading,
    Error
};

/**
 * Hot Reloader for runtime code updates
 */
class HotReloader {
public:
    /**
     * Start monitoring a DLL for changes
     */
    Result<void> StartMonitoring(const std::string& dllPath);
    
    /**
     * Stop monitoring
     */
    void StopMonitoring();
    
    /**
     * Manually trigger reload
     */
    Result<void> Reload();
    
    /**
     * Get current state
     */
    ReloadState GetState() const;
    
    /**
     * Set reload callback
     */
    void OnReload(std::function<void()> callback);
    
    /**
     * Preserve state before reload
     */
    void* PreserveState(size_t size);
    
    /**
     * Restore state after reload
     */
    void RestoreState(void* data, size_t size);
    
private:
    void MonitorThread();
    
    ReloadState m_state = ReloadState::Idle;
    std::string m_dllPath;
    std::thread m_monitorThread;
    std::atomic<bool> m_running{false};
    std::function<void()> m_reloadCallback;
    void* m_preservedState = nullptr;
    size_t m_stateSize = 0;
};

} // namespace Alpha1::DLL
