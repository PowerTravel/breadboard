#pragma once

#include "platform.h"
#include "debug.h"
#include "memory.h"
#include "bitmap.h"
#include "breadboard_tile.h"
#include "assets.h"
#include "breadboard_entity_components.h"
#include "menu_interface.h"
#include "breadboard_components.h"

#define MAX_ELECTRICAL_IO 32

struct mouse_selector
{
  canonical_screen_coordinate CanPos;
  screen_coordinate ScreenPos;
  world_coordinate WorldPos;
  r32 Rotation;
  tile_map_position TilePos;
  binary_signal_state LeftButton;
  binary_signal_state RightButton;
  tile_contents SelectedContent;
};


struct world
{
  r32 GlobalTimeSec;
  r32 dtForFrame;

  memory_arena* Arena;
  
  tile_map TileMap;
  electrical_component* Source;
  mouse_selector MouseSelector;
};

typedef void(*func_ptr_void)(void);

struct function_ptr
{
  c8* Name;
  func_ptr_void Function;
};

struct function_pool
{
  u32 Count;
  function_ptr Functions[256];
};

// This is to be a ginormous struct where we can set things we wanna access from everywhere.
struct game_state
{
  game_render_commands* RenderCommands;
  
  memory_arena* PersistentArena;
  memory_arena* TransientArena;
  temporary_memory TransientTempMem;
  
  game_asset_manager* AssetManager;
  entity_manager* EntityManager;
  menu_interface* MenuInterface;
  
  game_input* Input;
  
  world* World;
  
  function_pool* FunctionPool;
  
  b32 IsInitialized;
  
  u32 Threads[4];
};

/// Game Global API

game_state* GlobalGameState = 0;
platform_api Platform;

struct game_window_size
{
  r32 WidthPx;
  r32 HeightPx;
  // r32 DesiredAspectRatio;
};

inline game_window_size GameGetWindowSize()
{
  game_window_size Result ={};
  Result.WidthPx  = (r32)GlobalGameState->RenderCommands->ScreenWidthPixels;
  Result.HeightPx = (r32)GlobalGameState->RenderCommands->ScreenHeightPixels;
  return Result;
}

inline r32 GameGetAspectRatio()
{
  game_window_size GameWindowSize = GameGetWindowSize();
  r32 Result = GameWindowSize.WidthPx/GameWindowSize.HeightPx;
  return Result;
}


// Points in Canonical Space will be transformed to ScreenSpace
inline m4 GetCanonicalSpaceProjectionMatrix()
{
  r32 AspectRatio = GameGetAspectRatio();
  // Transforms from canonical space to screenSpace
  m4 ProjectionMatrix = 
       M4( 2/AspectRatio, 0, 0, -1,
                       0, 2, 0, -1,
                       0, 0, 0,  0,
                       0, 0, 0,  1);
  return ProjectionMatrix;
}

inline v2 CanonicalToRasterSpace(v2 Pos)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 OneOverAspectRatio = WindowSize.HeightPx/WindowSize.WidthPx;
  v2 Result{};
  Result.X = Pos.X*OneOverAspectRatio*WindowSize.WidthPx;
  Result.Y = (1-Pos.Y)*WindowSize.HeightPx;
  return Result;
}


inline func_ptr_void* _DeclareFunction(func_ptr_void Function, const c8* Name)
{
  Assert(GlobalGameState);
  function_pool* Pool = GlobalGameState->FunctionPool;
  Assert(Pool->Count < ArrayCount(Pool->Functions))
    function_ptr* Result = Pool->Functions;
  u32 FunctionIndex = 0;
  while(Result->Name && !str::ExactlyEquals(Result->Name, Name))
  {
    Result++;
  }
  if(!Result->Function)
  {
    Assert(Pool->Count == (Result - Pool->Functions))
      Pool->Count++;
    Result->Name = (c8*) PushCopy(GlobalGameState->PersistentArena, (str::StringLength(Name)+1)*sizeof(c8), (void*) Name);
    Result->Function = Function;
  }else{
    Result->Function = Function;
  }
  return &Result->Function;
}

#define DeclareFunction(Type, Name) (Type**) _DeclareFunction((func_ptr_void) (&Name), #Name )
#define CallFunctionPointer(PtrToFunPtr, ... ) (**PtrToFunPtr)(__VA_ARGS__)