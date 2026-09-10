#!/usr/bin/env python3
"""
Animation Tools - Utilities for creating and managing game animations.
"""

from typing import List, Dict, Any, Optional, Callable
from dataclasses import dataclass, field
import math
import json


@dataclass
class Keyframe:
    """Represents a single keyframe in an animation."""
    time: float  # Time in seconds
    value: Any   # Can be float, Vector2, Vector3, etc.
    interpolation: str = 'linear'  # 'linear', 'ease_in', 'ease_out', 'ease_in_out'
    
    def to_dict(self) -> dict:
        return {
            'time': self.time,
            'value': self.value,
            'interpolation': self.interpolation
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Keyframe':
        return cls(**data)


@dataclass
class AnimationClip:
    """Represents an animation clip with multiple keyframes."""
    name: str
    duration: float
    keyframes: List[Keyframe] = field(default_factory=list)
    loop: bool = False
    property_name: str = ''  # e.g., 'position', 'rotation', 'scale'
    
    def add_keyframe(self, time: float, value: Any, interpolation: str = 'linear'):
        """Add a keyframe to the animation."""
        keyframe = Keyframe(time=time, value=value, interpolation=interpolation)
        self.keyframes.append(keyframe)
        self.keyframes.sort(key=lambda k: k.time)
        self.duration = max(self.duration, time)
    
    def evaluate(self, time: float) -> Optional[Any]:
        """
        Evaluate the animation at a specific time.
        
        Args:
            time: Time in seconds
            
        Returns:
            Interpolated value at the given time
        """
        if not self.keyframes:
            return None
        
        # Handle looping
        if self.loop and time > self.duration:
            time = time % self.duration
        
        # Clamp to duration
        time = max(0, min(time, self.duration))
        
        # Find surrounding keyframes
        before_key = None
        after_key = None
        
        for i, keyframe in enumerate(self.keyframes):
            if keyframe.time <= time:
                before_key = keyframe
            if keyframe.time >= time and after_key is None:
                after_key = keyframe
                break
        
        # Edge cases
        if before_key is None:
            return self.keyframes[0].value
        if after_key is None:
            return self.keyframes[-1].value
        if before_key == after_key:
            return before_key.value
        
        # Calculate interpolation factor
        duration = after_key.time - before_key.time
        if duration == 0:
            return before_key.value
        
        t = (time - before_key.time) / duration
        
        # Apply easing
        t = self._apply_easing(t, before_key.interpolation)
        
        # Interpolate values
        return self._interpolate(before_key.value, after_key.value, t)
    
    def _apply_easing(self, t: float, interpolation: str) -> float:
        """Apply easing function to interpolation factor."""
        if interpolation == 'linear':
            return t
        elif interpolation == 'ease_in':
            return t * t
        elif interpolation == 'ease_out':
            return t * (2 - t)
        elif interpolation == 'ease_in_out':
            if t < 0.5:
                return 2 * t * t
            else:
                return -1 + (4 - 2 * t) * t
        else:
            return t
    
    def _interpolate(self, start: Any, end: Any, t: float) -> Any:
        """Interpolate between two values."""
        if isinstance(start, (int, float)) and isinstance(end, (int, float)):
            return start + (end - start) * t
        elif hasattr(start, '__iter__') and hasattr(end, '__iter__'):
            # Handle vectors/lists
            result = []
            for s, e in zip(start, end):
                result.append(s + (e - s) * t)
            return type(start)(result)
        else:
            # For non-numeric types, use step interpolation
            return start if t < 0.5 else end
    
    def to_dict(self) -> dict:
        """Serialize to dictionary."""
        return {
            'name': self.name,
            'duration': self.duration,
            'keyframes': [kf.to_dict() for kf in self.keyframes],
            'loop': self.loop,
            'property_name': self.property_name
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'AnimationClip':
        """Deserialize from dictionary."""
        clip = cls(
            name=data['name'],
            duration=data['duration'],
            loop=data.get('loop', False),
            property_name=data.get('property_name', '')
        )
        clip.keyframes = [Keyframe.from_dict(kf) for kf in data.get('keyframes', [])]
        return clip


class Animator:
    """Manages animation playback for a game object."""
    
    def __init__(self):
        self.clips: Dict[str, AnimationClip] = {}
        self.active_clip: Optional[str] = None
        self.current_time: float = 0.0
        self.playing: bool = False
        self.speed: float = 1.0
        self.on_animation_complete: Optional[Callable] = None
    
    def add_clip(self, clip: AnimationClip):
        """Add an animation clip."""
        self.clips[clip.name] = clip
    
    def play(self, clip_name: str, loop: bool = False):
        """Start playing an animation clip."""
        if clip_name not in self.clips:
            print(f"Error: Clip '{clip_name}' not found")
            return
        
        self.active_clip = clip_name
        self.current_time = 0.0
        self.playing = True
        self.clips[clip_name].loop = loop
    
    def stop(self):
        """Stop the current animation."""
        self.playing = False
        self.active_clip = None
    
    def pause(self):
        """Pause the current animation."""
        self.playing = False
    
    def resume(self):
        """Resume the paused animation."""
        if self.active_clip:
            self.playing = True
    
    def update(self, delta_time: float):
        """
        Update the animation state.
        
        Args:
            delta_time: Time elapsed since last update
        """
        if not self.playing or not self.active_clip:
            return
        
        clip = self.clips[self.active_clip]
        self.current_time += delta_time * self.speed
        
        if self.current_time >= clip.duration:
            if clip.loop:
                self.current_time = self.current_time % clip.duration
            else:
                self.current_time = clip.duration
                self.playing = False
                if self.on_animation_complete:
                    self.on_animation_complete()
    
    def get_current_value(self) -> Optional[Any]:
        """Get the current animated value."""
        if not self.active_clip or not self.playing:
            return None
        
        clip = self.clips[self.active_clip]
        return clip.evaluate(self.current_time)
    
    def get_state(self) -> dict:
        """Get the current animator state."""
        return {
            'active_clip': self.active_clip,
            'current_time': self.current_time,
            'playing': self.playing,
            'speed': self.speed
        }


class AnimationTools:
    """Utility functions for working with animations."""
    
    @staticmethod
    def create_position_animation(name: str, positions: List[tuple], 
                                   times: Optional[List[float]] = None,
                                   loop: bool = False) -> AnimationClip:
        """
        Create a position animation from a list of positions.
        
        Args:
            name: Name of the animation
            positions: List of (x, y, z) positions
            times: Optional list of times (defaults to evenly spaced)
            loop: Whether the animation should loop
            
        Returns:
            AnimationClip
        """
        if times is None:
            duration = len(positions) - 1
            times = [i for i in range(len(positions))]
        else:
            duration = times[-1] if times else 0
        
        clip = AnimationClip(name=name, duration=duration, loop=loop, 
                            property_name='position')
        
        for pos, time in zip(positions, times):
            clip.add_keyframe(time, list(pos))
        
        return clip
    
    @staticmethod
    def create_rotation_animation(name: str, rotations: List[tuple],
                                   times: Optional[List[float]] = None,
                                   loop: bool = False) -> AnimationClip:
        """Create a rotation animation from a list of Euler angles."""
        if times is None:
            duration = len(rotations) - 1
            times = [i for i in range(len(rotations))]
        else:
            duration = times[-1] if times else 0
        
        clip = AnimationClip(name=name, duration=duration, loop=loop,
                            property_name='rotation')
        
        for rot, time in zip(rotations, times):
            clip.add_keyframe(time, list(rot))
        
        return clip
    
    @staticmethod
    def create_scale_animation(name: str, scales: List[tuple],
                                times: Optional[List[float]] = None,
                                loop: bool = False) -> AnimationClip:
        """Create a scale animation from a list of scale values."""
        if times is None:
            duration = len(scales) - 1
            times = [i for i in range(len(scales))]
        else:
            duration = times[-1] if times else 0
        
        clip = AnimationClip(name=name, duration=duration, loop=loop,
                            property_name='scale')
        
        for scale, time in zip(scales, times):
            clip.add_keyframe(time, list(scale))
        
        return clip
    
    @staticmethod
    def create_pulse_animation(name: str, start_scale: float = 1.0,
                               end_scale: float = 1.2, duration: float = 0.5,
                               loop: bool = True) -> AnimationClip:
        """Create a simple pulse/scale animation."""
        clip = AnimationClip(name=name, duration=duration * 2, loop=loop,
                            property_name='scale')
        
        clip.add_keyframe(0, [start_scale, start_scale, start_scale], 'ease_out')
        clip.add_keyframe(duration, [end_scale, end_scale, end_scale], 'ease_in')
        clip.add_keyframe(duration * 2, [start_scale, start_scale, start_scale], 'ease_in')
        
        return clip
    
    @staticmethod
    def blend_animations(clip1: AnimationClip, clip2: AnimationClip,
                         blend_factor: float, time: float) -> Any:
        """
        Blend between two animations at a specific time.
        
        Args:
            clip1: First animation clip
            clip2: Second animation clip
            blend_factor: 0.0 = full clip1, 1.0 = full clip2
            time: Time to evaluate
            
        Returns:
            Blended value
        """
        val1 = clip1.evaluate(time)
        val2 = clip2.evaluate(time)
        
        if val1 is None:
            return val2
        if val2 is None:
            return val1
        
        # Simple linear blend
        if isinstance(val1, (int, float)) and isinstance(val2, (int, float)):
            return val1 * (1 - blend_factor) + val2 * blend_factor
        elif hasattr(val1, '__iter__') and hasattr(val2, '__iter__'):
            result = []
            for v1, v2 in zip(val1, val2):
                result.append(v1 * (1 - blend_factor) + v2 * blend_factor)
            return type(val1)(result)
        else:
            return val1 if blend_factor < 0.5 else val2
    
    @staticmethod
    def save_animation_library(animations: List[AnimationClip], filepath: str) -> bool:
        """Save a library of animations to a JSON file."""
        try:
            data = [anim.to_dict() for anim in animations]
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving animation library: {e}")
            return False
    
    @staticmethod
    def load_animation_library(filepath: str) -> List[AnimationClip]:
        """Load a library of animations from a JSON file."""
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            return [AnimationClip.from_dict(item) for item in data]
        except Exception as e:
            print(f"Error loading animation library: {e}")
            return []
    
    @staticmethod
    def calculate_animation_duration(keyframes: List[dict]) -> float:
        """Calculate the total duration of an animation from keyframes."""
        if not keyframes:
            return 0.0
        return max(kf.get('time', 0) for kf in keyframes)
