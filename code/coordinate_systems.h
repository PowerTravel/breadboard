#pragma once 

#pragma once

#include "types.h"
#include "math/rect2f.h"
#include "math/vector_math.h"

/*
 * --== Types of screen coordinate systems ==--
 *   AR = AspectRatio = Width/Height
 *   W_px = Width in pixels
 *   H_px = Height in pixels
 *   NDC = Normalized Device Coordinates
 * Canonical Space A:                 Canonical Space B:                
 * (1,0)                     (AR,1)   (0,0)                     (AR, 0) 
 * ______________________________     ______________________________    
 * |                            |     |                            |    
 * |                            |     |                            |    
 * |                            |     |                            |    
 * |                            |     |                            |    
 * |                            |     |                            |    
 * |____________________________|     |____________________________|    
 * (0,0)                     (AR, 0)  (0,1)                     (AR, 1) 
 *
 * Screen Space                          RasterSpace:                      
 * (The world seen from the Camera       
 *  view matrix, Same as NDC)            
 * (-1,1)                     (1,1)      (0,0)                  (W_px,0)
 *   ______________________________      ______________________________
 *   |                            |      |                            |
 *   |                            |      |                            |
 *   |                            |      |                            |
 *   |                            |      |                            |
 *   |                            |      |                            |
 *   |____________________________|      |____________________________|
 * (-1,-1)                     (-1, 1)   (0,H_px)             (W_px, H_px)
 *
 */

// Canonical Space A: Origin bot left
// Note: Screen space with Origin top left of the window.
// [X,      Y] -> [X,        Y]
// [Left, Bot] -> [Right,  Top]
// [0,      0] -> [AR,       1]
// TODO: Rename canonical_coordinate_2d_rh ? 
struct canonical_screen_coordinate_a
{
  r32 X;
  r32 Y;
  r32 AspectRatio;
};

typedef canonical_screen_coordinate_a canonical_screen_coordinate;

// Canonical Space:
// Note: Screen space with Origin top left of the window.
// [X,      Y] -> [X,        Y]
// [Left, Top] -> [Right,  Bot]
// [0,      0] -> [AR,       1]
// TODO: Rename canonical_coordinate_2d_lh ? 
struct canonical_screen_coordinate_b
{
  r32 X;
  r32 Y;
  r32 AspectRatio;
};


canonical_screen_coordinate CanonicalScreenCoordinate(r32 X, r32 Y, r32 AspectRatio)
{
  canonical_screen_coordinate Result = {};
  Result.X = X;
  Result.Y = Y;
  Result.AspectRatio = AspectRatio;
  return Result;
}

// Raster Space:
// Note: Screen space with Origin top left of the window. Uses Pixels.
// [X,      Y] -> [X,        Y]
// [Left, Top] -> [Right,  Bot]
// [0,      0] -> [W_px,  H_px]
struct raster_screen_coordinate
{
  s32 X;
  s32 Y;
  s32 Width;
  s32 Height;
};

// Screen Space: (Origin bot left)
// [X,      Y] -> [X,        Y]
// [Left, Bot] -> [Right,  Top]
// [-1,    -1] -> [1,        1]
typedef v2 screen_coordinate;


screen_coordinate ScreenCoordinate(r32 X, r32 Y)
{
  screen_coordinate Result = {};
  Result.X = X;
  Result.Y = Y;
  return Result;
}

rect2f GetCameraScreenRect(r32 Zoom, r32 AspectRatio)
{
  const r32 Right = Zoom*AspectRatio;
  const r32 Left  = -Right;
  const r32 Top   = Zoom;
  const r32 Bot   = -Top;
  rect2f Result = Rect2f(Left, Bot, Right-Left, Top-Bot);
  return Result;
}


/* 
 * --== World Coordinate system ==--
 *           World Coordinate
 *                              
 *      |        |        |      
 * __(-1, 1)__( 0, 1)__( 1, 1)___
 *      |        |        |      
 *      |        |        |      
 * __(-1, 0)__( 0, 0)__( 1, 0)___
 *      |        |        |      
 *      |        |        |      
 * __(-1,-1)__( 0,-1)__( 1,-1)___
 *      |        |        |      
 *      |        |        |      
 *
 */

// [Left,          Top,    OutOfScreen] -> [Right,   Bot,     IntoScreen]
// [-R32_MAX, -R32_MAX,       -R32_MAX] -> [R32_MAX, R32_MAX,    R32_MAX]
typedef v3 world_coordinate;

inline world_coordinate WorldCoordinate(r32 X, r32 Y, r32 Z)
{
  world_coordinate Result = {};
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  return Result;
}

inline v3 ToV3(world_coordinate* r)
{
  v3 Result = V3(r->X, r->Y, r->Z);
  return Result;
}


inline screen_coordinate CanonicalToScreenSpace(canonical_screen_coordinate CanPos)
{
  screen_coordinate Result{};
  Result.X = (2*CanPos.X/CanPos.AspectRatio - 1);
  Result.Y = (2*CanPos.Y - 1);
  return Result;
}



// Result coordinate has origin in the center of the screen
world_coordinate ToWorldCoordinateRelativeCameraPosition(screen_coordinate ScreenCoordinate, r32 Zoom, r32 AspectRatio)
{
  rect2f ScreenRect = GetCameraScreenRect(Zoom, AspectRatio);
  world_coordinate Result = {};
  Result.X = 0.5f * ScreenCoordinate.X * ScreenRect.W;
  Result.Y = 0.5f * ScreenCoordinate.Y * ScreenRect.H;
  return Result;
}

// Result coordinate is relative world origin
world_coordinate ToWorldCoordinate(screen_coordinate ScreenCoordinate, world_coordinate CameraPosition, r32 OrthoZoom, r32 AspectRatio)
{ 
  world_coordinate PosRelativeCamera = ToWorldCoordinateRelativeCameraPosition(ScreenCoordinate, OrthoZoom, AspectRatio);
  world_coordinate PosRelativeOrigin = PosRelativeCamera + CameraPosition;
  return PosRelativeOrigin;
}
