/**
 * Alpha1 Game Development Framework
 * 
 * A high-performance C++17 framework for game development tooling,
 * featuring vendor-specific GPU optimizations, ECS architecture,
 * and production-ready asset pipelines.
 * 
 * @file alpha1.h
 */

#pragma once

// Core Systems
#include "alpha1/core/types.h"
#include "alpha1/core/memory.h"
#include "alpha1/core/logging.h"
#include "alpha1/core/threading.h"

// Entity Component System
#include "alpha1/ecs/entity.h"
#include "alpha1/ecs/component.h"
#include "alpha1/ecs/system.h"
#include "alpha1/ecs/world.h"

// Asset Pipeline
#include "alpha1/asset/asset_manager.h"
#include "alpha1/asset/vfs.h"
#include "alpha1/asset/cooker.h"

// DCC & USD Integration
#include "alpha1/dcc/usd_pipeline.h"
#include "alpha1/dcc/live_link.h"

// Version Control
#include "alpha1/vcs/version_control.h"

// Renderer (Vulkan with NVIDIA/AMD optimizations)
#include "alpha1/renderer/vulkan_engine.h"
#include "alpha1/renderer/nvidia_optimizations.h"
#include "alpha1/renderer/amd_optimizations.h"

// DLL Optimization System
#include "alpha1/dll/dll_manager.h"
#include "alpha1/dll/dependency_tracker.h"
#include "alpha1/dll/hot_reloader.h"

namespace Alpha1 {

/**
 * Framework version information
 */
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "1.0.0";

/**
 * Initialize the Alpha1 framework
 * 
 * @param config_path Path to configuration file
 * @return Result code indicating success or failure
 */
A1_API Result<void> Initialize(const char* config_path = nullptr);

/**
 * Shutdown the Alpha1 framework and release all resources
 */
A1_API void Shutdown();

/**
 * Check if the framework is initialized
 * 
 * @return true if initialized, false otherwise
 */
A1_API bool IsInitialized();

/**
 * Get the current frame time in seconds
 * 
 * @return Frame time in seconds
 */
A1_API float GetFrameTime();

/**
 * Get the current FPS
 * 
 * @return Frames per second
 */
A1_API float GetFPS();

/**
 * Main update loop - call once per frame
 */
A1_API void Update();

} // namespace Alpha1
