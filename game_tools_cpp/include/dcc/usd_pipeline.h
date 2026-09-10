// include/dcc/usd_pipeline.h
#pragma once
#include "core/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace game_tools {

// USD Primitive types
enum class UsdPrimType : u32 {
    Xform,
    Mesh,
    Material,
    Light,
    Camera,
    Skeleton,
    Animation,
    Scope,
    Unknown
};

// USD Layer for non-destructive editing
class UsdLayer {
public:
    explicit UsdLayer(const std::string& identifier) 
        : identifier_(identifier), is_anonymous_(false) {}
    
    static UsdLayer create_anonymous() {
        UsdLayer layer("anonymous");
        layer.is_anonymous_ = true;
        return layer;
    }
    
    const std::string& get_identifier() const { return identifier_; }
    bool is_anonymous() const { return is_anonymous_; }
    
    // Layer composition operations
    void add_sub_layer(const std::string& sub_layer_path) {
        sub_layers_.push_back(sub_layer_path);
    }
    
    void set_offset(f64 offset) { offset_ = offset; }
    void set_scale(f64 scale) { scale_ = scale; }
    
private:
    std::string identifier_;
    bool is_anonymous_;
    std::vector<std::string> sub_layers_;
    f64 offset_ = 0.0;
    f64 scale_ = 1.0;
};

// USD Attribute with value and animation support
template<typename T>
class UsdAttribute {
public:
    UsdAttribute(const std::string& name) : name_(name) {}
    
    void set_value(const T& value, f64 time = 0.0) {
        if (time == 0.0) {
            default_value_ = value;
        } else {
            animated_values_[time] = value;
        }
    }
    
    T get_value(f64 time = 0.0) const {
        if (animated_values_.empty()) {
            return default_value_;
        }
        
        // Find keyframes before and after time
        auto it = animated_values_.lower_bound(time);
        if (it == animated_values_.begin()) {
            return it->second;
        }
        if (it == animated_values_.end()) {
            return animated_values_.rbegin()->second;
        }
        
        // Linear interpolation between keyframes
        auto it_after = it;
        auto it_before = std::prev(it);
        
        f64 t0 = it_before->first;
        f64 t1 = it_after->first;
        f64 alpha = (time - t0) / (t1 - t0);
        
        return interpolate(it_before->second, it_after->second, alpha);
    }
    
    bool has_animation() const { return !animated_values_.empty(); }
    const std::map<f64, T>& get_keyframes() const { return animated_values_; }
    
private:
    template<typename U = T>
    typename std::enable_if<std::is_arithmetic<U>::value, U>::type
    interpolate(const U& a, const U& b, f64 alpha) const {
        return static_cast<U>(a + (b - a) * alpha);
    }
    
    std::string name_;
    T default_value_{};
    std::map<f64, T> animated_values_;
};

// USD Prim (primitive) - basic building block
class UsdPrim {
public:
    UsdPrim(const std::string& path, UsdPrimType type)
        : path_(path), type_(type) {}
    
    virtual ~UsdPrim() = default;
    
    const std::string& get_path() const { return path_; }
    UsdPrimType get_type() const { return type_; }
    
    // Attributes
    template<typename T>
    void create_attribute(const std::string& name) {
        attributes_[name] = std::make_unique<UsdAttribute<T>>(name);
    }
    
    template<typename T>
    UsdAttribute<T>* get_attribute(const std::string& name) {
        auto it = attributes_.find(name);
        if (it != attributes_.end()) {
            return dynamic_cast<UsdAttribute<T>*>(it->second.get());
        }
        return nullptr;
    }
    
    // Relationships
    void add_relationship(const std::string& name, const std::string& target_path) {
        relationships_[name].push_back(target_path);
    }
    
    const std::vector<std::string>& get_relationship_targets(const std::string& name) const {
        static std::vector<std::string> empty;
        auto it = relationships_.find(name);
        return it != relationships_.end() ? it->second : empty;
    }
    
    // Children management
    void add_child(std::shared_ptr<UsdPrim> child) {
        children_.push_back(child);
    }
    
    const std::vector<std::shared_ptr<UsdPrim>>& get_children() const {
        return children_;
    }
    
protected:
    std::string path_;
    UsdPrimType type_;
    std::unordered_map<std::string, std::unique_ptr<void>> attributes_;
    std::unordered_map<std::string, std::vector<std::string>> relationships_;
    std::vector<std::shared_ptr<UsdPrim>> children_;
};

// USD Stage - container for all prims in a scene
class UsdStage {
public:
    static std::shared_ptr<UsdStage> create_new() {
        return std::shared_ptr<UsdStage>(new UsdStage());
    }
    
    static std::shared_ptr<UsdStage> open(const std::string& path) {
        auto stage = std::shared_ptr<UsdStage>(new UsdStage());
        stage->load_from_file(path);
        return stage;
    }
    
    // Root prim
    std::shared_ptr<UsdPrim> get_root_prim() const { return root_prim_; }
    
    // Prim creation
    std::shared_ptr<UsdPrim> define_prim(
        const std::string& path,
        UsdPrimType type
    ) {
        auto prim = std::make_shared<UsdPrim>(path, type);
        prim_map_[path] = prim;
        
        if (path == "/") {
            root_prim_ = prim;
        } else {
            // Find parent and add as child
            size_t last_slash = path.rfind('/');
            if (last_slash != std::string::npos && last_slash > 0) {
                std::string parent_path = path.substr(0, last_slash);
                auto parent_it = prim_map_.find(parent_path);
                if (parent_it != prim_map_.end()) {
                    parent_it->second->add_child(prim);
                }
            }
        }
        
        return prim;
    }
    
    std::shared_ptr<UsdPrim> get_prim_at_path(const std::string& path) const {
        auto it = prim_map_.find(path);
        if (it != prim_map_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Layer management
    void add_root_layer(const std::string& layer_path) {
        root_layers_.push_back(layer_path);
    }
    
    UsdLayer create_anonymous_layer() {
        return UsdLayer::create_anonymous();
    }
    
    // Export
    Result<bool> export_to_file(const std::string& path, bool as_binary = true) {
        // Implementation would write USDA (ASCII) or USDC (binary) format
        return Result<bool>::Ok(true);
    }
    
private:
    UsdStage() {
        root_prim_ = std::make_shared<UsdPrim>("/", UsdPrimType::Scope);
        prim_map_["/"] = root_prim_;
    }
    
    void load_from_file(const std::string& path) {
        // Parse USDA/USDC file and populate prims
    }
    
    std::shared_ptr<UsdPrim> root_prim_;
    std::unordered_map<std::string, std::shared_ptr<UsdPrim>> prim_map_;
    std::vector<std::string> root_layers_;
};

// Live Link - Real-time synchronization with DCC tools
class LiveLink {
public:
    using TransformCallback = std::function<void(const std::string& prim_path, const f32* transform)>;
    using AnimationCallback = std::function<void(const std::string& prim_path, const f32* joint_transforms, u32 joint_count)>;
    
    static LiveLink& instance() {
        static LiveLink link;
        return link;
    }
    
    bool connect(const std::string& host, u16 port) {
        // Establish socket connection to DCC tool
        host_ = host;
        port_ = port;
        connected_ = true;
        return true;
    }
    
    void disconnect() {
        connected_ = false;
    }
    
    bool is_connected() const { return connected_; }
    
    // Register callbacks for real-time updates
    void register_transform_callback(TransformCallback callback) {
        transform_callbacks_.push_back(callback);
    }
    
    void register_animation_callback(AnimationCallback callback) {
        animation_callbacks_.push_back(callback);
    }
    
    // Send data to engine (called from DCC plugin)
    void send_transform_update(const std::string& prim_path, const f32 transform[16]) {
        for (auto& callback : transform_callbacks_) {
            callback(prim_path, transform);
        }
    }
    
    void send_animation_update(
        const std::string& prim_path,
        const f32* joint_transforms,
        u32 joint_count
    ) {
        for (auto& callback : animation_callbacks_) {
            callback(prim_path, joint_transforms, joint_count);
        }
    }
    
    // Frame synchronization
    void set_frame_rate(f32 fps) { frame_rate_ = fps; }
    f32 get_frame_rate() const { return frame_rate_; }
    
    void request_frame(u32 frame_number) {
        current_frame_ = frame_number;
        // Request frame data from DCC
    }
    
private:
    LiveLink() : connected_(false), frame_rate_(30.0f), current_frame_(0) {}
    
    std::string host_;
    u16 port_;
    bool connected_;
    f32 frame_rate_;
    u32 current_frame_;
    std::vector<TransformCallback> transform_callbacks_;
    std::vector<AnimationCallback> animation_callbacks_;
};

// USD Schema Extension for engine-specific data
class EngineSchema {
public:
    // Custom prim definitions
    static void register_engine_prims() {
        // Register custom prim types like "GameEntity", "TriggerVolume", etc.
    }
    
    // Add game-specific attributes to standard prims
    template<typename UsdPrimType>
    static void add_game_attributes(std::shared_ptr<UsdPrimType> prim) {
        prim->template create_attribute<f32>("game:mass");
        prim->template create_attribute<f32>("game:friction");
        prim->template create_attribute<std::string>("game:collisionProfile");
        prim->template create_attribute<bool>("game:generateCollisions");
    }
};

} // namespace game_tools
