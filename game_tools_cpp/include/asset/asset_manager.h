// include/asset/asset_manager.h
#pragma once
#include "core/types.h"
#include "core/memory.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <future>
#include <atomic>

namespace game_tools {

// Asset types
enum class AssetType : u32 {
    Texture2D,
    Texture3D,
    Mesh,
    Skeleton,
    Animation,
    Shader,
    Material,
    Audio,
    Font,
    Config,
    Unknown
};

// Asset metadata
struct AssetMetadata {
    std::string path;
    std::string name;
    AssetType type;
    u64 size_bytes;
    u64 hash;
    f64 load_time_ms;
    bool is_compressed;
    bool is_streamed;
};

// Base asset interface
class IAsset {
public:
    virtual ~IAsset() = default;
    virtual AssetType get_type() const = 0;
    virtual const AssetMetadata& get_metadata() const = 0;
    virtual void unload() = 0;
    virtual size_t get_memory_footprint() const = 0;
};

// Virtual File System for platform-independent file access
class VirtualFileSystem {
public:
    struct FileHandle {
        void* data;
        size_t size;
        bool is_valid() const { return data != nullptr; }
    };
    
    static VirtualFileSystem& instance() {
        static VirtualFileSystem vfs;
        return vfs;
    }
    
    void mount(const std::string& mount_point, const std::string& physical_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        mount_points_[mount_point] = physical_path;
    }
    
    FileHandle open(const std::string& virtual_path) {
        std::string physical_path = resolve_path(virtual_path);
        
        // Platform-specific file opening (simplified)
#if defined(_WIN32)
        FILE* file = nullptr;
        fopen_s(&file, physical_path.c_str(), "rb");
#else
        FILE* file = fopen(physical_path.c_str(), "rb");
#endif
        
        if (!file) {
            return {nullptr, 0};
        }
        
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        void* data = aligned_alloc(CACHE_LINE_SIZE, size);
        fread(data, 1, size, file);
        fclose(file);
        
        return {data, size};
    }
    
    bool exists(const std::string& virtual_path) const {
        std::string physical_path = resolve_path(virtual_path);
#if defined(_WIN32)
        DWORD attrs = GetFileAttributesA(physical_path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES;
#else
        struct stat buffer;
        return (stat(physical_path.c_str(), &buffer) == 0);
#endif
    }
    
private:
    std::string resolve_path(const std::string& virtual_path) const {
        for (const auto& [mount_point, physical_path] : mount_points_) {
            if (virtual_path.find(mount_point) == 0) {
                return physical_path + virtual_path.substr(mount_point.length());
            }
        }
        return virtual_path;
    }
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> mount_points_;
};

// Async asset loading with priority queue
class AssetLoader {
public:
    enum class Priority : i32 {
        Low = 0,
        Normal = 5,
        High = 10,
        Critical = 15
    };
    
    struct LoadRequest {
        std::string path;
        AssetType type;
        Priority priority;
        std::promise<IAsset*> promise;
    };
    
    AssetLoader() : running_(true) {
        worker_thread_ = std::thread([this]() { worker_loop(); });
    }
    
    ~AssetLoader() {
        running_ = false;
        cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
    
    std::future<IAsset*> load_asset_async(
        const std::string& path,
        AssetType type,
        Priority priority = Priority::Normal
    ) {
        LoadRequest request;
        request.path = path;
        request.type = type;
        request.priority = priority;
        
        auto future = request.promise.get_future();
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            load_queue_.push_back(std::move(request));
            // Sort by priority (descending)
            std::sort(load_queue_.begin(), load_queue_.end(),
                [](const LoadRequest& a, const LoadRequest& b) {
                    return static_cast<i32>(a.priority) > static_cast<i32>(b.priority);
                });
        }
        
        cv_.notify_one();
        return future;
    }
    
    IAsset* load_asset_sync(const std::string& path, AssetType type) {
        auto future = load_asset_async(path, type, Priority::Critical);
        return future.get();
    }
    
    void cancel_load(const std::string& path) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        load_queue_.erase(
            std::remove_if(load_queue_.begin(), load_queue_.end(),
                [&path](const LoadRequest& req) { return req.path == path; }),
            load_queue_.end()
        );
    }
    
    size_t get_pending_loads() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return load_queue_.size();
    }
    
private:
    void worker_loop() {
        while (running_) {
            LoadRequest request;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this]() { 
                    return !load_queue_.empty() || !running_; 
                });
                
                if (!running_ && load_queue_.empty()) {
                    break;
                }
                
                request = std::move(load_queue_.front());
                load_queue_.pop_front();
            }
            
            // Load asset (platform-specific implementation)
            IAsset* asset = load_asset_impl(request.path, request.type);
            request.promise.set_value(asset);
        }
    }
    
    IAsset* load_asset_impl(const std::string& path, AssetType type) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto& vfs = VirtualFileSystem::instance();
        auto handle = vfs.open(path);
        
        if (!handle.is_valid()) {
            return nullptr;
        }
        
        // Create appropriate asset type (simplified)
        IAsset* asset = nullptr;
        switch (type) {
            case AssetType::Texture2D:
                // asset = new Texture2D(handle.data, handle.size);
                break;
            case AssetType::Mesh:
                // asset = new Mesh(handle.data, handle.size);
                break;
            default:
                break;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Update metadata
        if (asset) {
            AssetMetadata& meta = const_cast<AssetMetadata&>(asset->get_metadata());
            meta.load_time_ms = duration.count() / 1000.0;
            meta.size_bytes = handle.size;
        }
        
        return asset;
    }
    
    std::atomic<bool> running_;
    std::thread worker_thread_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::deque<LoadRequest> load_queue_;
};

// Main Asset Manager with caching and streaming
class AssetManager {
public:
    static AssetManager& instance() {
        static AssetManager manager;
        return manager;
    }
    
    template<typename T>
    T* load_asset(const std::string& path) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        // Check cache first
        auto it = asset_cache_.find(path);
        if (it != asset_cache_.end()) {
            it->second.reference_count++;
            return static_cast<T*>(it->second.asset);
        }
        
        // Load from disk
        AssetType type = deduce_asset_type<T>();
        IAsset* asset = loader_.load_asset_sync(path, type);
        
        if (asset) {
            AssetCacheEntry entry;
            entry.asset = asset;
            entry.reference_count = 1;
            entry.last_access = std::chrono::steady_clock::now();
            asset_cache_[path] = std::move(entry);
            total_memory_usage_ += asset->get_memory_footprint();
        }
        
        return static_cast<T*>(asset);
    }
    
    template<typename T>
    std::future<T*> load_asset_async(const std::string& path) {
        AssetType type = deduce_asset_type<T>();
        auto future = loader_.load_asset_async(path, type);
        
        // Wrap future to cast to specific type
        auto wrapped = std::async(std::launch::deferred, 
            [future = std::move(future)]() mutable -> T* {
                return static_cast<T*>(future.get());
            });
        
        return wrapped;
    }
    
    void unload_asset(const std::string& path) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto it = asset_cache_.find(path);
        if (it != asset_cache_.end()) {
            it->second.reference_count--;
            if (it->second.reference_count == 0) {
                total_memory_usage_ -= it->second.asset->get_memory_footprint();
                it->second.asset->unload();
                asset_cache_.erase(it);
            }
        }
    }
    
    void unload_unused_assets() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto now = std::chrono::steady_clock::now();
        auto it = asset_cache_.begin();
        
        while (it != asset_cache_.end()) {
            if (it->second.reference_count == 0) {
                auto age = std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->second.last_access).count();
                
                if (age > 30) { // Unload if unused for 30 seconds
                    total_memory_usage_ -= it->second.asset->get_memory_footprint();
                    it->second.asset->unload();
                    it = asset_cache_.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
    
    void clear_cache() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        for (auto& [path, entry] : asset_cache_) {
            total_memory_usage_ -= entry.asset->get_memory_footprint();
            entry.asset->unload();
        }
        
        asset_cache_.clear();
        total_memory_usage_ = 0;
    }
    
    size_t get_total_memory_usage() const { return total_memory_usage_; }
    size_t get_cached_asset_count() const { return asset_cache_.size(); }
    
    void set_memory_limit(size_t limit_bytes) { memory_limit_ = limit_bytes; }
    
private:
    AssetManager() : total_memory_usage_(0), memory_limit_(1024 * 1024 * 1024) {} // 1GB default
    
    struct AssetCacheEntry {
        IAsset* asset;
        u32 reference_count;
        std::chrono::steady_clock::time_point last_access;
    };
    
    template<typename T>
    AssetType deduce_asset_type() {
        // Compile-time type to AssetType mapping
        if constexpr (std::is_same_v<T, class Texture2D>) {
            return AssetType::Texture2D;
        } else if constexpr (std::is_same_v<T, class Mesh>) {
            return AssetType::Mesh;
        } else {
            return AssetType::Unknown;
        }
    }
    
    AssetType deduce_asset_type_from_path(const std::string& path) const {
        if (path.find(".png") != std::string::npos || path.find(".dds") != std::string::npos) {
            return AssetType::Texture2D;
        } else if (path.find(".fbx") != std::string::npos || path.find(".obj") != std::string::npos) {
            return AssetType::Mesh;
        }
        return AssetType::Unknown;
    }
    
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, AssetCacheEntry> asset_cache_;
    AssetLoader loader_;
    size_t total_memory_usage_;
    size_t memory_limit_;
};

} // namespace game_tools
