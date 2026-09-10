/**
 * Alpha1 ECS Component Header
 * 
 * Component base classes and utilities.
 * This header is intentionally minimal - most ECS logic is in entity.h
 * 
 * @file component.h
 */

#pragma once

#include "alpha1/core/types.h"
#include "alpha1/ecs/entity.h"

namespace Alpha1::ECS {

/**
 * Component marker interface
 * All components should inherit from this (optional)
 */
struct IComponent {
    virtual ~IComponent() = default;
};

} // namespace Alpha1::ECS
