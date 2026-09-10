#!/usr/bin/env python3
"""
Collision Utils - 2D and 3D collision detection utilities.
"""

import math
from typing import List, Tuple, Optional
from dataclasses import dataclass


@dataclass
class Vector2:
    """2D vector."""
    x: float = 0.0
    y: float = 0.0
    
    def __add__(self, other: 'Vector2') -> 'Vector2':
        return Vector2(self.x + other.x, self.y + other.y)
    
    def __sub__(self, other: 'Vector2') -> 'Vector2':
        return Vector2(self.x - other.x, self.y - other.y)
    
    def __mul__(self, scalar: float) -> 'Vector2':
        return Vector2(self.x * scalar, self.y * scalar)
    
    def magnitude(self) -> float:
        return math.sqrt(self.x ** 2 + self.y ** 2)
    
    def normalized(self) -> 'Vector2':
        mag = self.magnitude()
        if mag > 0:
            return Vector2(self.x / mag, self.y / mag)
        return Vector2(0, 0)
    
    def dot(self, other: 'Vector2') -> float:
        return self.x * other.x + self.y * other.y
    
    def distance_to(self, other: 'Vector2') -> float:
        return (other - self).magnitude()


@dataclass
class Vector3:
    """3D vector."""
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0
    
    def __add__(self, other: 'Vector3') -> 'Vector3':
        return Vector3(self.x + other.x, self.y + other.y, self.z + other.z)
    
    def __sub__(self, other: 'Vector3') -> 'Vector3':
        return Vector3(self.x - other.x, self.y - other.y, self.z - other.z)
    
    def __mul__(self, scalar: float) -> 'Vector3':
        return Vector3(self.x * scalar, self.y * scalar, self.z * scalar)
    
    def magnitude(self) -> float:
        return math.sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2)
    
    def normalized(self) -> 'Vector3':
        mag = self.magnitude()
        if mag > 0:
            return Vector3(self.x / mag, self.y / mag, self.z / mag)
        return Vector3(0, 0, 0)
    
    def dot(self, other: 'Vector3') -> float:
        return self.x * other.x + self.y * other.y + self.z * other.z
    
    def cross(self, other: 'Vector3') -> 'Vector3':
        return Vector3(
            self.y * other.z - self.z * other.y,
            self.z * other.x - self.x * other.z,
            self.x * other.y - self.y * other.x
        )
    
    def distance_to(self, other: 'Vector3') -> float:
        return (other - self).magnitude()


class CollisionUtils:
    """Collection of collision detection functions for 2D and 3D."""
    
    @staticmethod
    def point_circle_collision_2d(point: Vector2, circle_center: Vector2, 
                                   radius: float) -> bool:
        """Check if a point is inside a circle."""
        return point.distance_to(circle_center) <= radius
    
    @staticmethod
    def circle_circle_collision_2d(center1: Vector2, radius1: float,
                                    center2: Vector2, radius2: float) -> bool:
        """Check if two circles are colliding."""
        distance = center1.distance_to(center2)
        return distance <= (radius1 + radius2)
    
    @staticmethod
    def rect_rect_collision_2d(pos1: Vector2, size1: Vector2,
                                pos2: Vector2, size2: Vector2) -> bool:
        """
        Check if two axis-aligned rectangles are colliding.
        
        Args:
            pos1: Center position of rectangle 1
            size1: Size (width, height) of rectangle 1
            pos2: Center position of rectangle 2
            size2: Size (width, height) of rectangle 2
        """
        half1 = size1 * 0.5
        half2 = size2 * 0.5
        
        dx = abs(pos1.x - pos2.x)
        dy = abs(pos1.y - pos2.y)
        
        return dx <= (half1.x + half2.x) and dy <= (half1.y + half2.y)
    
    @staticmethod
    def point_rect_collision_2d(point: Vector2, rect_pos: Vector2, 
                                 rect_size: Vector2) -> bool:
        """Check if a point is inside an axis-aligned rectangle."""
        half_size = rect_size * 0.5
        left = rect_pos.x - half_size.x
        right = rect_pos.x + half_size.x
        top = rect_pos.y + half_size.y
        bottom = rect_pos.y - half_size.y
        
        return (left <= point.x <= right) and (bottom <= point.y <= top)
    
    @staticmethod
    def line_circle_collision_2d(line_start: Vector2, line_end: Vector2,
                                  circle_center: Vector2, radius: float) -> bool:
        """Check if a line segment intersects with a circle."""
        # Vector from line start to end
        line_vec = line_end - line_start
        # Vector from line start to circle center
        start_to_center = circle_center - line_start
        
        # Project start_to_center onto line_vec
        t = start_to_center.dot(line_vec) / (line_vec.dot(line_vec) + 1e-10)
        
        # Clamp t to [0, 1] to stay on the line segment
        t = max(0, min(1, t))
        
        # Find closest point on line segment
        closest_point = line_start + line_vec * t
        
        return closest_point.distance_to(circle_center) <= radius
    
    @staticmethod
    def sphere_sphere_collision_3d(center1: Vector3, radius1: float,
                                    center2: Vector3, radius2: float) -> bool:
        """Check if two spheres are colliding."""
        distance = center1.distance_to(center2)
        return distance <= (radius1 + radius2)
    
    @staticmethod
    def point_sphere_collision_3d(point: Vector3, sphere_center: Vector3, 
                                   radius: float) -> bool:
        """Check if a point is inside a sphere."""
        return point.distance_to(sphere_center) <= radius
    
    @staticmethod
    def box_box_collision_3d(pos1: Vector3, size1: Vector3,
                              pos2: Vector3, size2: Vector3) -> bool:
        """
        Check if two axis-aligned bounding boxes (AABB) are colliding.
        
        Args:
            pos1: Center position of box 1
            size1: Size (width, height, depth) of box 1
            pos2: Center position of box 2
            size2: Size (width, height, depth) of box 2
        """
        half1 = size1 * 0.5
        half2 = size2 * 0.5
        
        dx = abs(pos1.x - pos2.x)
        dy = abs(pos1.y - pos2.y)
        dz = abs(pos1.z - pos2.z)
        
        return (dx <= (half1.x + half2.x) and 
                dy <= (half1.y + half2.y) and 
                dz <= (half1.z + half2.z))
    
    @staticmethod
    def ray_sphere_intersection_3d(ray_origin: Vector3, ray_direction: Vector3,
                                    sphere_center: Vector3, sphere_radius: float) -> Optional[float]:
        """
        Check if a ray intersects with a sphere.
        
        Args:
            ray_origin: Origin point of the ray
            ray_direction: Normalized direction vector of the ray
            sphere_center: Center of the sphere
            sphere_radius: Radius of the sphere
            
        Returns:
            Distance to intersection point, or None if no intersection
        """
        # Vector from ray origin to sphere center
        oc = ray_origin - sphere_center
        
        # Quadratic equation coefficients
        b = oc.dot(ray_direction)
        c = oc.dot(oc) - sphere_radius ** 2
        
        discriminant = b * b - c
        
        if discriminant < 0:
            return None
        
        # Return the closest intersection
        t = -b - math.sqrt(discriminant)
        if t < 0:
            t = -b + math.sqrt(discriminant)
            if t < 0:
                return None
        
        return t
    
    @staticmethod
    def get_collision_normal_2d(pos1: Vector2, size1: Vector2,
                                 pos2: Vector2, size2: Vector2) -> Vector2:
        """
        Get the collision normal between two rectangles.
        
        Returns:
            Normal vector pointing from rect1 to rect2
        """
        dx = pos2.x - pos1.x
        dy = pos2.y - pos1.y
        
        half1 = size1 * 0.5
        half2 = size2 * 0.5
        
        overlap_x = (half1.x + half2.x) - abs(dx)
        overlap_y = (half1.y + half2.y) - abs(dy)
        
        if overlap_x < overlap_y:
            return Vector2(1 if dx > 0 else -1, 0)
        else:
            return Vector2(0, 1 if dy > 0 else -1)
    
    @staticmethod
    def get_collision_depth_2d(pos1: Vector2, size1: Vector2,
                                pos2: Vector2, size2: Vector2) -> float:
        """Get the penetration depth between two rectangles."""
        dx = abs(pos2.x - pos1.x)
        dy = abs(pos2.y - pos1.y)
        
        half1 = size1 * 0.5
        half2 = size2 * 0.5
        
        overlap_x = (half1.x + half2.x) - dx
        overlap_y = (half1.y + half2.y) - dy
        
        return min(overlap_x, overlap_y) if overlap_x > 0 and overlap_y > 0 else 0.0
    
    @staticmethod
    def polygon_point_collision_2d(vertices: List[Vector2], point: Vector2) -> bool:
        """
        Check if a point is inside a convex polygon using ray casting.
        
        Args:
            vertices: List of polygon vertices in order
            point: Point to test
        """
        n = len(vertices)
        inside = False
        
        j = n - 1
        for i in range(n):
            vi = vertices[i]
            vj = vertices[j]
            
            if ((vi.y > point.y) != (vj.y > point.y)) and \
               (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y + 1e-10) + vi.x):
                inside = not inside
            
            j = i
        
        return inside
