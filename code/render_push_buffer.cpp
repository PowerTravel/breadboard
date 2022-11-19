#include "render_push_buffer.h"
#include "component_camera.h"
#include "breadboard_tile.h"
#include "breadboard_components.h"
#include "random.h"

// TODO: Move to settings
#define DRAW_HITBOX_AND_POINTS 0

#define GetBody(Header, Type) ((Type*) (((push_buffer_header*) Header ) + 1))


// Rect has its origin in the bottom left.
// Our geometry has its center at the origin.
// Therefore we need to translate rects before rendering them.
void Recenter(rect2f* Rect)
{
  Rect->X += Rect->W*0.5f;
  Rect->Y += Rect->H*0.5f;
}

u32 RenderTypeToBodySize(render_buffer_entry_type Type)
{
  switch(Type)
  {
    case render_buffer_entry_type::NEW_LEVEL: return 0;
    case render_buffer_entry_type::QUAD_2D: return sizeof(entry_type_2d_quad); 
    case render_buffer_entry_type::QUAD_2D_SPECIAL: return sizeof(entry_type_2d_quad); 
    case render_buffer_entry_type::QUAD_2D_COLOR: return sizeof(entry_type_2d_quad);
  }
  Assert(0);
  return 0;
}

push_buffer_header* PushNewEntry(render_group* RenderGroup, render_buffer_entry_type Type)
{
  RenderGroup->ElementCount++;
  // Note: Header and body needs to be allocated in one call to ensure they end up on the same block.
  //       If the header is in another memory block as the header they won't be located togeather
  //       in memory.
  //       Alternate solution is to let the header carry a pointer to the body.
  u32 BodySize = RenderTypeToBodySize(Type);
  push_buffer_header* NewEntryHeader = (push_buffer_header*) PushSize(&RenderGroup->Arena, sizeof(push_buffer_header) + BodySize);
  NewEntryHeader->Next = 0;

  if(!RenderGroup->First)
  {
    RenderGroup->First = NewEntryHeader;
    RenderGroup->Last  = NewEntryHeader;
  }else{
    RenderGroup->Last->Next = NewEntryHeader;
    RenderGroup->Last = NewEntryHeader;
  }
  NewEntryHeader->Type = Type;
  RenderGroup->BufferCounts[(u32) Type]++;

  return NewEntryHeader;
}

void PushNewRenderLevel(render_group* RenderGroup)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::NEW_LEVEL);
}


// The rect is drawn with the lower left corner at X and Y of Quadrect. W and H extend right and up.
//  _________
//  |       |
//  |       H
//  |___W___|
// X,Y

void Push2DColoredQuad(render_group* RenderGroup, rect2f QuadRect, v4 Color, r32 Rotation, v2 RotationCenterOffset)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D_COLOR);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->Colour = Color;
  Body->QuadRect = QuadRect;
  Body->Rotation = Rotation;
  Body->RotationCenterOffset = RotationCenterOffset;
  Recenter(&Body->QuadRect);
}

void Push2DQuadSpecial(render_group* RenderGroup, rect2f QuadRect, float Rotation, v2 RotationCenterOffset, rect2f UVRect, v4 Color, bitmap_handle BitmapHandle)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D_SPECIAL);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->UVRect = UVRect;
  Body->QuadRect = QuadRect;
  Body->BitmapHandle = BitmapHandle;
  Body->Colour = Color;
  Body->Rotation = Rotation;
  Body->RotationCenterOffset = RotationCenterOffset;
  Recenter(&Body->QuadRect);
}


void Push2DQuad(render_group* RenderGroup, rect2f QuadRect, float Rotation, rect2f UVRect, v4 Color, bitmap_handle BitmapHandle)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->UVRect = UVRect;
  Body->QuadRect = QuadRect;
  Body->BitmapHandle = BitmapHandle;
  Body->Colour = Color;
  Body->Rotation = Rotation;
  Recenter(&Body->QuadRect);
}

void PushOverlayQuad(rect2f QuadRect, v4 Color)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  Push2DColoredQuad(RenderGroup, QuadRect, Color, 0, V2(0,0));
}

void PushTexturedOverlayQuad(rect2f QuadRect,  rect2f UVRect,  bitmap_handle BitmapHandle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  Push2DQuad(RenderGroup, QuadRect, 0, UVRect, V4(1,1,1,1), BitmapHandle);
}

r32 GetTextLineHeightSize(u32 FontSize)
{
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  game_window_size WindowSize = GameGetWindowSize();
  r32 Result = FontMap->FontHeightPx/ (r32) WindowSize.HeightPx;
  return Result;
}

r32 GetTextWidth(const c8* String, u32 FontSize)
{
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);

  game_window_size WindowSize = GameGetWindowSize();
  const r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 Width = 0;
  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    Width += CH->xadvance;
    ++String;
  }
  r32 Result = PixelSize*Width;
  return Result;
}

rect2f GetTextSize(r32 x, r32 y, const c8* String, u32 FontSize)
{
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);

  game_window_size WindowSize = GameGetWindowSize();
  const r32 ScreenScaleFactor = 1.f / WindowSize.HeightPx;

  r32 Width = 0;
  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    Width += CH->xadvance;
    ++String;
  }
  rect2f Result = {};
  Result.X = x;
  Result.Y = y+ScreenScaleFactor*FontMap->Descent;
  Result.H = ScreenScaleFactor*FontMap->FontHeightPx;
  Result.W = ScreenScaleFactor*Width;
  return Result;
}


// The thing about STB is that their bit map has its origin in the top-left
// Our standard is that the origin is in the bottom left
inline internal rect2f
GetSTBBitMapTextureCoords(stbtt_bakedchar* CH, r32 WidthScale, r32 HeightScale)
{
  // This transforms from stbtt coordinates to Texture Coordinates
  // stbtt Bitmap Coordinates:                      Screen Canonical Coodinates
  // [0,0](top left):[PixelX, PixelY](bot right) -> [0,0](bot left):[1,1](top right)
  const r32 s0 = CH->x0 * WidthScale;
  const r32 s1 = CH->x1 * WidthScale;
  const r32 Width  = s1-s0;

  const r32 t0 = CH->y1 * HeightScale;
  const r32 t1 = CH->y0 * HeightScale;
  const r32 Height = t1-t0;

  rect2f Result = Rect2f(s0, t0, Width, Height);
  return Result;
}

// The thing about STB is that their bit map has its origin in the top-left
// Our standard is that the origin is in the bottom left
// The output is the translation needed to get a Quad properly centered with the
// base-point at x0, y0
inline internal rect2f
GetSTBGlyphRect(r32 xPosPx, r32 yPosPx, stbtt_bakedchar* CH )
{
  const r32 GlyphWidth   =  (r32)(CH->x1 - CH->x0); // Width of the symbol
  const r32 GlyphHeight  =  (r32)(CH->y1 - CH->y0); // Height of the symbol
  const r32 GlyphOffsetX =  CH->xoff;               // Distance from Left to BasepointX
  const r32 GlyphOffsetY = -CH->yoff;               // Distance from Top to BasepointY

  const r32 X = Floor(xPosPx + 0.5f) + GlyphOffsetX;
  const r32 Y = Floor(yPosPx + 0.5f) - GlyphHeight + GlyphOffsetY;
  rect2f Result = Rect2f(X,Y, GlyphWidth, GlyphHeight);
  return Result;
}

void PushTextAt(r32 CanPosX, r32 CanPosY, const c8* String, u32 FontSize, v4 Color)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelPosX = Floor(CanPosX*WindowSize.HeightPx);
  r32 PixelPosY = Floor(CanPosY*WindowSize.HeightPx);
  game_asset_manager* AssetManager =  GlobalGameState->AssetManager;
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);

  bitmap* BitMap = GetAsset(AssetManager, FontMap->BitmapHandle);
  stbtt_aligned_quad Quad  = {};

  const r32 ScreenScaleFactor = 1.f / WindowSize.HeightPx;

  const r32 Ks = 1.f / BitMap->Width;
  const r32 Kt = 1.f / BitMap->Height;

  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    if(*String != ' ')
    {
      rect2f TextureRect = GetSTBBitMapTextureCoords(CH, Ks, Kt);
      rect2f GlyphOffset = GetSTBGlyphRect(PixelPosX,PixelPosY,CH);
      GlyphOffset.X *= ScreenScaleFactor;
      GlyphOffset.Y *= ScreenScaleFactor;
      GlyphOffset.W *= ScreenScaleFactor;
      GlyphOffset.H *= ScreenScaleFactor;
      Push2DQuad(RenderGroup, GlyphOffset, 0, TextureRect,Color, FontMap->BitmapHandle);
    }
    PixelPosX += CH->xadvance;
    ++String;
  }
}

render_group* InitiateRenderGroup()
{
  render_group* Result = BootstrapPushStruct(render_group, Arena);
  Result->PushBufferMemory = BeginTemporaryMemory(&Result->Arena);

  ResetRenderGroup(Result);
  return Result;
}

void PushElectricalComponent(component_hitbox* HitBox, r32 PixelsPerUnitLength, u32 TileType, bitmap_handle TileHandle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->WorldGroup;
  bitmap_points TilePoint = GetElectricalComponentSpriteBitmapPoints(TileType);
  rect2f RectInPixels = GetTextureRectRh(TilePoint.TopLeft, TilePoint.BotRight);
  rect2f UVRect = GetUVRectRh(TilePoint.TopLeft, TilePoint.BotRight);
  v4 Color = V4(1,1,1,1);


  rect2f MainRect = Rect2f(HitBox->Main.Pos.X - HitBox->Main.W*0.5f,
                           HitBox->Main.Pos.Y - HitBox->Main.H*0.5f,
                           HitBox->Main.W,
                           HitBox->Main.H);
  v2 MainRectRotOffset = HitBox->Main.RotationCenterOffset;

  // Actual Sprite
  Push2DQuadSpecial(RenderGroup, MainRect, HitBox->Main.Rotation, MainRectRotOffset, UVRect, Color, TileHandle);


  r32 MouseSelectionWidth = GlobalGameState->World->TileMap.TileWidth;
  r32 MouseSelectionHeight = GlobalGameState->World->TileMap.TileHeight;
  b32 IsIntersecting = Intersects_XYPlane(&HitBox->Main, &GlobalGameState->World->MouseSelector.WorldPos);
  if(IsIntersecting)
  {
    v2 BoxDimensions     = V2(HitBox->Main.W,
                              HitBox->Main.H);
    v2 BoxCenterPosition = V2(HitBox->Main.Pos.X,
                              HitBox->Main.Pos.Y);
    v2 RelativeRotationCenter = HitBox->Main.RotationCenterOffset;
    v4 BoxColor = V4(1,1,1,0.5);

    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BoxDimensions.X*0.5f),
      (BoxCenterPosition.Y - BoxDimensions.Y*0.5f),
      BoxDimensions.X, BoxDimensions.Y), BoxColor, HitBox->Main.Rotation, RelativeRotationCenter);

    r32 BorderThickness = 0.01f;
    r32 BorderWidth  = BoxDimensions.X - 2 * BorderThickness;
    r32 BorderOffsetX = (BorderWidth + BorderThickness)*0.5f;

    r32 BorderHeight = BoxDimensions.Y - 2 * BorderThickness;
    r32 BorderOffsetY = (BorderHeight + BorderThickness)*0.5f;
    v4 BorderColor = V4(0,1,0,1);
    v4 CornerColor = V4(1,0,0,1);

    // Top Right Corner
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderThickness * 0.5f),
      (BoxCenterPosition.Y - BorderThickness * 0.5f),
      BorderThickness,
      BorderThickness),
      CornerColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X - BorderOffsetX,
         RelativeRotationCenter.Y - BorderOffsetY));

    // Top Left Corner
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderThickness * 0.5f),
      (BoxCenterPosition.Y - BorderThickness * 0.5f),
      BorderThickness,
      BorderThickness),
      CornerColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X + BorderOffsetX,
         RelativeRotationCenter.Y - BorderOffsetY));

    // Bot Right Corner
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderThickness * 0.5f),
      (BoxCenterPosition.Y - BorderThickness * 0.5f),
      BorderThickness,
      BorderThickness),
      CornerColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X - BorderOffsetX,
         RelativeRotationCenter.Y + BorderOffsetY));

    // Bot Left Corner
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderThickness * 0.5f),
      (BoxCenterPosition.Y - BorderThickness * 0.5f),
      BorderThickness,
      BorderThickness),
      CornerColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X + BorderOffsetX,
         RelativeRotationCenter.Y + BorderOffsetY));

    // Top Border
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderWidth * 0.5f),
      (BoxCenterPosition.Y - BorderThickness * 0.5f),
      BorderWidth,
      BorderThickness),
      BorderColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X,
         RelativeRotationCenter.Y - BorderOffsetY));
    
    // Bot Border
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderWidth * 0.5f),
      (BoxCenterPosition.Y - BorderThickness*0.5f),
      BorderWidth,
      BorderThickness),
      BorderColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X,
         RelativeRotationCenter.Y + BorderOffsetY));

    // Left Border
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderThickness*0.5f),
      (BoxCenterPosition.Y - BorderHeight*0.5f),
      BorderThickness,
      BorderHeight),
      BorderColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X + BorderOffsetX,
         RelativeRotationCenter.Y));

    // Right Border
    Push2DColoredQuad(RenderGroup, Rect2f(
      (BoxCenterPosition.X - BorderThickness*0.5f),
      (BoxCenterPosition.Y - BorderHeight*0.5f),
      BorderThickness,
      BorderHeight),
      BorderColor, HitBox->Main.Rotation,
      V2(RelativeRotationCenter.X - BorderOffsetX,
         RelativeRotationCenter.Y));

  }
  
  #if DRAW_HITBOX_AND_POINTS
  // Rotation Center
  Push2DColoredQuad(RenderGroup, Rect2f(HitBox->Main.Pos.X - MouseSelectionWidth/2.f, HitBox->Main.Pos.Y - MouseSelectionHeight/2.f, MouseSelectionWidth, MouseSelectionHeight), V4(1,0,0,1), HitBox->Main.Rotation, V2(0,0));

  for(u32 idx = 1; idx < ArrayCount(HitBox->Box); idx++)
  {
    hitbox* hb = HitBox->Box + idx;
    if(hb->W > 0)
    {
      rect2f Rect = Rect2f(HitBox->Main.Pos.X - hb->W * 0.5f,
                           HitBox->Main.Pos.Y - hb->H * 0.5f,
                           hb->W,
                           hb->H);
      Push2DColoredQuad(RenderGroup, Rect, V4(1,1,1,0.7),  HitBox->Main.Rotation, hb->RotationCenterOffset);
    } 
  }

  #endif

}

//  a0 < t < a1;
//  t == a0 = 1;
//  t == a1 = 0;
//(y0x1 - y1x0)/(x1-x0) = b
//k = (y1-y0) / (x1-x0)

r32 Linearize(r32 x, r32 x0, r32 y0, r32 x1, r32 y1)
{
  if(x<x0)
  {
    return y0;
  }else if (x>x1)
  {
    return y1;
  }
  r32 k = (y1-y0) / (x1-x0);
  r32 b = (y0*x1 - y1*x0)/(x1-x0);
  return k * x + b;
}

void DrawGrid( rect2f ScreenRect, tile_map* TileMap, u32 TileStride, v3 CameraPosition, r32 Alpha )
{
  if(Alpha<=0)
  {
    return;
  }

  render_group* RenderGroup = GlobalGameState->RenderCommands->WorldGroup;

  r32 TileStepX = TileMap->TileWidth;
  r32 MinX = ScreenRect.X + CameraPosition.X;
  r32 MaxX = MinX + ScreenRect.W;
  
  r32 StrideXLength = TileStride * TileStepX;
  r32 StrideXCount = Ciel(ScreenRect.W / StrideXLength) + 1;
  r32 Width = StrideXCount * StrideXLength;

  r32 MinStridesCountX = MinX / StrideXLength;
  r32 StridesToMinX = Floor(MinStridesCountX);
  r32 StartX = StridesToMinX * StrideXLength;
  r32 EndX = StartX + Width;

  r32 TileStepY = TileMap->TileHeight;
  r32 MinY = ScreenRect.Y + CameraPosition.Y;
  r32 MaxY = MinY + ScreenRect.H;

  r32 StrideYLength = TileStride * TileStepY;
  r32 StrideYCount = Ciel(ScreenRect.H / StrideYLength) + 1;
  r32 Height = StrideYCount * StrideYLength;

  r32 MinStridesCountY = MinY / StrideYLength;
  r32 StridesToMinY = Floor(MinStridesCountY);
  r32 StartY = StridesToMinY * StrideYLength;
  r32 EndY = StartY + Height;

  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = GameGetAspectRatio();
  r32 PixelWidthInWorld  = ScreenRect.W / (r32) WindowSize.WidthPx;
  r32 PixelHeightInWorld = ScreenRect.H / WindowSize.HeightPx;

  for(r32 TileY  = StartY;
          TileY <= EndY;
          TileY += StrideYLength)
  {
    rect2f Rect = Rect2f(StartX, TileY - PixelHeightInWorld*0.5f + 0.5f*TileStepY, Width, PixelHeightInWorld );  
    Push2DColoredQuad(RenderGroup, Rect, V4(1,1,1,Alpha), 0, V2(0,0)); 
  }  
  for(r32 TileX  = StartX;
          TileX <= EndX;
          TileX += StrideXLength)
  {
    rect2f Rect = Rect2f(TileX - PixelWidthInWorld*0.5f + 0.5f*TileStepX, StartY, PixelWidthInWorld, Height );
    Push2DColoredQuad(RenderGroup, Rect, V4(1,1,1,Alpha), 0, V2(0,0));
  }
}

void FillRenderPushBuffer(world* World)
{
  TIMED_FUNCTION();

  render_group* RenderGroup = GlobalGameState->RenderCommands->WorldGroup;
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  entity_manager* EM = GlobalGameState->EntityManager;
  game_asset_manager* AM = GlobalGameState->AssetManager;

  r32 OrthoZoom = 0;
  {
    filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    while(Next(&EntityIterator))
    {
      component_camera* Camera = (component_camera*) GetComponent(EM, &EntityIterator, COMPONENT_FLAG_CAMERA);
      RenderGroup->ProjectionMatrix = Camera->P;
      RenderGroup->ViewMatrix       = Camera->V;
      RenderGroup->CameraPosition = GetPositionFromMatrix( &Camera->V);
      OrthoZoom = Camera->OrthoZoom;
    }
  }

  bitmap_handle TileHandle;
  GetHandle(AssetManager, "TileSheet", &TileHandle);
  bitmap* ElectricalComponentSpriteSheet = GetAsset(AssetManager, TileHandle);

  r32 SpriteSheetWidth =  (r32) ElectricalComponentSpriteSheet->Width;
  r32 SpriteSheetHeight = (r32) ElectricalComponentSpriteSheet->Height;
  
  rect2f ScreenRect = ScreenRect = GetCameraScreenRect(OrthoZoom);

  electrical_component* Component = World->Source;
  #if 0
  r32 xPos = 0;
  while(Component)
  {
    switch(Component->Type)
    {
      case ElectricalComponentType_Source:
      {
        PushElectricalComponent(xPos, 0, 1, 1, Tau32/4.f, ElectricalComponentSprite_Source, SpriteSheetWidth,SpriteSheetHeight, TileHandle);
        Component = GetComponentConnectedAtPin(Component, ElectricalPinType_Output);
      }break;
      case ElectricalComponentType_Ground:
      {
        PushElectricalComponent(xPos, 0, 1, 1, Tau32/4.f, ElectricalComponentSprite_Ground, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        Component = 0;
      }break;
      case ElectricalComponentType_Led_Red:
      case ElectricalComponentType_Led_Green:
      case ElectricalComponentType_Led_Blue:
      {
        u32 LedState = 0;
        if(Component->DynamicState.Volt < 2.5f)
        {
          LedState = ElectricalComponentSprite_LedRedOff;
        }
        else
        {
          LedState = ElectricalComponentSprite_LedRedOn;
        }
        PushElectricalComponent(xPos, 0, 1, 1,  Tau32/4.f, ElectricalComponentSprite_WireBlack, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        PushElectricalComponent(xPos, 0, 1, 1, -Tau32/4.f, ElectricalComponentSprite_WireBlack, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        PushElectricalComponent(xPos, 0, 1, 1,  Tau32/4.f, LedState, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        Component = GetComponentConnectedAtPin(Component, ElectricalPinType_Negative);
      }break;
      case ElectricalComponentType_Resistor:
      {
        PushElectricalComponent(xPos, 0, 1, 1, Tau32/4.f, ElectricalComponentSprite_Resistor, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        Component = GetComponentConnectedAtPin(Component, ElectricalPinType_B);
      }break;
      case ElectricalComponentType_Wire:
      {
      }break;
    }
    xPos++;
  }
  #endif

  r32 MinY = ScreenRect.Y + RenderGroup->CameraPosition.Y;
  r32 MaxY = MinY + ScreenRect.H;
  r32 MinY_Tiles = Floor(MinY);
  r32 MaxY_Tiles = Ciel(MaxY);

  r32 MinX = ScreenRect.X + RenderGroup->CameraPosition.X;
  r32 MaxX = MinX + ScreenRect.W;
  r32 MinX_Tiles = Floor(MinX);
  r32 MaxX_Tiles = Ciel(MaxX);

  r32 x = Log(OrthoZoom);

  r32 Alpha = 1;
  r32 x0 = Log(1);
  r32 y0 = 1;
  r32 x1 = Log(14);
  r32 y1 = 0;
  
  Alpha = Linearize(x, x0, y0, x1, y1);

  r32 Alpha2 = 0;
  r32 x0_2 = Log(2);
  r32 y0_2 = 0;
  r32 x1_2 = Log(10);
  r32 y1_2 = 1;
  r32 x2_2 = Log(2000);
  r32 y2_2 = 0;
  if(x > x0_2 && x < x1_2 )
    Alpha2 = Linearize(x, x0_2, y0_2, x1_2, y1_2);
  else
    Alpha2 = Linearize(x, x1_2, y1_2, x2_2, y2_2);


  r32 Alpha3 = 0;
  r32 x0_3 = Log(100);
  r32 y0_3 = 0;
  r32 x1_3 = Log(1000);
  r32 y1_3 = 1;
  r32 x2_3 = Log(200000);
  r32 y2_3 = 0;
  if(x > x0_2 && x < x1_2 )
    Alpha3 = Linearize(x, x0_3, y0_3, x1_3, y1_3);
  else
    Alpha3 = Linearize(x, x1_3, y1_3, x2_3, y2_3);

  tile_map* TileMap = &GlobalGameState->World->TileMap;
  DrawGrid(ScreenRect, TileMap, 1, RenderGroup->CameraPosition, Alpha);
  DrawGrid(ScreenRect, TileMap, 100, RenderGroup->CameraPosition, Alpha2);
  DrawGrid(ScreenRect, TileMap, 10000, RenderGroup->CameraPosition, Alpha3);

  mouse_selector* MouseSelector = &GlobalGameState->World->MouseSelector;
  {
    filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_ELECTRICAL | COMPONENT_FLAG_HITBOX);
    while(Next(&EntityIterator))
    {
      electrical_component* ElectricalComponent = GetElectricalComponent(&EntityIterator);
      component_hitbox* Hitbox = GetHitboxComponent(&EntityIterator);
      u32 TileSpriteSheet = ElectricalComponentToSpriteType(ElectricalComponent);
      PushElectricalComponent(Hitbox, PIXELS_PER_UNIT_LENGTH, TileSpriteSheet, TileHandle);
    }
    
  }
#if 1
    char StringBuffer[1024] = {};
    mouse_input* Mouse = &GlobalGameState->Input->Mouse;

    Platform.DEBUGFormatString(StringBuffer, 1024, 1024-1, "CanPos (%2.2f %2.2f) ScreenPos (%2.2f %2.2f) WorldPos (%2.2f %2.2f)",
      MouseSelector->CanPos.X, MouseSelector->CanPos.Y, MouseSelector->ScreenPos.X, MouseSelector->ScreenPos.Y, MouseSelector->WorldPos.X, MouseSelector->WorldPos.Y);
    PushTextAt(MouseSelector->CanPos.X, MouseSelector->CanPos.Y, StringBuffer, 8, V4(1,1,1,1));
#endif
}