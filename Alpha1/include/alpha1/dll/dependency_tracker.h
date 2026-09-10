/**
 * Alpha1 Dependency Tracker
 * 
 * Tracks and resolves DLL dependencies with topological sorting.
 * 
 * @file dependency_tracker.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Alpha1::DLL {

using namespace Core;

/**
 * Dependency graph node
 */
struct DependencyNode {
    std::string path;
    std::vector<std::string> dependencies;
    bool resolved = false;
};

/**
 * Dependency Tracker for optimal load ordering
 */
class DependencyTracker {
public:
    /**
     * Analyze DLL dependencies
     */
    Result<void> Analyze(const std::string& dllPath);
    
    /**
     * Get load order (topologically sorted)
     */
    Result<std::vector<std::string>> GetLoadOrder() const;
    
    /**
     * Check for circular dependencies
     */
    bool HasCircularDependency() const;
    
    /**
     * Clear all tracked dependencies
     */
    void Clear();
    
private:
    bool DetectCycle(const std::string& node, 
                     std::unordered_set<std::string>& visited,
                     std::unordered_set<std::string>& recStack) const;
    
    std::unordered_map<std::string, DependencyNode> m_dependencies;
};

} // namespace Alpha1::DLL
