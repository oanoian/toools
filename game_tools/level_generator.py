#!/usr/bin/env python3
"""
Level Generator - Procedural level generation utilities.
"""

import random
from typing import List, Dict, Tuple, Optional, Set
from dataclasses import dataclass, field
from enum import Enum


class TileType(Enum):
    """Types of tiles for grid-based levels."""
    EMPTY = 0
    FLOOR = 1
    WALL = 2
    DOOR = 3
    CORRIDOR = 4
    ROOM_START = 5
    ROOM_END = 6


@dataclass
class Room:
    """Represents a room in a dungeon or level."""
    x: int
    y: int
    width: int
    height: int
    
    @property
    def center(self) -> Tuple[int, int]:
        return (self.x + self.width // 2, self.y + self.height // 2)
    
    @property
    def bounds(self) -> Tuple[int, int, int, int]:
        """Returns (x1, y1, x2, y2)"""
        return (self.x, self.y, self.x + self.width, self.y + self.height)
    
    def intersects(self, other: 'Room', margin: int = 1) -> bool:
        """Check if this room intersects with another."""
        x1, y1, x2, y2 = self.bounds
        ox1, oy1, ox2, oy2 = other.bounds
        
        return not (x2 + margin < ox1 or x1 > ox2 + margin or
                    y2 + margin < oy1 or y1 > oy2 + margin)
    
    def to_dict(self) -> dict:
        return {
            'x': self.x,
            'y': self.y,
            'width': self.width,
            'height': self.height
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Room':
        return cls(**data)


@dataclass
class Corridor:
    """Represents a corridor connecting two rooms."""
    start: Tuple[int, int]
    end: Tuple[int, int]
    horizontal_first: bool = True
    
    def get_tiles(self) -> List[Tuple[int, int]]:
        """Get all tiles along the corridor path."""
        tiles = []
        x1, y1 = self.start
        x2, y2 = self.end
        
        # Horizontal segment
        if self.horizontal_first:
            step_x = 1 if x2 > x1 else -1
            for x in range(x1, x2 + step_x, step_x):
                tiles.append((x, y1))
            
            # Vertical segment
            step_y = 1 if y2 > y1 else -1
            for y in range(y1, y2 + step_y, step_y):
                tiles.append((x2, y))
        else:
            # Vertical first
            step_y = 1 if y2 > y1 else -1
            for y in range(y1, y2 + step_y, step_y):
                tiles.append((x1, y))
            
            # Horizontal segment
            step_x = 1 if x2 > x1 else -1
            for x in range(x1, x2 + step_x, step_x):
                tiles.append((x, y2))
        
        return tiles


class LevelGenerator:
    """Procedural level generator for dungeon-crawler and roguelike games."""
    
    def __init__(self, width: int = 50, height: int = 50):
        self.width = width
        self.height = height
        self.grid: List[List[TileType]] = []
        self.rooms: List[Room] = []
        self.corridors: List[Corridor] = []
        self.seed: Optional[int] = None
    
    def set_seed(self, seed: int):
        """Set random seed for reproducible generation."""
        self.seed = seed
        random.seed(seed)
    
    def _initialize_grid(self):
        """Initialize the grid with empty tiles."""
        self.grid = [[TileType.EMPTY for _ in range(self.width)] 
                     for _ in range(self.height)]
    
    def generate_dungeon(self, min_rooms: int = 10, max_rooms: int = 20,
                        min_room_size: int = 5, max_room_size: int = 15,
                        max_attempts: int = 100) -> bool:
        """
        Generate a dungeon using BSP-like room placement.
        
        Args:
            min_rooms: Minimum number of rooms to generate
            max_rooms: Maximum number of rooms
            min_room_size: Minimum room dimension
            max_room_size: Maximum room dimension
            max_attempts: Maximum attempts to place rooms
            
        Returns:
            True if generation was successful
        """
        self._initialize_grid()
        self.rooms.clear()
        self.corridors.clear()
        
        rooms_placed = 0
        attempts = 0
        
        while rooms_placed < max_rooms and attempts < max_attempts:
            attempts += 1
            
            # Generate random room dimensions
            width = random.randint(min_room_size, max_room_size)
            height = random.randint(min_room_size, max_room_size)
            
            # Generate random position
            x = random.randint(1, self.width - width - 1)
            y = random.randint(1, self.height - height - 1)
            
            new_room = Room(x, y, width, height)
            
            # Check for intersections with existing rooms
            intersects = False
            for existing_room in self.rooms:
                if new_room.intersects(existing_room):
                    intersects = True
                    break
            
            if not intersects:
                self.rooms.append(new_room)
                rooms_placed += 1
        
        if rooms_placed < min_rooms:
            print(f"Warning: Only placed {rooms_placed} rooms (minimum: {min_rooms})")
            return False
        
        # Connect rooms with corridors
        self._connect_rooms()
        
        # Carve rooms and corridors into the grid
        self._carve_level()
        
        return True
    
    def _connect_rooms(self):
        """Connect all rooms using a minimum spanning tree approach."""
        if len(self.rooms) < 2:
            return
        
        connected: Set[int] = {0}
        unconnected: Set[int] = set(range(1, len(self.rooms)))
        
        while unconnected:
            best_distance = float('inf')
            best_pair = None
            
            # Find closest pair of connected/unconnected rooms
            for ci in connected:
                for ui in unconnected:
                    c_room = self.rooms[ci]
                    u_room = self.rooms[ui]
                    
                    cx, cy = c_room.center
                    ux, uy = u_room.center
                    
                    distance = abs(cx - ux) + abs(cy - uy)
                    
                    if distance < best_distance:
                        best_distance = distance
                        best_pair = (ci, ui)
            
            if best_pair:
                ci, ui = best_pair
                c_room = self.rooms[ci]
                u_room = self.rooms[ui]
                
                # Create corridor between room centers
                corridor = Corridor(
                    start=c_room.center,
                    end=u_room.center,
                    horizontal_first=random.choice([True, False])
                )
                self.corridors.append(corridor)
                
                connected.add(ui)
                unconnected.remove(ui)
    
    def _carve_level(self):
        """Carve rooms and corridors into the grid."""
        # Carve rooms
        for room in self.rooms:
            x1, y1, x2, y2 = room.bounds
            
            for y in range(y1, y2):
                for x in range(x1, x2):
                    if 0 <= x < self.width and 0 <= y < self.height:
                        self.grid[y][x] = TileType.FLOOR
            
            # Mark room boundaries as walls
            for x in range(x1, x2):
                if y1 - 1 >= 0:
                    self.grid[y1 - 1][x] = TileType.WALL
                if y2 < self.height:
                    self.grid[y2][x] = TileType.WALL
            
            for y in range(y1, y2):
                if x1 - 1 >= 0:
                    self.grid[y][x1 - 1] = TileType.WALL
                if x2 < self.width:
                    self.grid[y][x2] = TileType.WALL
        
        # Carve corridors
        for corridor in self.corridors:
            for x, y in corridor.get_tiles():
                if 0 <= x < self.width and 0 <= y < self.height:
                    if self.grid[y][x] == TileType.EMPTY:
                        self.grid[y][x] = TileType.CORRIDOR
    
    def get_tile(self, x: int, y: int) -> TileType:
        """Get the tile type at a position."""
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.grid[y][x]
        return TileType.EMPTY
    
    def is_walkable(self, x: int, y: int) -> bool:
        """Check if a tile is walkable."""
        tile = self.get_tile(x, y)
        return tile in [TileType.FLOOR, TileType.CORRIDOR, TileType.DOOR]
    
    def get_spawn_points(self) -> List[Tuple[int, int]]:
        """Get valid spawn points (floor tiles)."""
        points = []
        for y in range(self.height):
            for x in range(self.width):
                if self.grid[y][x] == TileType.FLOOR:
                    points.append((x, y))
        return points
    
    def get_room_for_point(self, x: int, y: int) -> Optional[Room]:
        """Find which room contains a point."""
        for room in self.rooms:
            x1, y1, x2, y2 = room.bounds
            if x1 <= x < x2 and y1 <= y < y2:
                return room
        return None
    
    def export_grid(self) -> List[List[int]]:
        """Export the grid as a 2D array of integers."""
        return [[tile.value for tile in row] for row in self.grid]
    
    def export_to_dict(self) -> dict:
        """Export the entire level to a dictionary."""
        return {
            'width': self.width,
            'height': self.height,
            'grid': self.export_grid(),
            'rooms': [room.to_dict() for room in self.rooms],
            'corridors': [
                {
                    'start': c.start,
                    'end': c.end,
                    'horizontal_first': c.horizontal_first
                }
                for c in self.corridors
            ],
            'seed': self.seed
        }
    
    @classmethod
    def import_from_dict(cls, data: dict) -> 'LevelGenerator':
        """Import a level from a dictionary."""
        gen = cls(data['width'], data['height'])
        gen.seed = data.get('seed')
        
        if gen.seed:
            random.seed(gen.seed)
        
        gen.grid = [[TileType(val) for val in row] for row in data['grid']]
        gen.rooms = [Room.from_dict(r) for r in data.get('rooms', [])]
        
        for c_data in data.get('corridors', []):
            gen.corridors.append(Corridor(
                start=tuple(c_data['start']),
                end=tuple(c_data['end']),
                horizontal_first=c_data.get('horizontal_first', True)
            ))
        
        return gen
    
    def visualize(self) -> str:
        """Create an ASCII visualization of the level."""
        chars = {
            TileType.EMPTY: '#',
            TileType.FLOOR: '.',
            TileType.WALL: '#',
            TileType.DOOR: '+',
            TileType.CORRIDOR: ',',
            TileType.ROOM_START: '<',
            TileType.ROOM_END: '>'
        }
        
        lines = []
        for row in self.grid:
            line = ''.join(chars[tile] for tile in row)
            lines.append(line)
        
        return '\n'.join(lines)


class WaveBasedLevelGenerator:
    """Generate levels for wave-based games like tower defense."""
    
    @staticmethod
    def generate_arena(width: int, height: int, 
                       obstacle_count: int = 10,
                       obstacle_size_range: Tuple[int, int] = (2, 5)) -> List[List[int]]:
        """
        Generate an arena with random obstacles.
        
        Args:
            width: Arena width
            height: Arena height
            obstacle_count: Number of obstacles to place
            obstacle_size_range: Min and max obstacle size
            
        Returns:
            2D grid where 0 = walkable, 1 = obstacle
        """
        grid = [[0 for _ in range(width)] for _ in range(height)]
        
        # Leave borders clear
        for x in range(width):
            grid[0][x] = 1
            grid[height-1][x] = 1
        
        for y in range(height):
            grid[y][0] = 1
            grid[y][width-1] = 1
        
        # Place random obstacles
        placed = 0
        attempts = 0
        min_size, max_size = obstacle_size_range
        
        while placed < obstacle_count and attempts < obstacle_count * 10:
            attempts += 1
            
            size = random.randint(min_size, max_size)
            x = random.randint(2, width - size - 2)
            y = random.randint(2, height - size - 2)
            
            # Check if area is clear
            clear = True
            for dy in range(size):
                for dx in range(size):
                    if grid[y + dy][x + dx] != 0:
                        clear = False
                        break
                if not clear:
                    break
            
            if clear:
                for dy in range(size):
                    for dx in range(size):
                        grid[y + dy][x + dx] = 1
                placed += 1
        
        return grid
    
    @staticmethod
    def generate_path(width: int, height: int, 
                      path_width: int = 2,
                      num_turns: int = 5) -> List[Tuple[int, int]]:
        """
        Generate a winding path through the level.
        
        Returns:
            List of path waypoints
        """
        waypoints = []
        
        # Start from left side
        current_x = 0
        current_y = height // 2
        waypoints.append((current_x, current_y))
        
        segment_width = width // (num_turns + 1)
        
        for i in range(num_turns):
            current_x += segment_width + random.randint(-segment_width//2, segment_width//2)
            current_x = min(current_x, width - 2)
            
            # Alternate direction for y
            direction = 1 if i % 2 == 0 else -1
            current_y += random.randint(2, height // 4) * direction
            current_y = max(path_width, min(current_y, height - path_width - 1))
            
            waypoints.append((current_x, current_y))
        
        # End at right side
        waypoints.append((width - 1, current_y))
        
        return waypoints
