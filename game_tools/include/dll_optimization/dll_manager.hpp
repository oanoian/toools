#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

namespace gametools {
namespace dll_opt {

// Forward declarations
class DLLModule;
class DependencyTracker;
class HotReloader;
class ImportAddressTableHook;

/**
 * @brief Enum for DLL loading flags with extended options
 */
enum class LoadFlags : uint32_t {
    None = 0,
    LazyBinding = 1 << 0,           // Delay import binding
    NoSerialDlls = 1 << 1,          // Don't load serially
    SearchAppDir = 1 << 2,          // Search application directory
    SearchSystem32 = 1 << 3,        // Search system directory
    SafeCurrentDir = 1 << 4,        // Safe current directory search
    ArchitectureSpecific = 1 << 5,  // Load architecture-specific version
    OptimizedImports = 1 << 6,      // Apply IAT optimization
    PreloadDependencies = 1 << 7,   // Preload all dependencies
    EnableHotReload = 1 << 8        // Enable hot reloading capability
};

inline LoadFlags operator|(LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline LoadFlags operator&(LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * @brief Structure holding DLL export information
 */
struct ExportInfo {
    std::string name;
    uint32_t ordinal;
    void* address;
    bool isForwarded;
    std::string forwardName;
};

/**
 * @brief Structure holding DLL import information
 */
struct ImportInfo {
    std::string moduleName;
    std::string functionName;
    uint32_t functionOrdinal;
    void** thunkAddress;
    void* resolvedAddress;
};

/**
 * @brief Performance metrics for DLL operations
 */
struct DLLPerformanceMetrics {
    std::atomic<uint64_t> loadTimeUs{0};      // Load time in microseconds
    std::atomic<uint64_t> bindTimeUs{0};      // Import binding time
    std::atomic<uint64_t> initTimeUs{0};      // DllMain initialization time
    std::atomic<size_t> memoryFootprint{0};   // Memory usage in bytes
    std::atomic<size_t> importCount{0};       // Number of imports
    std::atomic<size_t> exportCount{0};       // Number of exports
    std::atomic<size_t> dependencyCount{0};   // Number of dependencies
    std::atomic<bool> isOptimized{false};     // Whether optimizations applied
};

/**
 * @brief Dependency tracker for managing DLL dependency graphs
 */
class DependencyTracker {
public:
    DependencyTracker();
    ~DependencyTracker();

    /**
     * @brief Build dependency graph for a DLL
     * @param dllPath Path to the DLL file
     * @return Vector of dependency paths in load order
     */
    std::vector<std::string> buildDependencyGraph(const std::string& dllPath);

    /**
     * @brief Check for circular dependencies
     * @param dllPath Path to check
     * @return True if circular dependency detected
     */
    bool hasCircularDependency(const std::string& dllPath);

    /**
     * @brief Get optimal load order using topological sort
     * @param dlls Vector of DLL paths
     * @return Ordered vector of DLL paths
     */
    std::vector<std::string> getOptimalLoadOrder(const std::vector<std::string>& dlls);

    /**
     * @brief Clear dependency cache
     */
    void clearCache();

private:
    struct DependencyNode {
        std::string path;
        std::vector<std::string> dependencies;
        bool visited = false;
        bool visiting = false; // For cycle detection
    };

    std::unordered_map<std::string, DependencyNode> dependencyGraph_;
    std::mutex graphMutex_;

    bool buildNode(const std::string& dllPath, DependencyNode& node);
    bool detectCycle(const std::string& node, std::vector<std::string>& path);
    void topologicalSort(const std::string& node, 
                         std::vector<std::string>& result,
                         std::unordered_map<std::string, bool>& visited);
    
    std::string findDLLInSearchPaths(const std::string& dllName);
};

/**
 * @brief Import Address Table (IAT) optimizer
 */
class ImportAddressTableOptimizer {
public:
    ImportAddressTableOptimizer();
    ~ImportAddressTableOptimizer();

    /**
     * @brief Optimize IAT for faster resolution
     * @param module Handle to loaded module
     * @return Number of optimized entries
     */
    size_t optimizeIAT(HMODULE module);

    /**
     * @brief Prefetch import addresses to reduce page faults
     * @param module Handle to loaded module
     * @return Number of prefetched entries
     */
    size_t prefetchImports(HMODULE module);

    /**
     * @brief Create local copy of frequently called imports
     * @param module Handle to loaded module
     * @param threshold Minimum call count to optimize
     * @return Number of cached imports
     */
    size_t cacheFrequentImports(HMODULE module, size_t threshold = 100);

    /**
     * @brief Align IAT entries for better cache locality
     * @param module Handle to loaded module
     * @return Success status
     */
    bool alignIATEntries(HMODULE module);

    /**
     * @brief Get IAT optimization statistics
     */
    struct IATStats {
        size_t totalEntries;
        size_t optimizedEntries;
        size_t cachedEntries;
        size_t pageFaultsReduced;
        double averageLookupTimeNs;
    };

    IATStats getStats(HMODULE module) const;

private:
    struct ImportCache {
        void* originalAddress;
        void* cachedAddress;
        size_t callCount;
        uint64_t lastAccessTime;
    };

    std::unordered_map<HMODULE, std::vector<ImportInfo>> importTables_;
    std::unordered_map<HMODULE, std::vector<ImportCache>> importCaches_;
    mutable std::mutex cacheMutex_;

    bool parsePEHeaders(HMODULE module, void*& baseAddress, size_t& imageSize);
    bool walkImportTable(void* baseAddress, std::vector<ImportInfo>& imports);
    void* resolveImportFast(const std::string& moduleName, const std::string& funcName);
};

/**
 * @brief Hot reloader for DLLs during development
 */
class HotReloader {
public:
    HotReloader();
    ~HotReloader();

    /**
     * @brief Register a DLL for hot reloading
     * @param dllPath Path to the DLL
     * @param reloadCallback Callback invoked on reload
     * @return Registration ID
     */
    uint64_t registerForReload(const std::string& dllPath,
                               std::function<void()> reloadCallback);

    /**
     * @brief Unregister from hot reloading
     * @param registrationId ID from registerForReload
     */
    void unregister(uint64_t registrationId);

    /**
     * @brief Manually trigger reload
     * @param registrationId ID to reload
     * @return Success status
     */
    bool triggerReload(uint64_t registrationId);

    /**
     * @brief Start automatic file monitoring
     * @param intervalMs Check interval in milliseconds
     */
    void startMonitoring(uint32_t intervalMs = 1000);

    /**
     * @brief Stop file monitoring
     */
    void stopMonitoring();

    /**
     * @brief Check if a DLL is being monitored
     */
    bool isMonitored(const std::string& dllPath) const;

    /**
     * @brief Get reload history
     */
    struct ReloadRecord {
        uint64_t timestamp;
        std::string dllPath;
        bool success;
        std::string errorMessage;
    };

    std::vector<ReloadRecord> getReloadHistory(uint64_t registrationId) const;

private:
    struct MonitoredDLL {
        uint64_t id;
        std::string path;
        std::string backupPath;
        HMODULE currentHandle;
        std::function<void()> reloadCallback;
        uint64_t lastModifiedTime;
        std::vector<ReloadRecord> history;
        bool isActive;
    };

    std::unordered_map<uint64_t, MonitoredDLL> monitoredDLLs_;
    std::atomic<uint64_t> nextId_{1};
    std::atomic<bool> monitoringActive_{false};
    mutable std::mutex monitorMutex_;

    void monitoringThread();
    bool performReload(MonitoredDLL& dll);
    uint64_t getFileModifiedTime(const std::string& path);
    std::string createBackupPath(const std::string& originalPath);
};

/**
 * @brief Main DLL Manager with optimization capabilities
 */
class DLLManager {
public:
    static DLLManager& getInstance();

    /**
     * @brief Load a DLL with optimization flags
     * @param dllPath Path to DLL file
     * @param flags Optimization and loading flags
     * @return Module handle or nullptr on failure
     */
    HMODULE loadDLL(const std::string& dllPath, LoadFlags flags = LoadFlags::None);

    /**
     * @brief Unload a DLL safely
     * @param module Module handle
     * @param force Force unload even if reference count > 0
     * @return Success status
     */
    bool unloadDLL(HMODULE module, bool force = false);

    /**
     * @brief Get export by name
     * @param module Module handle
     * @param exportName Name of export
     * @return Function pointer or nullptr
     */
    void* getExport(HMODULE module, const std::string& exportName);

    /**
     * @brief Get export by ordinal
     * @param module Module handle
     * @param ordinal Ordinal number
     * @return Function pointer or nullptr
     */
    void* getExport(HMODULE module, uint32_t ordinal);

    /**
     * @brief Get all exports from a module
     * @param module Module handle
     * @return Vector of export information
     */
    std::vector<ExportInfo> getAllExports(HMODULE module);

    /**
     * @brief Get all imports for a module
     * @param module Module handle
     * @return Vector of import information
     */
    std::vector<ImportInfo> getAllImports(HMODULE module);

    /**
     * @brief Get performance metrics for a loaded DLL
     */
    const DLLPerformanceMetrics* getMetrics(HMODULE module) const;

    /**
     * @brief Enable vendor-specific optimizations (NVIDIA/AMD)
     * @param module Module handle
     * @param vendor Vendor identifier ("nvidia", "amd", "intel")
     * @return Success status
     */
    bool applyVendorOptimizations(HMODULE module, const std::string& vendor);

    /**
     * @brief Preload and optimize multiple DLLs
     * @param dllPaths Vector of DLL paths
     * @param flags Loading flags
     * @return Number of successfully loaded DLLs
     */
    size_t preloadDLLs(const std::vector<std::string>& dllPaths, 
                       LoadFlags flags = LoadFlags::PreloadDependencies);

    /**
     * @brief Get dependency tracker instance
     */
    DependencyTracker& getDependencyTracker();

    /**
     * @brief Get IAT optimizer instance
     */
    ImportAddressTableOptimizer& getIATOptimizer();

    /**
     * @brief Get hot reloader instance
     */
    HotReloader& getHotReloader();

    /**
     * @brief Set custom DLL search path
     * @param path Directory to add to search path
     */
    void addSearchPath(const std::string& path);

    /**
     * @brief Clear all loaded DLLs
     * @param force Force unload all
     */
    void unloadAll(bool force = false);

    /**
     * @brief Get list of currently loaded modules
     */
    std::vector<HMODULE> getLoadedModules() const;

private:
    DLLManager();
    ~DLLManager();
    DLLManager(const DLLManager&) = delete;
    DLLManager& operator=(const DLLManager&) = delete;

    struct LoadedModule {
        HMODULE handle;
        std::string path;
        LoadFlags flags;
        DLLPerformanceMetrics metrics;
        std::vector<std::string> dependencies;
        std::chrono::steady_clock::time_point loadTime;
        size_t referenceCount;
        bool isOptimized;
    };

    std::unordered_map<HMODULE, LoadedModule> loadedModules_;
    std::vector<std::string> searchPaths_;
    DependencyTracker dependencyTracker_;
    ImportAddressTableOptimizer iatOptimizer_;
    HotReloader hotReloader_;
    mutable std::mutex managerMutex_;

    HMODULE loadWithFlags(const std::string& dllPath, LoadFlags flags);
    void applyOptimizations(LoadedModule& module);
    void measureLoadTime(LoadedModule& module, 
                         std::chrono::steady_clock::time_point startTime);
    bool validatePEHeader(const std::string& dllPath);
    size_t calculateMemoryFootprint(HMODULE module);
};

// Utility functions
namespace utils {

/**
 * @brief Get DLL file size
 */
size_t getFileSize(const std::string& path);

/**
 * @brief Calculate CRC32 checksum of DLL
 */
uint32_t calculateChecksum(const std::string& path);

/**
 * @brief Check if DLL is 32-bit or 64-bit
 */
bool is64Bit(const std::string& path);

/**
 * @brief Get DLL compilation timestamp
 */
uint32_t getCompilationTimestamp(const std::string& path);

/**
 * @brief Verify DLL signature (Authenticode)
 */
bool verifySignature(const std::string& path);

/**
 * @brief Get list of DLLs loaded in current process
 */
std::vector<std::string> getCurrentProcessDLLs();

/**
 * @brief Inject DLL into another process (advanced)
 */
bool injectIntoProcess(DWORD processId, const std::string& dllPath);

} // namespace utils

} // namespace dll_opt
} // namespace gametools
