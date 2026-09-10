#!/usr/bin/env python3
"""
Game Development Tools Suite
A collection of utilities for game developers.
"""

__version__ = "1.0.0"
__author__ = "Game Dev Tools"

from .asset_manager import AssetManager
from .scene_builder import SceneBuilder
from .collision_utils import CollisionUtils
from .animation_tools import AnimationTools
from .level_generator import LevelGenerator
from .vulkan_engine import (
    VulkanOptimizationManager,
    NvidiaOptimizations,
    AmdOptimizations,
    MemoryAllocator,
    CommandBufferOptimizer,
    BatchRenderer,
    TiledRenderer,
    GPUVendor,
    VendorExtensions
)

__all__ = [
    'AssetManager',
    'SceneBuilder',
    'CollisionUtils',
    'AnimationTools',
    'LevelGenerator',
    # Vulkan Engine
    'VulkanOptimizationManager',
    'NvidiaOptimizations',
    'AmdOptimizations',
    'MemoryAllocator',
    'CommandBufferOptimizer',
    'BatchRenderer',
    'TiledRenderer',
    'GPUVendor',
    'VendorExtensions'
]
