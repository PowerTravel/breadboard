#pragma once 

#include "types.h"

/*
 * --== Types of screen coordinate systems ==--
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
 * Screen Space (The world seen from the Camera view matrix, Same as NDC)
 * (-1,1)                     (1,1)
 *   ______________________________
 *   |                            |
 *   |                            |
 *   |                            |
 *   |                            |
 *   |                            |
 *   |____________________________|
 * (-1,-1)                     (-1, 1)
 *
 */

// Canonical Space:
// Note: Screen space with Origin top left of the window.
// [X,      Y] -> [X,        Y]
// [Left, Top] -> [Right,  Bot]
// [0,      0] -> [AR,       1]
struct canonical_screen_coordinate
{
  r32 X;
  r32 Y;
  r32 AspectRatio;
};

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
struct screen_coordinate
{
  r32 X;
  r32 Y;
};



/* 
 * --== World Coordinate system ==--
 *        Cell Centered                                    Grid Centered
 *
 * (-1, 2) | ( 0, 2) | ( 1, 2) | ( 2, 2)              |        |        |      
 * ________|_________|_________|________         __(-1, 1)__( 0, 1)__( 1, 1)___
 *         |         |         |                      |        |        |      
 * (-1, 1) | ( 0, 1) | ( 1, 1) | ( 2, 1)              |        |        |      
 * ________|_________|_________|________         __(-1, 0)__( 0, 0)__( 1, 0)___
 *         |         |         |                      |        |        |      
 * (-1, 0) | ( 0, 0) | ( 1, 0) | ( 2, 0)              |        |        |      
 * ________|_________|_________|________         __(-1,-1)__( 0,-1)__( 1,-1)___
 *         |         |         |                      |        |        |      
 * (-1,-1) | ( 0,-1) | ( 1,-1) | ( 2,-1)              |        |        |      
 *
 */

// Cell Centered
// [Left,          Bot] -> [Right,        Top]
// [-R32_MAX, -R32_MAX] -> [R32_MAX,  R32_MAX]
struct world_coordinate_2d_cc
{
  r32 X;
  r32 Y;
};

// Grid Centered
// [Left,          Top] -> [Right,        Bot]
// [-R32_MAX, -R32_MAX] -> [R32_MAX,  R32_MAX]
struct world_coordinate_2d_gc
{
  r32 X;
  r32 Y;
};
