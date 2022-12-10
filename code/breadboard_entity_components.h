#pragma once
#include "entity_components_backend.h"

#include "math/affine_transformations.h"
#include "bitmap.h"

#include "Assets.h"
#include "breadboard_tile.h"
#include "data_containers.h"
#include "breadboard_components.h"
#include "math/rect2f.h"


// Todo, Can we use a location component instead of DeltaRot, DeltaPos etc?
struct component_camera
{
  r32 AngleOfView;
  r32 AspectRatio;
  r32 OrthoZoom; // Just used in orthographic projection to control how much of the screen is rendered
  m4  DeltaRot;
  v3  DeltaPos;
  m4  V;
  m4  P;
};

struct game_controller_input;
enum ControllerType
{
  ControllerType_Hero,
  ControllerType_FlyingCamera,
};

struct component_controller
{
  keyboard_input* Keyboard;
  mouse_input* Mouse;
  game_controller_input* Controller;
  u32 Type;
};

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
  r32 Rotation;
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
  r32 CenterPoint; // Base center point. [0 (All the way to the left), 1 (all the way to the right)] 
  r32 Height;
  r32 Rotation;
};

struct component_hitbox
{
  HitboxType Type;
  world_coordinate Position;
  union
  {
    hitbox_circle Circle;
    hitbox_rectangle Rectangle;
    hitbox_triangle Triangle;
  };
};


void getTriangle2DPointsCentroidAtOrigin(hitbox_triangle* HitboxTriangle, v2* A, v2* B, v2* C)
{
  *A = V2(-HitboxTriangle->Base * HitboxTriangle->CenterPoint, 0);
  *B = V2( HitboxTriangle->Base * (1.f-HitboxTriangle->CenterPoint), 0);
  *C = V2(0, HitboxTriangle->Height);
  v2 Centroid = GetTriangleCentroid(*A,*B,*C);
  *A -= Centroid;
  *B -= Centroid;
  *C -= Centroid;
}

bool Intersects(component_hitbox* HitboxComponent, world_coordinate IntersectionPoint)
{
  b32 Result = false;
  switch(HitboxComponent->Type)
  {
    case HitboxType::CIRCLE:
    {
      r32 Radius = Norm(IntersectionPoint - HitboxComponent->Position);
      Result = Radius < HitboxComponent->Circle.Radius;
    }break;
    case HitboxType::RECTANGLE:
    {
      // If the roatation angle of the htibox is off center, this is the vector from the center 
      // of the hitbox to the center of rotation, if it's zero, it means the rotation point is equal to 
      // the geometric center.
      world_coordinate RotationCenterOffset = {};

      hitbox_rectangle HitboxRect = HitboxComponent->Rectangle;

      // Translate IntersectionPoint to have origin at the rectangle
      world_coordinate PointToTest = IntersectionPoint - HitboxComponent->Position;
      
      // Rotate the point we want to test "backwards" instead of rotating the hitbox "Forward"
      r32 Rotation = -HitboxRect.Rotation;
      v3 RotatedPointToTest = M3(Cos(Rotation), -Sin(Rotation), 0,
                                 Sin(Rotation),  Cos(Rotation), 0,
                                 0,              0,             1) * PointToTest;
      // LowerLeft
      v3 A = V3(-HitboxRect.Width, -HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;
      // LowerRight
      v3 B = V3( HitboxRect.Width, -HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;
      // UpperRight
      v3 C = V3( HitboxRect.Width,  HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;
      // UpperLeft
      v3 D = V3(-HitboxRect.Width,  HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;

      b32 InLowerLeftTriangle  = IsVertexInsideTriangle(RotatedPointToTest, V3(0, 0, 1), A, B, D);
      b32 InUpperRightTriangle = IsVertexInsideTriangle(RotatedPointToTest, V3(0, 0, 1), B, C, D);
      
      Result = InLowerLeftTriangle || InUpperRightTriangle;
  
    }break;
    case HitboxType::TRIANGLE:
    {
      // If the roatation angle of the htibox is off center, this is the vector from the center 
      // of the hitbox to the center of rotation, if it's zero, it means the rotation point is equal to 
      // the geometric center.
      world_coordinate RotationCenterOffset = {};

      hitbox_triangle HitboxTriangle = HitboxComponent->Triangle;

      v2 A,B,C;
      getTriangle2DPointsCentroidAtOrigin(&HitboxTriangle, &A,&B,&C);
      A -= V2(RotationCenterOffset);
      B -= V2(RotationCenterOffset);
      C -= V2(RotationCenterOffset);

      // Translate IntersectionPoint to have origin at the rectangle
      world_coordinate PointToTest = IntersectionPoint - (HitboxComponent->Position);

      // Rotate the point we want to test "backwards" instead of rotating the hitbox "Forward"
      r32 Rotation = -HitboxTriangle.Rotation;
      v3 RotatedPointToTest = M3(Cos(Rotation), -Sin(Rotation), 0,
                                 Sin(Rotation),  Cos(Rotation), 0,
                                 0,              0,             1) * PointToTest;
      
      v3 TriangleNormal = V3(0, 0, 1);
      Result  = IsVertexInsideTriangle(RotatedPointToTest, TriangleNormal, V3(A,0), V3(B,0), V3(C,0));
    }break;
  }

  return Result;
}

#if 0

inline b32 Intersects_XYPlane(hitbox* Hitbox, world_coordinate* P)
{
  rect2f Rect = HitboxToRect(Hitbox);

  // Translate unrotated hitbox to center
  world_coordinate Pos = Hitbox->Pos;
  v3 PointToTest = V3(P->X - Pos.X, P->Y - Pos.Y, 0);
  
  // Rotate the point we want to test "backwards" instead of rotating the hitbox "Forward"
  r32 Rotation = -Hitbox->Rotation;
  v3 RotatedPointToTest = M3(Cos(Rotation), -Sin(Rotation), 0,
                             Sin(Rotation),  Cos(Rotation), 0,
                             0,              0,             1) * PointToTest;
  // LowerLeft
  v3 A = V3(-Hitbox->W, -Hitbox->H, 0) * 0.5f - V3(Hitbox->RotationCenterOffset,0);
  // LowerRight
  v3 B = V3( Hitbox->W, -Hitbox->H, 0) * 0.5f - V3(Hitbox->RotationCenterOffset,0);
  // UpperRight
  v3 C = V3( Hitbox->W,  Hitbox->H, 0) * 0.5f - V3(Hitbox->RotationCenterOffset,0);
  // UpperLeft
  v3 D = V3(-Hitbox->W,  Hitbox->H, 0) * 0.5f - V3(Hitbox->RotationCenterOffset,0);

  b32 InLowerLeftTriangle  = IsVertexInsideTriangle(RotatedPointToTest, V3(0, 0, 1), A, B, D);
  b32 InUpperRightTriangle = IsVertexInsideTriangle(RotatedPointToTest, V3(0, 0, 1), B, C, D);
  b32 Result = InLowerLeftTriangle || InUpperRightTriangle;
  
  return InLowerLeftTriangle || InUpperRightTriangle;
}
#endif

struct component_render
{
  object_handle Object;
  material_handle Material;
  bitmap_handle Bitmap;
};

// TODO: Move to AssetManager.
// A sprite animation is just a static lookup table setup at start.
// Perfect for AssetManager to deal with.
// This component can still exist but only keeps a handle to the active subframe.
struct component_sprite_animation
{
  hash_map< list <m4> > Animation;
  list<m4>* ActiveSeries;
  b32 InvertX;
};

enum component_type
{
  COMPONENT_FLAG_NONE               = 0,
  COMPONENT_FLAG_CAMERA             = 1<<0,
  COMPONENT_FLAG_CONTROLLER         = 1<<1,
  COMPONENT_FLAG_RENDER             = 1<<2,
  COMPONENT_FLAG_SPRITE_ANIMATION   = 1<<3,
  COMPONENT_FLAG_HITBOX             = 1<<4,
  COMPONENT_FLAG_ELECTRICAL         = 1<<5,
  COMPONENT_FLAG_CONNECTOR_PIN      = 1<<6,
  COMPONENT_FLAG_FINAL              = 1<<7,
};

// Initiates the backend entity manager with breadboard specific components
entity_manager* CreateEntityManager();

#define GetCameraComponent(EntityID) ((component_camera*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CAMERA))
#define GetControllerComponent(EntityID) ((component_controller*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CONTROLLER))
#define GetRenderComponent(EntityID) ((component_render*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_RENDER))
#define GetSpriteAnimationComponent(EntityID) ((component_sprite_animation*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_SPRITE_ANIMATION))
#define GetHitboxComponent(EntityID) ((component_hitbox*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_HITBOX))
#define GetElectricalComponent(EntityID) ((component_electrical*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_ELECTRICAL))
#define GetConnectorPinComponent(EntityID) ((component_connector_pin*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CONNECTOR_PIN))