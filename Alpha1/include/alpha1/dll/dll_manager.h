/**
 * Alpha1 DLL Manager
 * 
 * Dynamic library loading and management.
 * 
 * @file dll_manager.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <unordered_map>

namespace Alpha1::DLL {

using namespace Core;

/**
 * Dynamic library handle
 */
using LibraryHandle = void*;

/**
 * DLL Manager for dynamic library operations
 */
class DLLManager {
public:
    static DLLManager& Instance() {
        static DLLManager instance;
        return instance;
    }
    
    /**
     * Load a dynamic library
     */
    Result<LibraryHandle> LoadLibrary(const std::string& path);
    
    /**
     * Unload a dynamic library
     */
    Result<void> UnloadLibrary(LibraryHandle handle);
    
    /**
     * Get function pointer from library
     */
    template<typename T>
    Result<T> GetFunction(LibraryHandle handle, const std::string& name);
    
    /**
     * Check if library is loaded
     */
    bool IsLoaded(const std::string& path) const;
    
private:
    DLLManager() = default;
    ~DLLManager();
    
    std::unordered_map<std::string, LibraryHandle> m_libraries;
};

} // namespace Alpha1::DLL
