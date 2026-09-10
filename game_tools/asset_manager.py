#!/usr/bin/env python3
"""
Asset Manager - Handles loading, caching, and management of game assets.
"""

import os
import json
from typing import Dict, Any, Optional
from pathlib import Path


class AssetManager:
    """Manages game assets including textures, sounds, models, and configurations."""
    
    def __init__(self, base_path: str = "./assets"):
        self.base_path = Path(base_path)
        self.assets: Dict[str, Any] = {}
        self.asset_metadata: Dict[str, dict] = {}
        self._ensure_directories()
    
    def _ensure_directories(self):
        """Create standard asset directories if they don't exist."""
        dirs = ['textures', 'sounds', 'models', 'configs', 'fonts', 'shaders']
        for dir_name in dirs:
            (self.base_path / dir_name).mkdir(parents=True, exist_ok=True)
    
    def load_asset(self, asset_path: str, asset_type: str = 'auto') -> Optional[Any]:
        """
        Load an asset from disk.
        
        Args:
            asset_path: Path to the asset file
            asset_type: Type of asset ('texture', 'sound', 'model', 'config', 'auto')
            
        Returns:
            Loaded asset data or None if failed
        """
        full_path = self.base_path / asset_path
        
        if not full_path.exists():
            print(f"Warning: Asset not found: {full_path}")
            return None
        
        if asset_type == 'auto':
            asset_type = self._detect_asset_type(full_path)
        
        try:
            if asset_type == 'config':
                with open(full_path, 'r') as f:
                    asset_data = json.load(f)
            elif asset_type in ['texture', 'sound', 'model']:
                # Placeholder for binary asset loading
                asset_data = {'path': str(full_path), 'type': asset_type, 'size': full_path.stat().st_size}
            else:
                with open(full_path, 'rb') as f:
                    asset_data = f.read()
            
            self.assets[asset_path] = asset_data
            self.asset_metadata[asset_path] = {
                'type': asset_type,
                'size': full_path.stat().st_size,
                'loaded': True
            }
            
            return asset_data
            
        except Exception as e:
            print(f"Error loading asset {asset_path}: {e}")
            return None
    
    def _detect_asset_type(self, path: Path) -> str:
        """Detect asset type based on file extension."""
        ext = path.suffix.lower()
        texture_exts = ['.png', '.jpg', '.jpeg', '.gif', '.bmp', '.tga']
        sound_exts = ['.wav', '.mp3', '.ogg', '.flac']
        model_exts = ['.obj', '.fbx', '.gltf', '.glb']
        config_exts = ['.json', '.yaml', '.yml', '.xml']
        
        if ext in texture_exts:
            return 'texture'
        elif ext in sound_exts:
            return 'sound'
        elif ext in model_exts:
            return 'model'
        elif ext in config_exts:
            return 'config'
        else:
            return 'binary'
    
    def get_asset(self, asset_path: str) -> Optional[Any]:
        """Get a loaded asset from cache."""
        return self.assets.get(asset_path)
    
    def unload_asset(self, asset_path: str) -> bool:
        """Unload an asset from memory."""
        if asset_path in self.assets:
            del self.assets[asset_path]
            if asset_path in self.asset_metadata:
                self.asset_metadata[asset_path]['loaded'] = False
            return True
        return False
    
    def clear_cache(self):
        """Clear all loaded assets from memory."""
        self.assets.clear()
        self.asset_metadata.clear()
    
    def list_assets(self, asset_type: Optional[str] = None) -> list:
        """List all available assets, optionally filtered by type."""
        assets = []
        for root, dirs, files in os.walk(self.base_path):
            for file in files:
                file_path = Path(root) / file
                rel_path = str(file_path.relative_to(self.base_path))
                
                if asset_type is None or self._detect_asset_type(file_path) == asset_type:
                    assets.append(rel_path)
        
        return assets
    
    def save_config(self, config_name: str, config_data: dict) -> bool:
        """Save a configuration file."""
        try:
            config_path = self.base_path / 'configs' / config_name
            config_path.parent.mkdir(parents=True, exist_ok=True)
            
            with open(config_path, 'w') as f:
                json.dump(config_data, f, indent=2)
            
            return True
        except Exception as e:
            print(f"Error saving config: {e}")
            return False
    
    def get_stats(self) -> dict:
        """Get asset manager statistics."""
        total_size = sum(meta.get('size', 0) for meta in self.asset_metadata.values())
        return {
            'total_assets': len(self.assets),
            'cached_assets': sum(1 for m in self.asset_metadata.values() if m.get('loaded', False)),
            'total_memory_usage': total_size,
            'asset_types': self._count_by_type()
        }
    
    def _count_by_type(self) -> dict:
        """Count assets by type."""
        type_counts = {}
        for meta in self.asset_metadata.values():
            asset_type = meta.get('type', 'unknown')
            type_counts[asset_type] = type_counts.get(asset_type, 0) + 1
        return type_counts
