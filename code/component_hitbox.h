#pragma once

#include "component_position.h"

enum class HitboxType
{
  RECTANGLE,
  CIRCLE,
  TRIANGLE
};

struct hitbox_circle
{
  r32 Radius;
};

struct hitbox_rectangle
{
  r32 Width;
  r32 Height;
};

struct hitbox_triangle
{

//          /|\
//         / | \
//       /   | \
//   Height->|  \
//     /     |   \  
//   /_______|____\
//   Base    |    
//      CenterPoint

  r32 Base;
  r32 Height;
  r32 CenterPoint; // Base center point. [0 (All the way to the left), 1 (all the way to the right)] 
};

struct component_hitbox
{
  HitboxType Type;
  position_node* Position;
  union
  {
    hitbox_circle Circle;
    hitbox_rectangle Rectangle;
    hitbox_triangle Triangle;
  };
};

void InitiateTriangleHitboxComponent(component_hitbox* Hitbox, position_node* PositionNode, r32 Base, r32 Height, r32 CenterPoint);
void InitiateRectangleHitboxComponent(component_hitbox* Hitbox, position_node* PositionNode, r32 Width, r32 Height);
void InitiateCircleHitboxComponent(component_hitbox* Hitbox, position_node* PositionNode, r32 Radius);
bool Intersects(component_hitbox* HitboxComponent, world_coordinate IntersectionPoint);