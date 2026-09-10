/**
 * Alpha1 Entity Component System (ECS)
 * 
 * Data-oriented entity system optimized for cache locality and parallel processing.
 * Entities are simple IDs, components are plain data, systems operate on contiguous arrays.
 * 
 * @file entity.h
 */

#pragma once

#include "alpha1/core/types.h"
#include "alpha1/core/memory.h"
#include <vector>
#include <unordered_map>
#include <bitset>
#include <type_traits>

namespace Alpha1::ECS {

using namespace Core;

/**
 * Maximum number of component types supported
 */
constexpr size_t MAX_COMPONENT_TYPES = 64;

/**
 * Component base class (marker type)
 */
struct Component {
    virtual ~Component() = default;
};

/**
 * Component type registry
 */
class ComponentTypeRegistry {
public:
    template<typename T>
    static ComponentTypeID GetTypeID() {
        static ComponentTypeID id = s_nextID++;
        return id;
    }
    
    static ComponentTypeID GetNextID() {
        return s_nextID;
    }
    
private:
    static inline ComponentTypeID s_nextID = 0;
};

/**
 * Entity handle with validity checking
 */
class Entity {
public:
    Entity() : m_id(INVALID_ENTITY), m_version(0) {}
    
    bool IsValid() const { return m_id != INVALID_ENTITY; }
    EntityID GetID() const { return m_id; }
    uint32_t GetVersion() const { return m_version; }
    
    bool operator==(const Entity& other) const {
        return m_id == other.m_id && m_version == other.version;
    }
    
    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }
    
private:
    friend class World;
    
    Entity(EntityID id, uint32_t version) : m_id(id), m_version(version) {}
    
    EntityID m_id;
    uint32_t m_version;
};

/**
 * Component array for a specific type - stored contiguously for cache efficiency
 */
template<typename T>
class ComponentArray {
public:
    using ValueType = T;
    
    void Insert(EntityID entity, const T& component) {
        if (m_size >= m_entities.size()) {
            Grow();
        }
        
        m_entities[m_size] = entity;
        m_components[m_size] = component;
        m_entityToIndex[entity] = m_size;
        ++m_size;
    }
    
    void Remove(EntityID entity) {
        auto it = m_entityToIndex.find(entity);
        if (it == m_entityToIndex.end()) {
            return;
        }
        
        IndexType index = it->second;
        IndexType lastIndex = m_size - 1;
        
        // Swap with last element
        m_entities[index] = m_entities[lastIndex];
        m_components[index] = m_components[lastIndex];
        m_entityToIndex[m_entities[lastIndex]] = index;
        
        // Remove last
        m_entityToIndex.erase(entity);
        --m_size;
    }
    
    T* Get(EntityID entity) {
        auto it = m_entityToIndex.find(entity);
        if (it == m_entityToIndex.end()) {
            return nullptr;
        }
        return &m_components[it->second];
    }
    
    const T* Get(EntityID entity) const {
        auto it = m_entityToIndex.find(entity);
        if (it == m_entityToIndex.end()) {
            return nullptr;
        }
        return &m_components[it->second];
    }
    
    // Direct access to contiguous component array for system iteration
    T* Data() { return m_components.data(); }
    const T* Data() const { return m_components.data(); }
    
    size_t Size() const { return m_size; }
    bool Empty() const { return m_size == 0; }
    
private:
    void Grow() {
        size_t newCapacity = m_entities.empty() ? 1024 : m_entities.size() * 2;
        m_entities.resize(newCapacity);
        m_components.resize(newCapacity);
    }
    
    std::vector<EntityID> m_entities;
    std::vector<T> m_components;
    std::unordered_map<EntityID, IndexType> m_entityToIndex;
    size_t m_size = 0;
};

/**
 * Bitset representing which components an entity has
 */
using ComponentBitset = std::bitset<MAX_COMPONENT_TYPES>;

/**
 * Entity signature for matching entities with specific component combinations
 */
class Signature {
public:
    Signature() = default;
    
    template<typename T>
    Signature& Set() {
        m_bits.set(ComponentTypeRegistry::GetTypeID<T>());
        return *this;
    }
    
    template<typename T>
    Signature& Unset() {
        m_bits.reset(ComponentTypeRegistry::GetTypeID<T>());
        return *this;
    }
    
    bool Matches(const ComponentBitset& entityBits) const {
        return (entityBits & m_bits) == m_bits;
    }
    
    const ComponentBitset& GetBits() const { return m_bits; }
    
private:
    ComponentBitset m_bits;
};

/**
 * System base class
 */
class System {
public:
    virtual ~System() = default;
    virtual void Init() {}
    virtual void Update(DeltaTime dt) = 0;
    virtual void Shutdown() {}
    
    void SetSignature(const Signature& sig) { m_signature = sig; }
    const Signature& GetSignature() const { return m_signature; }
    
protected:
    Signature m_signature;
};

/**
 * ECS World - manages all entities and components
 */
class World {
public:
    World() : m_nextEntity(0), m_entityCapacity(1024) {
        m_entityVersions.resize(m_entityCapacity, 0);
        m_entitySignatures.resize(m_entityCapacity);
    }
    
    /**
     * Create a new entity
     */
    Entity CreateEntity() {
        if (m_nextEntity >= m_entityCapacity) {
            GrowEntityPool();
        }
        
        EntityID id = m_nextEntity++;
        uint32_t version = m_entityVersions[id];
        
        return Entity(id, version);
    }
    
    /**
     * Destroy an entity and all its components
     */
    void DestroyEntity(Entity entity) {
        if (!entity.IsValid()) {
            return;
        }
        
        EntityID id = entity.GetID();
        
        // Increment version to invalidate old handles
        ++m_entityVersions[id];
        
        // Clear signature
        m_entitySignatures[id].reset();
    }
    
    /**
     * Add a component to an entity
     */
    template<typename T, typename... Args>
    void AddComponent(Entity entity, Args&&... args) {
        if (!entity.IsValid()) {
            return;
        }
        
        EntityID id = entity.GetID();
        ComponentTypeID typeID = ComponentTypeRegistry::GetTypeID<T>();
        
        // Add to component array
        GetComponentArray<T>().Insert(id, T(std::forward<Args>(args)...));
        
        // Update signature
        m_entitySignatures[id].set(typeID);
    }
    
    /**
     * Remove a component from an entity
     */
    template<typename T>
    void RemoveComponent(Entity entity) {
        if (!entity.IsValid()) {
            return;
        }
        
        EntityID id = entity.GetID();
        ComponentTypeID typeID = ComponentTypeRegistry::GetTypeID<T>();
        
        // Remove from component array
        GetComponentArray<T>().Remove(id);
        
        // Update signature
        m_entitySignatures[id].reset(typeID);
    }
    
    /**
     * Get a component from an entity
     */
    template<typename T>
    T* GetComponent(Entity entity) {
        if (!entity.IsValid()) {
            return nullptr;
        }
        return GetComponentArray<T>().Get(entity.GetID());
    }
    
    template<typename T>
    const T* GetComponent(Entity entity) const {
        if (!entity.IsValid()) {
            return nullptr;
        }
        return GetComponentArray<T>().Get(entity.GetID());
    }
    
    /**
     * Check if entity has a component
     */
    template<typename T>
    bool HasComponent(Entity entity) const {
        if (!entity.IsValid()) {
            return false;
        }
        return GetComponent<T>(entity) != nullptr;
    }
    
    /**
     * Register a system
     */
    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        m_systems.push_back(std::move(system));
        return ptr;
    }
    
    /**
     * Get all component arrays for iteration
     */
    template<typename T>
    ComponentArray<T>& GetComponentArray() {
        return GetComponentArrayImpl<T>();
    }
    
    template<typename T>
    const ComponentArray<T>& GetComponentArray() const {
        return GetComponentArrayImpl<T>();
    }
    
    /**
     * Update all systems
     */
    void UpdateSystems(DeltaTime dt) {
        for (auto& system : m_systems) {
            system->Update(dt);
        }
    }
    
private:
    template<typename T>
    ComponentArray<T>& GetComponentArrayImpl() {
        // Use static storage for each component type
        static ComponentArray<T> array;
        return array;
    }
    
    void GrowEntityPool() {
        size_t newCapacity = m_entityCapacity * 2;
        m_entityVersions.resize(newCapacity, 0);
        m_entitySignatures.resize(newCapacity);
        m_entityCapacity = newCapacity;
    }
    
    std::vector<std::unique_ptr<System>> m_systems;
    std::vector<uint32_t> m_entityVersions;
    std::vector<ComponentBitset> m_entitySignatures;
    EntityID m_nextEntity;
    size_t m_entityCapacity;
};

} // namespace Alpha1::ECS
