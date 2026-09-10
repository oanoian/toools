/**
 * Alpha1 ECS World Header
 * 
 * World management for ECS - forward declaration to entity.h
 * where the full implementation resides.
 * 
 * @file world.h
 */

#pragma once

#include "alpha1/ecs/entity.h"

namespace Alpha1::ECS {

// World class is fully defined in entity.h
// This header exists for include consistency

using World = ECS::World;

} // namespace Alpha1::ECS
