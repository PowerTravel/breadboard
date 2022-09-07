#pragma once

#include "types.h"
#include "coordinate_systems.h"


#define AssertBitmapPointLh(Point) \
  Assert(Point.x >= 0);            \
  Assert(Point.y >= 0);            \
  Assert(Point.x <= Point.w);      \
  Assert(Point.y <= Point.h);
  

#define AssertBitmapPairLh(TopLeft, BotRight) \
  Assert(TopLeft.w == BotRight.w);            \
  Assert(TopLeft.h == BotRight.h);            \
  Assert(TopLeft.x <= BotRight.x);            \
  Assert(TopLeft.y <= BotRight.y);            \
  AssertBitmapPointLh(TopLeft);               \
  AssertBitmapPointLh(BotRight);




/*
 *  --== Pixel Space Left Hand oriented ==--
 *  Note: 
 *   - Origin Top Left Y Down
 *   - Unit is Pixels
 *   - The native coordinate system of raw image files unless otherwise specified
 *  (0,0)                   (W_px,0)                       
 *  ______________________________                         
 *  |                            |
 *  |             |              |
 *  |             | Y            |
 *  |            \|/             |
 *  |                            |
 *  |____________________________|
 *  (0,H_px)             (W_px, H_px)
 * 
 * [Left, Top] -> [Right,  Bot]
 * [0,      0] -> [W_px,  H_px]
 */

struct bitmap_coordinate_lh
{
  u32 x, y, w, h;
};

typedef bitmap_coordinate_lh bitmap_coordinate;

inline bitmap_coordinate_lh BitmapCoordinatePxLh(u32 x, u32 y, u32 w, u32 h)
{
  bitmap_coordinate_lh Result = {};
  Result.x = x;
  Result.y = y;
  Result.w = w;
  Result.h = h;
  AssertBitmapPointLh(Result);
  return Result;
}

// Raw Bitmap Points always has it's origin in Top Left with Y going Down
union bitmap_points
{
  bitmap_coordinate_lh P[7];
  struct {
    bitmap_coordinate_lh TopLeft;       // Top Left Point
    bitmap_coordinate_lh BotRight;      // Botom Right Point
    bitmap_coordinate_lh Center;        // Rotates around this point
    bitmap_coordinate_lh Points[4];     // Up to 4 points for various uses
  };
};

#if 0 
Not used by us, bitmap coordinates always is left handed. Origin top left, y down
/*                   
 *  --== Pixel Space Right Hand oriented ==--
 *  Note:
 *   - Origin Bot left, Y Up
 *   - Unit is Pixels
 *   - The native coordinate system when treating bitmaps and textures inside the application
 *     IE: When loaded from file they use LH, But renderer expects RH
 *  (0,H_px)                (W_px,H_px)   
 *  ______________________________
 *  |                            |
 *  |                            |
 *  |            /|\             |
 *  |             | Y            |
 *  |             |              |
 *  |                            |
 *  |____________________________|
 *  (0,0)                   (W_px, 0)                 
 *                                        
 * Pixel Space Right Hand
 * [Left, Bot] -> [Right,  Top]
 * [0,      0] -> [W_px,  H_px]
*/
struct bitmap_coordinate_rh
{
  u32 x, y, w, h;
};

inline bitmap_coordinate_rh BitmapCoordinatePxRh(u32 x, u32 y, u32 w, u32 h)
{
  bitmap_coordinate_rh Result = {x,y,w,h};
  return Result;
}
#endif

#if 0 
Not used in OpenGl so maybe we don't even need it
/*
 *  --== UV Space Left Hand oriented ==--
 *  Note: 
 *   - Origin top left, Y Down
 *   - UV is normalized texture coordinates.
 *   - Unitless
 *   - OpenGL uses RH for UV coordinates (not this).
 *  (0,0)                     (1,0)                        
 *  ______________________________                         
 *  |                            |
 *  |                            |
 *  |             |              |
 *  |             | Y            |
 *  |            \|/             |
 *  |                            |
 *  |____________________________|
 *  (0,1)                     (1,1) 
 *                                      
 *  UV Space A (Origin top left)
 *  [Left, Top] -> [Right,  Bot]
 *  [0,      0] -> [1,        1]
 */
struct uv_coordinate_lh
{
  r32 u, v;
};

inline uv_coordinate_lh UVCoordinateLh(r32 u, r32 v)
{
  uv_coordinate_lh Result = {u,v};
  return Result;
}
#endif

/*
 *  --== UV Space Right Hand oriented ==--
 *  Note:
 *   - Origin bot left, Y Up
 *   - UV is normalized texture coordinates.
 *   - Unitless
 *   - This orientation is what OpenGL uses UV coordinates.
 *  (0,1)                     (1,1)
 *  ______________________________
 *  |                            |
 *  |                            |
 *  |            /|\             |
 *  |             | Y            |
 *  |             |              |
 *  |                            |
 *  |____________________________|
 *  (0,0)                     (1,0)
 *                                      
 * UV Space B (Origin bot left)
 * [Left, Bot] -> [Right,  Top]
 * [0,      0] -> [1,        1]
 *
 */

struct uv_coordinate_rh
{
  r32 u, v;
};

inline uv_coordinate_rh UVCoordinateRh(r32 u, r32 v)
{
  uv_coordinate_rh Result = {u,v};
  return Result;
}

internal inline bitmap_coordinate_lh GetRelativeCenter(bitmap_coordinate_lh TopLeft, bitmap_coordinate_lh BotRight)
{
  AssertBitmapPairLh(TopLeft, BotRight);
  u32 HeightPx = TopLeft.h;
  u32 WidthPx  = TopLeft.w;

  bitmap_coordinate_lh Result = BitmapCoordinatePxLh(
    (u32)((BotRight.x - TopLeft.x) * 0.5f),
    (u32)((BotRight.y - TopLeft.y) * 0.5f),
    HeightPx, WidthPx);
  return Result;
}

internal inline bitmap_coordinate_lh GetAbsoluteCenter(bitmap_coordinate_lh TopLeft, bitmap_coordinate_lh BotRight)
{
  AssertBitmapPairLh(TopLeft, BotRight);
  u32 HeightPx = TopLeft.h;
  u32 WidthPx  = TopLeft.w;

  bitmap_coordinate_lh Result = BitmapCoordinatePxLh(
    (u32)((BotRight.x + TopLeft.x) * 0.5f),
    (u32)((BotRight.y + TopLeft.y) * 0.5f),
    HeightPx, WidthPx);
  return Result;
}

rect2f GetTextureRectRh(bitmap_coordinate_lh const & TopLeft, bitmap_coordinate_lh const & BotRight)
{
  AssertBitmapPairLh(TopLeft, BotRight);

  u32 HeightInPixels = BotRight.h;
  // From here on out we are origin bot left
  rect2f RectInPixels = {};
  RectInPixels.X = (r32) TopLeft.x;
  r32 X1_Pixels  = (r32) BotRight.x;
  RectInPixels.Y = (r32) (HeightInPixels - BotRight.y);
  r32 Y1_Pixels  = (r32) (HeightInPixels - TopLeft.y);

  RectInPixels.W = X1_Pixels-RectInPixels.X;
  RectInPixels.H = Y1_Pixels-RectInPixels.Y;
  return RectInPixels;
}

rect2f GetUVRectRh(bitmap_coordinate_lh const & TopLeft, bitmap_coordinate_lh const & BotRight)
{ 
  r32 WidthInPixels  = (r32) BotRight.w;
  r32 HeightInPixels = (r32) BotRight.h;

  rect2f RectInPixels = GetTextureRectRh(TopLeft, BotRight);
  // We want the bitmap_coordinate to fall in the middle of the pixel so that it's clamped to the correct edge
  RectInPixels.X += 0.5f;
  RectInPixels.Y += 0.5f;
  RectInPixels.W -= 1.f;
  RectInPixels.H -= 1.f;

  rect2f UVRect = {};
  UVRect.X = RectInPixels.X / WidthInPixels;
  UVRect.Y = RectInPixels.Y / HeightInPixels;
  UVRect.W = RectInPixels.W / WidthInPixels;  // Scale W
  UVRect.H = RectInPixels.H / HeightInPixels; // Scale H
  return UVRect;
}


enum ColorDecodeMask
{
  BITMAP_MASK_ALPHA  = 0xff000000,
  BITMAP_MASK_BLUE   = 0xff0000,
  BITMAP_MASK_GREEN  = 0xff00,
  BITMAP_MASK_RED    = 0xff,
  BITMAP_SHIFT_ALPHA = 24,
  BITMAP_SHIFT_BLUE  = 16,
  BITMAP_SHIFT_GREEN = 8,
  BITMAP_SHIFT_RED   = 0
};

struct bitmap
{
  // A 'Special' bitmap cannot fit into the 3D Texture and takes up its own texture slot
  // A 'Non-Special' bitmap fit into the 3D texture
  // TODO: Should special even be a thing here? Maybe it should be handled soly by the renderer.
  b32   Special;
  u32   BPP;
  u32   Width;
  u32   Height;
  void* Pixels;
};

bool GetPixelValue(bitmap* BitMap, const u32 X,  const u32 Y, u32* Result)
{
  if((X <= BitMap->Width) && (Y <= BitMap->Height))
  {
    *Result = *(( (u32*) BitMap->Pixels ) + BitMap->Width * Y + X);
    return true;
  }
  return false;
}

struct sprite_sheet
{
  bitmap* bitmap;
  u32 EntryCount;
};

void SplitPixelIntoARGBComponents(u32 PixelValue, u8* A, u8* R, u8* G, u8* B)
{
  *A = (u8) ((PixelValue & BITMAP_MASK_ALPHA) >> BITMAP_SHIFT_ALPHA);
  *R = (u8) ((PixelValue & BITMAP_MASK_RED)   >> BITMAP_SHIFT_RED);
  *G = (u8) ((PixelValue & BITMAP_MASK_GREEN) >> BITMAP_SHIFT_GREEN);
  *B = (u8) ((PixelValue & BITMAP_MASK_BLUE)  >> BITMAP_SHIFT_BLUE);
}