/**
 * @file test_dll_optimization.cpp
 * @brief Comprehensive tests for DLL optimization module
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <string>
#include "dll_optimization/dll_manager.hpp"

using namespace gametools::dll_opt;

// Test results tracker
struct TestResults {
    size_t passed = 0;
    size_t failed = 0;
    
    void record(bool success, const std::string& testName) {
        if (success) {
            std::cout << "[PASS] " << testName << std::endl;
            passed++;
        } else {
            std::cout << "[FAIL] " << testName << std::endl;
            failed++;
        }
    }
    
    void printSummary() {
        std::cout << "\n========================================\n";
        std::cout << "Test Summary: " << passed << " passed, " 
                  << failed << " failed\n";
        std::cout << "========================================\n";
    }
};

static TestResults g_results;

#define TEST(name, func) \
    try { \
        bool result = func(); \
        g_results.record(result, name); \
    } catch (const std::exception& e) { \
        g_results.record(false, std::string(name) + " - Exception: " + e.what()); \
    }

// ============================================================================
// Test Cases
// ============================================================================

bool test_singleton_initialization() {
    auto& manager = DLLManager::getInstance();
    return true;
}

bool test_dependency_tracker_creation() {
    auto& manager = DLLManager::getInstance();
    auto& tracker = manager.getDependencyTracker();
    return true;
}

bool test_iat_optimizer_creation() {
    auto& manager = DLLManager::getInstance();
    auto& optimizer = manager.getIATOptimizer();
    return true;
}

bool test_hot_reloader_creation() {
    auto& manager = DLLManager::getInstance();
    auto& reloader = manager.getHotReloader();
    return true;
}

bool test_load_system_dll() {
    auto& manager = DLLManager::getInstance();
    
    // Try to load a common system DLL
    HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::None);
    
    if (hModule) {
        manager.unloadDLL(hModule, false);
        return true;
    }
    
    // If kernel32 fails (already loaded), try another
    hModule = manager.loadDLL("user32.dll", LoadFlags::None);
    if (hModule) {
        manager.unloadDLL(hModule, false);
        return true;
    }
    
    return false;
}

bool test_load_with_optimizations() {
    auto& manager = DLLManager::getInstance();
    
    auto flags = LoadFlags::OptimizedImports | LoadFlags::SearchSystem32;
    HMODULE hModule = manager.loadDLL("kernel32.dll", flags);
    
    if (hModule) {
        const auto* metrics = manager.getMetrics(hModule);
        bool optimized = metrics ? metrics->isOptimized : false;
        
        manager.unloadDLL(hModule, false);
        return optimized;
    }
    
    return false;
}

bool test_get_exports() {
    auto& manager = DLLManager::getInstance();
    
    HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::None);
    if (!hModule) {
        return false;
    }
    
    auto exports = manager.getAllExports(hModule);
    
    manager.unloadDLL(hModule, false);
    
    // kernel32 should have many exports
    return exports.size() > 100;
}

bool test_get_specific_export() {
    auto& manager = DLLManager::getInstance();
    
    HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::None);
    if (!hModule) {
        return false;
    }
    
    void* proc = manager.getExport(hModule, "LoadLibraryA");
    
    manager.unloadDLL(hModule, false);
    
    return proc != nullptr;
}

bool test_dependency_graph_building() {
    auto& manager = DLLManager::getInstance();
    auto& tracker = manager.getDependencyTracker();
    
    // Build dependency graph for a system DLL
    auto deps = tracker.buildDependencyGraph("kernel32.dll");
    
    // kernel32 should have some dependencies
    return !deps.empty();
}

bool test_circular_dependency_detection() {
    auto& manager = DLLManager::getInstance();
    auto& tracker = manager.getDependencyTracker();
    
    // kernel32 shouldn't have circular dependencies
    bool hasCircular = tracker.hasCircularDependency("kernel32.dll");
    
    return !hasCircular;
}

bool test_optimal_load_order() {
    auto& manager = DLLManager::getInstance();
    auto& tracker = manager.getDependencyTracker();
    
    std::vector<std::string> dlls = {
        "kernel32.dll",
        "user32.dll",
        "advapi32.dll"
    };
    
    auto loadOrder = tracker.getOptimalLoadOrder(dlls);
    
    // Should return all DLLs in valid order
    return loadOrder.size() == dlls.size();
}

bool test_iat_optimization_stats() {
    auto& manager = DLLManager::getInstance();
    auto& iatOpt = manager.getIATOptimizer();
    
    HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::OptimizedImports);
    if (!hModule) {
        return false;
    }
    
    auto stats = iatOpt.getStats(hModule);
    
    manager.unloadDLL(hModule, false);
    
    // Should have some entries
    return stats.totalEntries > 0;
}

bool test_hot_reload_registration() {
    auto& manager = DLLManager::getInstance();
    auto& reloader = manager.getHotReloader();
    
    uint64_t id = reloader.registerForReload("test.dll", []() {
        std::cout << "Reload callback invoked" << std::endl;
    });
    
    bool isMonitored = reloader.isMonitored("test.dll");
    
    reloader.unregister(id);
    
    return isMonitored && (id > 0);
}

bool test_performance_metrics() {
    auto& manager = DLLManager::getInstance();
    
    auto startTime = std::chrono::steady_clock::now();
    HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::OptimizedImports);
    auto endTime = std::chrono::steady_clock::now();
    
    if (!hModule) {
        return false;
    }
    
    const auto* metrics = manager.getMetrics(hModule);
    
    manager.unloadDLL(hModule, false);
    
    if (!metrics) {
        return false;
    }
    
    // Verify metrics are populated
    bool hasLoadTime = metrics->loadTimeUs > 0;
    bool hasImportCount = metrics->importCount > 0;
    bool isOptimized = metrics->isOptimized;
    
    return hasLoadTime && hasImportCount && isOptimized;
}

bool test_preload_multiple_dlls() {
    auto& manager = DLLManager::getInstance();
    
    std::vector<std::string> dlls = {
        "kernel32.dll",
        "user32.dll"
    };
    
    size_t loaded = manager.preloadDLLs(dlls, LoadFlags::PreloadDependencies);
    
    // Clean up
    manager.unloadAll(true);
    
    // Should load at least one successfully
    return loaded >= 1;
}

bool test_search_path_management() {
    auto& manager = DLLManager::getInstance();
    
    manager.addSearchPath("C:\\TestPath");
    manager.addSearchPath("D:\\AnotherPath");
    
    // Just verify no crash - actual path testing requires real DLLs
    return true;
}

bool test_module_enumeration() {
    auto& manager = DLLManager::getInstance();
    
    // Load a few modules
    manager.loadDLL("kernel32.dll", LoadFlags::None);
    manager.loadDLL("user32.dll", LoadFlags::None);
    
    auto modules = manager.getLoadedModules();
    
    // Clean up
    manager.unloadAll(true);
    
    // Should have at least the modules we loaded
    return modules.size() >= 2;
}

bool test_utils_file_size() {
    namespace utils = gametools::dll_opt::utils;
    
    // Test with a known system file
    size_t size = utils::getFileSize("C:\\Windows\\System32\\kernel32.dll");
    
    // kernel32.dll should be > 0 bytes
    return size > 0;
}

bool test_utils_is64bit() {
    namespace utils = gametools::dll_opt::utils;
    
    // Modern Windows should have 64-bit kernel32
    bool is64 = utils::is64Bit("C:\\Windows\\System32\\kernel32.dll");
    
    return is64;
}

bool test_utils_checksum() {
    namespace utils = gametools::dll_opt::utils;
    
    uint32_t checksum = utils::calculateChecksum("C:\\Windows\\System32\\kernel32.dll");
    
    // Should produce a non-zero checksum
    return checksum != 0;
}

bool test_utils_compilation_timestamp() {
    namespace utils = gametools::dll_opt::utils;
    
    uint32_t timestamp = utils::getCompilationTimestamp("C:\\Windows\\System32\\kernel32.dll");
    
    // Should have a valid timestamp (> year 2000)
    return timestamp > 946684800; // Unix timestamp for 2000-01-01
}

bool test_utils_current_process_dlls() {
    namespace utils = gametools::dll_opt::utils;
    
    auto dlls = utils::getCurrentProcessDLLs();
    
    // Current process should have at least a few DLLs loaded
    return dlls.size() >= 5;
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

void benchmark_dll_loading() {
    std::cout << "\n--- DLL Loading Benchmark ---" << std::endl;
    
    auto& manager = DLLManager::getInstance();
    
    const int iterations = 100;
    std::vector<uint64_t> times;
    
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::None);
        auto end = std::chrono::high_resolution_clock::now();
        
        if (hModule) {
            manager.unloadDLL(hModule, false);
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        times.push_back(duration.count());
    }
    
    uint64_t total = 0;
    for (auto t : times) {
        total += t;
    }
    
    double avg = static_cast<double>(total) / times.size();
    std::cout << "Average load time: " << avg << " µs" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
}

void benchmark_iat_optimization() {
    std::cout << "\n--- IAT Optimization Benchmark ---" << std::endl;
    
    auto& manager = DLLManager::getInstance();
    auto& iatOpt = manager.getIATOptimizer();
    
    const int iterations = 50;
    std::vector<size_t> optimizedCounts;
    
    for (int i = 0; i < iterations; ++i) {
        HMODULE hModule = manager.loadDLL("kernel32.dll", LoadFlags::None);
        if (hModule) {
            size_t count = iatOpt.optimizeIAT(hModule);
            optimizedCounts.push_back(count);
            manager.unloadDLL(hModule, false);
        }
    }
    
    if (!optimizedCounts.empty()) {
        size_t total = 0;
        for (auto c : optimizedCounts) {
            total += c;
        }
        
        double avg = static_cast<double>(total) / optimizedCounts.size();
        std::cout << "Average optimized imports: " << avg << std::endl;
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "DLL Optimization Module Tests\n";
    std::cout << "========================================\n\n";
    
    // Run all tests
    TEST("Singleton Initialization", test_singleton_initialization);
    TEST("Dependency Tracker Creation", test_dependency_tracker_creation);
    TEST("IAT Optimizer Creation", test_iat_optimizer_creation);
    TEST("Hot Reloader Creation", test_hot_reloader_creation);
    TEST("Load System DLL", test_load_system_dll);
    TEST("Load with Optimizations", test_load_with_optimizations);
    TEST("Get Exports", test_get_exports);
    TEST("Get Specific Export", test_get_specific_export);
    TEST("Dependency Graph Building", test_dependency_graph_building);
    TEST("Circular Dependency Detection", test_circular_dependency_detection);
    TEST("Optimal Load Order", test_optimal_load_order);
    TEST("IAT Optimization Stats", test_iat_optimization_stats);
    TEST("Hot Reload Registration", test_hot_reload_registration);
    TEST("Performance Metrics", test_performance_metrics);
    TEST("Preload Multiple DLLs", test_preload_multiple_dlls);
    TEST("Search Path Management", test_search_path_management);
    TEST("Module Enumeration", test_module_enumeration);
    TEST("Utils: File Size", test_utils_file_size);
    TEST("Utils: Is 64-bit", test_utils_is64bit);
    TEST("Utils: Checksum", test_utils_checksum);
    TEST("Utils: Compilation Timestamp", test_utils_compilation_timestamp);
    TEST("Utils: Current Process DLLs", test_utils_current_process_dlls);
    
    // Print summary
    g_results.printSummary();
    
    // Run benchmarks
    std::cout << "\nRunning benchmarks...\n";
    benchmark_dll_loading();
    benchmark_iat_optimization();
    
    return g_results.failed > 0 ? 1 : 0;
}
