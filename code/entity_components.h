#pragma once

#include "types.h"

#include "math/affine_transformations.h"

#include "bitmap.h"
#include "data_containers.h"
#include "Assets.h"

struct component_head;

struct entity_component_chunk
{
  u32 Types;
  component_head** Components;

  entity_component_chunk* Next;
};

struct entity
{
  u32 ID; // ID starts at 1. Index is ID-1
  u32 ComponentFlags;
  entity_component_chunk* Components;
};

struct component_head
{
  entity* Entity;
  u32 Type;
};


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
  game_controller_input* Controller;
  u32 Type;
};

struct component_light
{
  v4 Color;
};

struct component_spatial
{
  component_spatial(v3 PosInit = V3(0,0,0), v3 ScaleInit = V3(1,1,1), v4 RotInit = V4(0,0,0,1) ) :
        Position(PosInit), Scale(ScaleInit), Rotation(RotInit){};
  v3 Scale;
  v3 Position;
  //  We define the Quaternion as (xi,yj,zk, Scalar)
  //  Some resources define it as (Scalar,xi,yj,zk)
  v4 Rotation;
  m4 ModelMatrix;
};


inline v3 DirectionToLocal(component_spatial* Spatial, v3 GlobalDirection)
{
  v3 Result = V3(Transpose(AffineInverse(Spatial->ModelMatrix))*V4(GlobalDirection,0));
  return Result;
}

inline v3 ToLocal( component_spatial* Spatial, v3 GlobalPosition )
{
  v3 Result = V3(AffineInverse(Spatial->ModelMatrix)*V4(GlobalPosition,1));
  return Result;
}

inline v3 ToLocal( component_spatial* Spatial)
{
  v3 Result = ToLocal(Spatial, Spatial->Position);
  return Result;
}

inline v3 ToGlobal( component_spatial* Spatial, v3 LocalPosition )
{
  v3 Result = V3(Spatial->ModelMatrix * V4(LocalPosition,1));
  return Result;
}

inline v3 ToGlobal( component_spatial* Spatial )
{
  v3 Result = ToGlobal(Spatial, Spatial->Position);
  return Result;
}


void UpdateModelMatrix( component_spatial* c )
{
  TIMED_FUNCTION();
  c->ModelMatrix = GetModelMatrix(c->Position, c->Rotation, c->Scale);
}

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
  COMPONENT_SPATIAL                 = 1<<4,
  COMPONENT_FLAG_FINAL              = 1<<5,
};

struct em_chunk
{
  midx Used;
  u8* Memory;
  em_chunk* Next;
};

struct component_list
{
  u32 Type;
  u32 Requirement;
  u32 Count;
  midx ComponentSize;
  midx ChunkSize;
  em_chunk* First;
};

component_list CreateComponentList(u32 Flag, midx ComponentSize, midx ChunkComponentCount, u32 Requires)
{
  component_list Result = {};
  Result.Type = Flag;
  Result.Requirement = Requires;
  Result.ComponentSize = sizeof(component_head) + ComponentSize;
  Result.ChunkSize = ChunkComponentCount * Result.ComponentSize;
  return Result;
}

struct entity_manager
{
  memory_arena Arena;
  temporary_memory TemporaryMemory;

  u32 EntityCount;
  u32 EntitiesPerChunk;
  u32 EntityChunkCount;
  em_chunk* EntityList;

  u32 ComponentCount;
  component_list* Components;
};

entity_manager* CreateEntityManager( );
void NewComponents( entity_manager* EM, entity* Entity, u32 EntityFlags );
u32 NewEntity( entity_manager* EM );

void BeginTransactions(entity_manager* EM)
{
  CheckArena(&EM->Arena);
  EM->TemporaryMemory = BeginTemporaryMemory(&EM->Arena);
}

void EndTransactions(entity_manager* EM)
{
  EndTemporaryMemory(EM->TemporaryMemory);
  CheckArena(&EM->Arena);
}

#define BeginScopedEntityManagerMemory() scoped_temporary_memory ScopedMemory_ = scoped_temporary_memory(&(GlobalGameState->EntityManager)->Arena)

struct filtered_components
{
  u32 Count;
  component_head* Heads;
};

struct component_result
{
  entity_manager* EM;
  u32 MainType;
  midx MainTypeSize;
  u32 Types;
  b32 Begun;

  u32 EntityCount;
  u32 ArrayCount;
  u32 ArrayIndex;
  u32 ComponentIndex;
  filtered_components* FilteredArray;
};

component_result* GetComponentsOfType(entity_manager* EM, u32 ComponentFlags);
b32 Next(entity_manager* EM, component_result* ComponentList);

u32 GetEntity( u8* Component )
{
  component_head* Base = (component_head*) (Component - sizeof(component_head));
  return Base->Entity->ID;
}

u8* GetComponent(entity_manager* EM, component_result* ComponentList, u32 ComponentFlag);
u8* GetComponent(entity_manager* EM, u32 EntityID, u32 ComponentFlag);


#define GetCameraComponent(Input) ((component_camera*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_CAMERA))
#define GetControllerComponent(Input) ((component_controller*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_CONTROLLER))
#define GetRenderComponent(Input) ((component_render*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_RENDER))
#define GetSpriteAnimationComponent(Input) ((component_sprite_animation*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_SPRITE_ANIMATION))
#define GetSpatialComponent(Input) ((component_spatial*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_SPATIAL))