#!/usr/bin/env python3
"""
Scene Builder - Tool for building and managing game scenes.
"""

from typing import Dict, List, Any, Optional
from dataclasses import dataclass, field
import json


@dataclass
class GameObject:
    """Represents a game object in a scene."""
    id: str
    name: str
    position: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    rotation: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    scale: List[float] = field(default_factory=lambda: [1.0, 1.0, 1.0])
    components: Dict[str, Any] = field(default_factory=dict)
    parent_id: Optional[str] = None
    children: List[str] = field(default_factory=list)
    active: bool = True
    
    def to_dict(self) -> dict:
        """Convert to dictionary for serialization."""
        return {
            'id': self.id,
            'name': self.name,
            'position': self.position,
            'rotation': self.rotation,
            'scale': self.scale,
            'components': self.components,
            'parent_id': self.parent_id,
            'children': self.children,
            'active': self.active
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'GameObject':
        """Create from dictionary."""
        return cls(**data)


class SceneBuilder:
    """Builds and manages game scenes with hierarchical game objects."""
    
    def __init__(self, scene_name: str = "Untitled Scene"):
        self.scene_name = scene_name
        self.objects: Dict[str, GameObject] = {}
        self.root_objects: List[str] = []
        self._next_id = 0
    
    def create_object(self, name: str, parent_id: Optional[str] = None) -> GameObject:
        """
        Create a new game object.
        
        Args:
            name: Name of the object
            parent_id: Optional parent object ID for hierarchy
            
        Returns:
            Created GameObject
        """
        obj_id = f"obj_{self._next_id}"
        self._next_id += 1
        
        obj = GameObject(id=obj_id, name=name, parent_id=parent_id)
        self.objects[obj_id] = obj
        
        if parent_id:
            if parent_id in self.objects:
                self.objects[parent_id].children.append(obj_id)
            else:
                print(f"Warning: Parent object {parent_id} not found")
                obj.parent_id = None
                self.root_objects.append(obj_id)
        else:
            self.root_objects.append(obj_id)
        
        return obj
    
    def add_component(self, object_id: str, component_name: str, component_data: Any) -> bool:
        """Add a component to a game object."""
        if object_id not in self.objects:
            print(f"Error: Object {object_id} not found")
            return False
        
        self.objects[object_id].components[component_name] = component_data
        return True
    
    def remove_object(self, object_id: str) -> bool:
        """
        Remove a game object and all its children.
        
        Args:
            object_id: ID of the object to remove
            
        Returns:
            True if successful, False otherwise
        """
        if object_id not in self.objects:
            return False
        
        # Remove children first
        obj = self.objects[object_id]
        for child_id in obj.children[:]:
            self.remove_object(child_id)
        
        # Remove from parent's children list
        if obj.parent_id and obj.parent_id in self.objects:
            if object_id in self.objects[obj.parent_id].children:
                self.objects[obj.parent_id].children.remove(object_id)
        
        # Remove from root objects if present
        if object_id in self.root_objects:
            self.root_objects.remove(object_id)
        
        del self.objects[object_id]
        return True
    
    def set_transform(self, object_id: str, 
                     position: Optional[List[float]] = None,
                     rotation: Optional[List[float]] = None,
                     scale: Optional[List[float]] = None) -> bool:
        """Set the transform properties of an object."""
        if object_id not in self.objects:
            return False
        
        obj = self.objects[object_id]
        if position is not None:
            obj.position = position
        if rotation is not None:
            obj.rotation = rotation
        if scale is not None:
            obj.scale = scale
        
        return True
    
    def get_object(self, object_id: str) -> Optional[GameObject]:
        """Get a game object by ID."""
        return self.objects.get(object_id)
    
    def find_objects_by_name(self, name: str) -> List[GameObject]:
        """Find all objects with a given name."""
        return [obj for obj in self.objects.values() if obj.name == name]
    
    def get_hierarchy(self, object_id: str, indent: int = 0) -> str:
        """Get a string representation of the object hierarchy."""
        if object_id not in self.objects:
            return ""
        
        obj = self.objects[object_id]
        result = "  " * indent + f"{obj.name} ({object_id})\n"
        
        for child_id in obj.children:
            result += self.get_hierarchy(child_id, indent + 1)
        
        return result
    
    def get_full_hierarchy(self) -> str:
        """Get the complete scene hierarchy."""
        result = f"=== {self.scene_name} ===\n"
        for root_id in self.root_objects:
            result += self.get_hierarchy(root_id)
        return result
    
    def to_dict(self) -> dict:
        """Serialize the scene to a dictionary."""
        return {
            'scene_name': self.scene_name,
            'objects': {obj_id: obj.to_dict() for obj_id, obj in self.objects.items()},
            'root_objects': self.root_objects,
            'next_id': self._next_id
        }
    
    def save_to_file(self, filepath: str) -> bool:
        """Save the scene to a JSON file."""
        try:
            with open(filepath, 'w') as f:
                json.dump(self.to_dict(), f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving scene: {e}")
            return False
    
    @classmethod
    def load_from_file(cls, filepath: str) -> Optional['SceneBuilder']:
        """Load a scene from a JSON file."""
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            
            scene = cls(data['scene_name'])
            scene.objects = {
                obj_id: GameObject.from_dict(obj_data)
                for obj_id, obj_data in data['objects'].items()
            }
            scene.root_objects = data['root_objects']
            scene._next_id = data.get('next_id', 0)
            
            return scene
        except Exception as e:
            print(f"Error loading scene: {e}")
            return None
    
    def duplicate_object(self, object_id: str, new_name: Optional[str] = None) -> Optional[GameObject]:
        """Create a copy of an existing object."""
        if object_id not in self.objects:
            return None
        
        original = self.objects[object_id]
        new_obj = self.create_object(new_name or f"{original.name}_copy")
        
        new_obj.position = original.position.copy()
        new_obj.rotation = original.rotation.copy()
        new_obj.scale = original.scale.copy()
        new_obj.components = {k: v.copy() if isinstance(v, dict) else v 
                             for k, v in original.components.items()}
        
        return new_obj
    
    def clear(self):
        """Clear all objects from the scene."""
        self.objects.clear()
        self.root_objects.clear()
        self._next_id = 0
