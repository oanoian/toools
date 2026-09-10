/**
 * Alpha1 Version Control Abstraction
 * 
 * Unified API for Perforce and Git LFS operations.
 * 
 * @file version_control.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <vector>

namespace Alpha1::VCS {

using namespace Core;

/**
 * Version control system type
 */
enum class VCSType {
    Perforce,
    GitLFS,
    Unknown
};

/**
 * File lock information
 */
struct LockInfo {
    std::string filePath;
    std::string lockedBy;
    int64_t lockTime;
};

/**
 * Version Control abstraction layer
 */
class VersionControl {
public:
    /**
     * Initialize VCS connection
     */
    Result<void> Initialize(const std::string& workspacePath);
    
    /**
     * Get VCS type in use
     */
    VCSType GetType() const;
    
    /**
     * Lock a file (exclusive checkout)
     */
    Result<void> LockFile(const std::string& filePath);
    
    /**
     * Unlock a file
     */
    Result<void> UnlockFile(const std::string& filePath);
    
    /**
     * Check if file is locked by another user
     */
    bool IsFileLocked(const std::string& filePath) const;
    
    /**
     * Get all locks
     */
    std::vector<LockInfo> GetLocks() const;
    
    /**
     * Submit changes with description
     */
    Result<void> Submit(const std::string& description);
    
    /**
     * Sync to latest
     */
    Result<void> Sync();
    
private:
    VCSType m_type = VCSType::Unknown;
    bool m_initialized = false;
};

} // namespace Alpha1::VCS
