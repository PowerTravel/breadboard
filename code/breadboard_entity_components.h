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

struct component_hitbox
{
  rect2f Rect;
  v2 RotationCenter;
  r32 Rotation;
};

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
  COMPONENT_FLAG_FINAL              = 1<<6,
};

// Initiates the backend entity manager with breadboard specific components
entity_manager* CreateEntityManager();

#define GetCameraComponent(EntityID) ((component_camera*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CAMERA))
#define GetControllerComponent(EntityID) ((component_controller*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CONTROLLER))
#define GetRenderComponent(EntityID) ((component_render*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_RENDER))
#define GetSpriteAnimationComponent(EntityID) ((component_sprite_animation*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_SPRITE_ANIMATION))
#define GetHitboxComponent(EntityID) ((component_hitbox*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_HITBOX))
#define GetElectricalComponent(EntityID) ((electrical_component*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_ELECTRICAL))