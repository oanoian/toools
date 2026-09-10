// include/vcs/version_control.h
#pragma once
#include "core/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>

namespace game_tools {

// File lock states
enum class LockState : u32 {
    Unlocked,
    LockedLocal,
    LockedRemote,
    LockPending
};

// Version control system types
enum class VCSType : u32 {
    Perforce,      // Centralized with exclusive locking
    GitLFS,        // Distributed with LFS pointers
    PlasticSCM,    // Hybrid
    Custom
};

// File revision information
struct FileRevision {
    u32 revision_number;
    std::string changelist_id;
    std::string author;
    std::string description;
    u64 timestamp;
    u64 file_size;
    std::string hash; // SHA-1 or MD5 for binary verification
};

// Pending change information
struct PendingChange {
    std::string file_path;
    enum class Action { Add, Edit, Delete, Move, Integrate } action;
    LockState lock_state;
    bool is_binary;
    u64 local_size;
    u64 depot_size;
};

// Changelist for grouping changes
struct Changelist {
    std::string id;
    std::string description;
    std::string author;
    u64 timestamp;
    std::vector<std::string> files;
    bool is_submitted;
};

// Base Version Control Interface
class IVersionControl {
public:
    virtual ~IVersionControl() = default;
    
    virtual bool connect(const std::string& server_url, const std::string& workspace) = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // File operations
    virtual Result<bool> checkout_file(const std::string& file_path) = 0;
    virtual Result<bool> checkin_file(const std::string& file_path, const std::string& description) = 0;
    virtual Result<bool> revert_file(const std::string& file_path) = 0;
    virtual Result<bool> add_file(const std::string& file_path) = 0;
    virtual Result<bool> delete_file(const std::string& file_path) = 0;
    
    // Lock management (critical for binary assets)
    virtual Result<bool> lock_file(const std::string& file_path) = 0;
    virtual Result<bool> unlock_file(const std::string& file_path, bool force = false) = 0;
    virtual LockState get_lock_state(const std::string& file_path) const = 0;
    
    // Sync operations
    virtual Result<bool> sync_file(const std::string& file_path, u32 revision = 0) = 0;
    virtual Result<bool> sync_workspace() = 0;
    
    // History and revisions
    virtual std::vector<FileRevision> get_file_history(const std::string& file_path) = 0;
    virtual Result<bool> get_file_at_revision(
        const std::string& file_path,
        u32 revision,
        const std::string& output_path
    ) = 0;
    
    // Changelists
    virtual Result<std::string> create_changelist(const std::string& description) = 0;
    virtual Result<bool> submit_changelist(const std::string& changelist_id) = 0;
    virtual std::vector<PendingChange> get_pending_changes() = 0;
    
    // Binary merge prevention
    virtual bool is_mergeable(const std::string& file_path) const = 0;
    virtual void set_exclusive_lock_required(const std::string& file_pattern, bool required) = 0;
};

// Perforce Helix Core Implementation
class PerforceClient : public IVersionControl {
public:
    PerforceClient() : connected_(false) {}
    
    bool connect(const std::string& server_url, const std::string& workspace) override {
        server_url_ = server_url;
        workspace_ = workspace;
        
        // Initialize Perforce connection (p4api integration would go here)
        connected_ = true;
        return true;
    }
    
    void disconnect() override {
        connected_ = false;
    }
    
    bool is_connected() const override { return connected_; }
    
    Result<bool> checkout_file(const std::string& file_path) override {
        if (!connected_) {
            return Result<bool>::Err("Not connected to Perforce server");
        }
        
        // Exclusive lock for binary files
        if (!is_mergeable(file_path)) {
            auto lock_result = lock_file(file_path);
            if (!lock_result.success) {
                return lock_result;
            }
        }
        
        // Mark file as open for edit
        PendingChange change;
        change.file_path = file_path;
        change.action = PendingChange::Action::Edit;
        change.lock_state = LockState::LockedLocal;
        change.is_binary = !is_mergeable(file_path);
        
        pending_changes_[file_path] = change;
        return Result<bool>::Ok(true);
    }
    
    Result<bool> checkin_file(const std::string& file_path, const std::string& description) override {
        auto it = pending_changes_.find(file_path);
        if (it == pending_changes_.end()) {
            return Result<bool>::Err("File not checked out");
        }
        
        // Submit to server
        // In real implementation: p4 submit
        
        // Unlock file
        unlock_file(file_path);
        
        // Remove from pending changes
        pending_changes_.erase(it);
        
        return Result<bool>::Ok(true);
    }
    
    Result<bool> revert_file(const std::string& file_path) override {
        auto it = pending_changes_.find(file_path);
        if (it == pending_changes_.end()) {
            return Result<bool>::Err("File not checked out");
        }
        
        // Revert in Perforce
        // In real implementation: p4 revert
        
        // Unlock if locked
        unlock_file(file_path);
        
        pending_changes_.erase(it);
        return Result<bool>::Ok(true);
    }
    
    Result<bool> add_file(const std::string& file_path) override {
        PendingChange change;
        change.file_path = file_path;
        change.action = PendingChange::Action::Add;
        change.is_binary = !is_mergeable(file_path);
        
        pending_changes_[file_path] = change;
        return Result<bool>::Ok(true);
    }
    
    Result<bool> delete_file(const std::string& file_path) override {
        auto it = pending_changes_.find(file_path);
        if (it == pending_changes_.end()) {
            // Need to checkout first
            auto result = checkout_file(file_path);
            if (!result.success) {
                return result;
            }
        }
        
        pending_changes_[file_path].action = PendingChange::Action::Delete;
        return Result<bool>::Ok(true);
    }
    
    Result<bool> lock_file(const std::string& file_path) override {
        if (!connected_) {
            return Result<bool>::Err("Not connected to server");
        }
        
        // Request exclusive lock from server
        // In real implementation: p4 lock
        
        locks_[file_path] = LockState::LockedRemote;
        return Result<bool>::Ok(true);
    }
    
    Result<bool> unlock_file(const std::string& file_path, bool force = false) override {
        auto it = locks_.find(file_path);
        if (it == locks_.end()) {
            return Result<bool>::Err("File not locked");
        }
        
        if (it->second == LockState::LockedRemote && !force) {
            // Need server confirmation
            // In real implementation: p4 unlock
        }
        
        locks_.erase(it);
        return Result<bool>::Ok(true);
    }
    
    LockState get_lock_state(const std::string& file_path) const override {
        auto it = locks_.find(file_path);
        if (it != locks_.end()) {
            return it->second;
        }
        
        // Check if someone else has locked it
        // This would query the server in real implementation
        return LockState::Unlocked;
    }
    
    Result<bool> sync_file(const std::string& file_path, u32 revision = 0) override {
        // Sync specific file to revision
        // In real implementation: p4 sync file_path#revision
        return Result<bool>::Ok(true);
    }
    
    Result<bool> sync_workspace() override {
        // Full workspace sync
        // In real implementation: p4 sync ...
        return Result<bool>::Ok(true);
    }
    
    std::vector<FileRevision> get_file_history(const std::string& file_path) override {
        std::vector<FileRevision> history;
        
        // Query file history from Perforce
        // In real implementation: p4 filelog file_path
        
        return history;
    }
    
    Result<bool> get_file_at_revision(
        const std::string& file_path,
        u32 revision,
        const std::string& output_path
    ) override {
        // Retrieve specific revision
        // In real implementation: p4 print -o output_path file_path#revision
        return Result<bool>::Ok(true);
    }
    
    Result<std::string> create_changelist(const std::string& description) override {
        std::string id = "CL_" + std::to_string(next_changelist_id_++);
        
        Changelist cl;
        cl.id = id;
        cl.description = description;
        cl.is_submitted = false;
        
        changelists_[id] = cl;
        return Result<std::string>::Ok(id);
    }
    
    Result<bool> submit_changelist(const std::string& changelist_id) override {
        auto it = changelists_.find(changelist_id);
        if (it == changelists_.end()) {
            return Result<bool>::Err("Changelist not found");
        }
        
        // Submit changelist to server
        // In real implementation: p4 submit -c changelist_id
        
        it->second.is_submitted = true;
        return Result<bool>::Ok(true);
    }
    
    std::vector<PendingChange> get_pending_changes() override {
        std::vector<PendingChange> changes;
        for (const auto& [path, change] : pending_changes_) {
            changes.push_back(change);
        }
        return changes;
    }
    
    bool is_mergeable(const std::string& file_path) const override {
        // Text-based files are mergeable
        static const std::vector<std::string> mergeable_extensions = {
            ".cpp", ".h", ".hpp", ".c", ".cs", ".java",
            ".txt", ".md", ".json", ".xml", ".yaml"
        };
        
        for (const auto& ext : mergeable_extensions) {
            if (file_path.size() >= ext.size() &&
                file_path.compare(file_path.size() - ext.size(), ext.size(), ext) == 0) {
                return true;
            }
        }
        
        // Check explicit patterns
        for (const auto& [pattern, required] : exclusive_lock_patterns_) {
            if (!required && file_path.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        // Default: binary files are not mergeable
        return false;
    }
    
    void set_exclusive_lock_required(const std::string& file_pattern, bool required) override {
        exclusive_lock_patterns_[file_pattern] = required;
    }
    
private:
    std::string server_url_;
    std::string workspace_;
    std::atomic<bool> connected_;
    
    std::unordered_map<std::string, PendingChange> pending_changes_;
    std::unordered_map<std::string, LockState> locks_;
    std::unordered_map<std::string, Changelist> changelists_;
    std::unordered_map<std::string, bool> exclusive_lock_patterns_;
    
    std::atomic<u32> next_changelist_id_ = 1000;
};

// Git LFS Implementation (simplified)
class GitLFSClient : public IVersionControl {
public:
    bool connect(const std::string& server_url, const std::string& workspace) override {
        remote_url_ = server_url;
        workspace_ = workspace;
        connected_ = true;
        return true;
    }
    
    void disconnect() override { connected_ = false; }
    bool is_connected() const override { return connected_; }
    
    Result<bool> checkout_file(const std::string& file_path) override {
        // Git doesn't support true exclusive locking
        // LFS locking is advisory, not enforced
        return Result<bool>::Ok(true);
    }
    
    Result<bool> checkin_file(const std::string& file_path, const std::string& description) override {
        // Commit and push
        return Result<bool>::Ok(true);
    }
    
    Result<bool> revert_file(const std::string& file_path) override {
        return Result<bool>::Ok(true);
    }
    
    Result<bool> add_file(const std::string& file_path) override {
        return Result<bool>::Ok(true);
    }
    
    Result<bool> delete_file(const std::string& file_path) override {
        return Result<bool>::Ok(true);
    }
    
    Result<bool> lock_file(const std::string& file_path) override {
        // Git LFS advisory lock (not server-enforced)
        locks_[file_path] = LockState::LockedLocal;
        return Result<bool>::Ok(true);
    }
    
    Result<bool> unlock_file(const std::string& file_path, bool force = false) override {
        locks_.erase(file_path);
        return Result<bool>::Ok(true);
    }
    
    LockState get_lock_state(const std::string& file_path) const override {
        auto it = locks_.find(file_path);
        return it != locks_.end() ? it->second : LockState::Unlocked;
    }
    
    Result<bool> sync_file(const std::string& file_path, u32 revision = 0) override {
        return Result<bool>::Ok(true);
    }
    
    Result<bool> sync_workspace() override {
        // git pull
        return Result<bool>::Ok(true);
    }
    
    std::vector<FileRevision> get_file_history(const std::string& file_path) override {
        return {};
    }
    
    Result<bool> get_file_at_revision(
        const std::string& file_path,
        u32 revision,
        const std::string& output_path
    ) override {
        return Result<bool>::Ok(true);
    }
    
    Result<std::string> create_changelist(const std::string& description) override {
        return Result<std::string>::Err("Git uses commits, not changelists");
    }
    
    Result<bool> submit_changelist(const std::string& changelist_id) override {
        return Result<bool>::Err("Git uses commits, not changelists");
    }
    
    std::vector<PendingChange> get_pending_changes() override {
        return {};
    }
    
    bool is_mergeable(const std::string& file_path) const override {
        return true; // Git assumes all files are mergeable
    }
    
    void set_exclusive_lock_required(const std::string& file_pattern, bool required) override {
        // Git LFS doesn't enforce exclusive locks
    }
    
private:
    std::string remote_url_;
    std::string workspace_;
    bool connected_ = false;
    std::unordered_map<std::string, LockState> locks_;
};

// VCS Manager - Factory and unified interface
class VCSManager {
public:
    static VCSManager& instance() {
        static VCSManager manager;
        return manager;
    }
    
    bool initialize(VCSType type, const std::string& server_url, const std::string& workspace) {
        switch (type) {
            case VCSType::Perforce:
                vcs_ = std::make_unique<PerforceClient>();
                break;
            case VCSType::GitLFS:
                vcs_ = std::make_unique<GitLFSClient>();
                break;
            default:
                return false;
        }
        
        return vcs_->connect(server_url, workspace);
    }
    
    IVersionControl* get_vcs() { return vcs_.get(); }
    
    // Convenience methods
    Result<bool> checkout(const std::string& file) {
        return vcs_ ? vcs_->checkout_file(file) : Result<bool>::Err("VCS not initialized");
    }
    
    Result<bool> checkin(const std::string& file, const std::string& desc) {
        return vcs_ ? vcs_->checkin_file(file, desc) : Result<bool>::Err("VCS not initialized");
    }
    
    bool is_binary_asset(const std::string& file_path) const {
        return vcs_ ? !vcs_->is_mergeable(file_path) : false;
    }
    
private:
    VCSManager() = default;
    std::unique_ptr<IVersionControl> vcs_;
};

} // namespace game_tools
