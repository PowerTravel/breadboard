#pragma once

#include "platform.h"
#include "debug.h"
#include "memory.h"
#include "bitmap.h"
#include "breadboard_tile.h"
#include "assets.h"
#include "entity_components.h"
#include "menu_interface.h"

struct picked_entity
{
  b32 Active;
  u32 EntityID;
  v3 Point;
  v3 PointObjectSpace;
  v3 MousePointOnPlane;
};

struct world
{
  r32 GlobalTimeSec;
  r32 dtForFrame;

  memory_arena* Arena;
  

  hash_map<bitmap_coordinate> BreadboardTilesheet;
  tile_map TileMap;
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

/* Types of screen coordinate systems,
 *   AR = AspectRatio = Width/Height
 *   W_px = Width in pixels
 *   H_px = Height in pixels
 *   NDC = Normalized Device Coordinates
 * Canonical Space:                      RasterSpace:
 * (1,0)                     (AR,1)      (0,0)                  (W_px,0)
 * ______________________________        ______________________________
 * |                            |        |                            |
 * |                            |        |                            |
 * |                            |        |                            |
 * |                            |        |                            |
 * |                            |        |                            |
 * |____________________________|        |____________________________|
 * (0,0)                     (AR, 0)     (0,H_px)             (W_px, H_px)
 *
 *  NDC Space                            Screen Space (The world seen from the Camera view matrix)
 * (0,0)                     (1,0)      (-1,1)                     (1,1)
 *  ______________________________        ______________________________
 *  |                            |        |                            |
 *  |                            |        |                            |
 *  |                            |        |                            |
 *  |                            |        |                            |
 *  |                            |        |                            |
 *  |____________________________|        |____________________________|
 *  (0,1)                     (1,1)     (-1,-1)                     (-1, 1)
 */

// Points in Canonical Space will be transformed to ScreenSpace
inline m4 GetCanonicalSpaceProjectionMatrix()
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;

  m4 ScreenToCubeScale =  M4( 2/AspectRatio, 0, 0, 0,
                                           0, 2, 0, 0,
                                           0, 0, 0, 0,
                                           0, 0, 0, 1);
  m4 ScreenToCubeTrans =  M4( 1, 0, 0, -1,
                              0, 1, 0, -1,
                              0, 0, 1,  0,
                              0, 0, 0,  1);
  m4 ProjectionMatrix = ScreenToCubeTrans*ScreenToCubeScale;
  return ProjectionMatrix;
}

inline v2 CanonicalToScreenSpace(v2 Pos)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 OneOverAspectRatio = WindowSize.HeightPx/WindowSize.WidthPx;
  v2 Result{};
  Result.X = (2*Pos.X*OneOverAspectRatio - 1);
  Result.Y = (2*Pos.Y - 1);
  return Result;
}
inline v2 CanonicalToNDCSpace(v2 Pos)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 OneOverAspectRatio = WindowSize.HeightPx/WindowSize.WidthPx;
  v2 Result{};
  Result.X = Pos.X*OneOverAspectRatio;
  Result.Y = 1-Pos.Y;
  return Result;
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