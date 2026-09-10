// include/ecs/entity.h
#pragma once
#include "core/types.h"
#include "core/memory.h"
#include <vector>
#include <bitset>
#include <type_traits>

namespace game_tools {

constexpr size_t MAX_COMPONENTS = 64;
constexpr size_t MAX_ENTITIES = 100000;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

// Component type registry for compile-time to runtime mapping
class ComponentRegistry {
public:
    template<typename T>
    static u32 get_component_id() {
        static u32 id = next_id_++;
        return id;
    }
    
    static u32 get_component_count() { return next_id_; }
    
private:
    static inline u32 next_id_ = 0;
};

// Base component interface (marker)
struct IComponent {
    virtual ~IComponent() = default;
};

// Entity class - lightweight identifier with component access
class Entity {
public:
    Entity() : id_(INVALID_ENTITY), mask_() {}
    Entity(EntityID id) : id_(id), mask_() {}
    
    EntityID get_id() const { return id_; }
    bool is_valid() const { return id_ != INVALID_ENTITY; }
    
    template<typename T>
    void add_component(ComponentMask& mask) {
        mask.set(ComponentRegistry::get_component_id<T>());
    }
    
    template<typename T>
    bool has_component(const ComponentMask& mask) const {
        return mask.test(ComponentRegistry::get_component_id<T>());
    }
    
private:
    EntityID id_;
    ComponentMask mask_;
};

// Component storage with contiguous memory layout
template<typename T>
class ComponentStorage {
public:
    ComponentStorage() = default;
    
    // Add component for entity, returns pointer to component data
    T* add_component(EntityID entity_id) {
        if (entity_id >= entities_.size()) {
            entities_.resize(entity_id + 1, INVALID_ENTITY);
            components_.resize(entity_id + 1);
        }
        
        entities_[entity_id] = entity_id;
        component_map_[entity_id] = components_.size();
        components_.emplace_back();
        return &components_.back();
    }
    
    T* get_component(EntityID entity_id) {
        auto it = component_map_.find(entity_id);
        if (it != component_map_.end()) {
            return &components_[it->second];
        }
        return nullptr;
    }
    
    const T* get_component(EntityID entity_id) const {
        auto it = component_map_.find(entity_id);
        if (it != component_map_.end()) {
            return &components_[it->second];
        }
        return nullptr;
    }
    
    bool has_component(EntityID entity_id) const {
        return component_map_.find(entity_id) != component_map_.end();
    }
    
    void remove_component(EntityID entity_id) {
        auto it = component_map_.find(entity_id);
        if (it != component_map_.end()) {
            // Swap with last element for O(1) removal
            size_t index = it->second;
            size_t last_index = components_.size() - 1;
            
            if (index != last_index) {
                components_[index] = std::move(components_[last_index]);
                EntityID last_entity = entities_[last_index];
                component_map_[last_entity] = index;
            }
            
            components_.pop_back();
            entities_.pop_back();
            component_map_.erase(it);
        }
    }
    
    // Get all components as contiguous array (for ECS system iteration)
    std::vector<T>& get_all_components() { return components_; }
    const std::vector<T>& get_all_components() const { return components_; }
    
    size_t size() const { return components_.size(); }
    
private:
    std::vector<EntityID> entities_;
    std::vector<T> components_;
    std::unordered_map<EntityID, size_t> component_map_;
};

// System base class
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(f32 delta_time) = 0;
    virtual ComponentMask get_required_components() const = 0;
};

// Entity Manager - handles entity lifecycle and component registration
class EntityManager {
public:
    EntityManager() {
        // Pre-allocate entity pool
        entity_pool_.reserve(MAX_ENTITIES);
        for (EntityID i = 0; i < MAX_ENTITIES; ++i) {
            entity_pool_.push_back(i);
        }
    }
    
    Entity create_entity() {
        if (entity_pool_.empty()) {
            return Entity(INVALID_ENTITY);
        }
        
        EntityID id = entity_pool_.back();
        entity_pool_.pop_back();
        active_entities_.insert(id);
        return Entity(id);
    }
    
    void destroy_entity(EntityID entity_id) {
        if (active_entities_.find(entity_id) != active_entities_.end()) {
            active_entities_.erase(entity_id);
            entity_pool_.push_back(entity_id);
            
            // Remove all components
            for (auto& [comp_id, storage] : component_storages_) {
                storage->remove_entity(entity_id);
            }
        }
    }
    
    template<typename T>
    void register_component_type() {
        u32 id = ComponentRegistry::get_component_id<T>();
        auto storage = std::make_unique<ComponentStorage<T>>();
        component_storages_[id] = std::move(storage);
    }
    
    template<typename T>
    T* get_component(EntityID entity_id) {
        u32 id = ComponentRegistry::get_component_id<T>();
        auto it = component_storages_.find(id);
        if (it != component_storages_.end()) {
            auto* storage = static_cast<ComponentStorage<T>*>(it->second.get());
            return storage->get_component(entity_id);
        }
        return nullptr;
    }
    
    template<typename T>
    T* add_component(EntityID entity_id) {
        u32 id = ComponentRegistry::get_component_id<T>();
        auto it = component_storages_.find(id);
        if (it != component_storages_.end()) {
            auto* storage = static_cast<ComponentStorage<T>*>(it->second.get());
            return storage->add_component(entity_id);
        }
        return nullptr;
    }
    
    bool is_entity_active(EntityID entity_id) const {
        return active_entities_.find(entity_id) != active_entities_.end();
    }
    
    const std::set<EntityID>& get_active_entities() const { return active_entities_; }
    
private:
    std::vector<EntityID> entity_pool_;
    std::set<EntityID> active_entities_;
    std::unordered_map<u32, std::unique_ptr<void>> component_storages_;
};

} // namespace game_tools
