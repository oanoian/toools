/**
 * Alpha1 ECS System Header
 * 
 * System base class and utilities for ECS processing.
 * Most system logic is implemented in entity.h
 * 
 * @file system.h
 */

#pragma once

#include "alpha1/core/types.h"
#include "alpha1/ecs/entity.h"

namespace Alpha1::ECS {

/**
 * System interface - already fully defined in entity.h
 * This header provides additional system utilities
 */

/**
 * Advanced system with explicit entity filtering
 */
class FilteredSystem : public System {
public:
    virtual void Init(World* world) { m_world = world; }
    virtual void Update(DeltaTime dt) override = 0;
    
protected:
    World* m_world = nullptr;
};

} // namespace Alpha1::ECS
