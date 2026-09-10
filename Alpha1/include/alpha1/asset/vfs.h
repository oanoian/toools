/**
 * Alpha1 Virtual File System
 * 
 * Platform-independent file access abstraction.
 * 
 * @file vfs.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <vector>

namespace Alpha1::Asset {

using namespace Core;

/**
 * Virtual File System for platform-independent file access
 */
class VFS {
public:
    static VFS& Instance() {
        static VFS instance;
        return instance;
    }
    
    /**
     * Mount a physical path to a virtual path
     */
    Result<void> Mount(const std::string& virtualPath, const std::string& physicalPath);
    
    /**
     * Read entire file into buffer
     */
    Result<std::vector<uint8_t>> ReadFile(const std::string& path);
    
    /**
     * Check if file exists
     */
    bool Exists(const std::string& path) const;
    
    /**
     * Get file size
     */
    Result<MemorySize> GetFileSize(const std::string& path) const;
    
private:
    VFS() = default;
    ~VFS() = default;
};

} // namespace Alpha1::Asset
