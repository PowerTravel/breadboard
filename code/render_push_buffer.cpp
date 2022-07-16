#include "render_push_buffer.h"
#include "component_camera.h"
#include "breadboard_tile.h"
#include "breadboard_components.h"
#include "random.h"


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

void Push2DColoredQuad(render_group* RenderGroup, rect2f QuadRect, v4 Color, r32 Rotation)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D_COLOR);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->Colour = Color;
  Body->QuadRect = QuadRect;
  Body->Rotation = Rotation;
  Recenter(&Body->QuadRect);
}

void Push2DQuadSpecial(render_group* RenderGroup, rect2f QuadRect, float Rotation, rect2f UVRect, v4 Color, bitmap_handle BitmapHandle)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D_SPECIAL);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->UVRect = UVRect;
  Body->QuadRect = QuadRect;
  Body->BitmapHandle = BitmapHandle;
  Body->Colour = Color;
  Body->Rotation = Rotation;
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
  Push2DColoredQuad(RenderGroup, QuadRect, Color, 0);
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

void PushElectricalComponent(r32 xPos, r32 yPos, r32 PixelsPerUnitLength, r32 Rotation,
  u32 TileType, r32 BitmapWidth, r32 BitmapHeight, bitmap_handle TileHandle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->WorldGroup;
  bitmap_points TilePoint = GetElectricalComponentSpriteBitmapPoints(TileType);
  
  rect2f RectInPixels = GetTextureRect(TilePoint.TopLeft, TilePoint.BotRight, BitmapWidth, BitmapHeight, TilePoint.YDirectionUp);

  rect2f UVRect = GetUVRect(RectInPixels, BitmapWidth, BitmapHeight);
  
  r32 OneOverPixelsPerUnitLength = 1.f/PixelsPerUnitLength;
  r32 ScaleW = RectInPixels.W * OneOverPixelsPerUnitLength;
  r32 ScaleH = RectInPixels.H * OneOverPixelsPerUnitLength;

  v2 Center = TilePoint.Center * OneOverPixelsPerUnitLength;

  xPos -= Center.X;
  yPos -= Center.Y;

  v4 Color = V4(1,1,1,1);
  Push2DQuadSpecial(RenderGroup, Rect2f(xPos, yPos, ScaleW, ScaleH), Rotation, UVRect, Color, TileHandle);
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
    Push2DColoredQuad(RenderGroup, Rect, V4(1,1,1,Alpha), 0); 
  }  
  for(r32 TileX  = StartX;
          TileX <= EndX;
          TileX += StrideXLength)
  {
    rect2f Rect = Rect2f(TileX - PixelWidthInWorld*0.5f + 0.5f*TileStepX, StartY, PixelWidthInWorld, Height );
    Push2DColoredQuad(RenderGroup, Rect, V4(1,1,1,Alpha), 0);
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
  
  component_hitbox* Hitbox = 0;
  {
    filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_ELECTRICAL);
    mouse_selector* MouseSelector = &GlobalGameState->World->MouseSelector;
    r32 BoxWidth = 0.05;
    while(Next(&EntityIterator))
    {
      electrical_component* ElectricalComponent = GetElectricalComponent(&EntityIterator);
      v2 CenterOffset = GetCenterOffset(ElectricalComponent);
      Hitbox = GetHitboxComponent(&EntityIterator);
      u32 TileSpriteSheet = ElectricalComponentToSpriteType(ElectricalComponent);
      r32 PixelsPerUnitLegth = 128;
      r32 X = Hitbox->Rect.X + CenterOffset.X;
      r32 Y = Hitbox->Rect.Y + CenterOffset.Y;

      PushElectricalComponent(X, Y, PixelsPerUnitLegth, Hitbox->Rotation, TileSpriteSheet, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
      rect2f ColoredRect = Rect2f(X - Hitbox->Rect.W * 0.5f,Y - Hitbox->Rect.H * 0.5f, Hitbox->Rect.W, Hitbox->Rect.H);
      Push2DColoredQuad(GlobalGameState->RenderCommands->WorldGroup, ColoredRect, V4(1,1,1,0.5), Hitbox->Rotation);
    

      rect2f CenterBox = Rect2f(Hitbox->Rect.X + Hitbox->RotationCenter.X - BoxWidth/2.f, Hitbox->Rect.Y + Hitbox->RotationCenter.Y - BoxWidth/2.f, BoxWidth, BoxWidth);
      Push2DColoredQuad(GlobalGameState->RenderCommands->WorldGroup, CenterBox, V4(0,1,0,1), 0);
    }
    
    // Draw Mouse
    rect2f MouseBox = Rect2f(MouseSelector->WorldPos.X - BoxWidth/2.f, MouseSelector->WorldPos.Y - BoxWidth/2.f, BoxWidth, BoxWidth);
    Push2DColoredQuad(GlobalGameState->RenderCommands->WorldGroup, MouseBox, V4(0,0,0,1), 0);
  }

#if 1
  if(Hitbox)
  {
    char StringBuffer[1024] = {};
    mouse_input* Mouse = &GlobalGameState->Input->Mouse;
    mouse_selector* ms = &GlobalGameState->World->MouseSelector; 
    rect2f ScreenRect2 = GetCameraScreenRect(OrthoZoom);

    r32 X = Hitbox->Rect.X + Hitbox->Rect.W * 0.5f;
    r32 Y = Hitbox->Rect.Y + Hitbox->Rect.H * 0.5f;
    Platform.DEBUGFormatString(StringBuffer, 1024, 1024-1, "%f %f, %2.2f %2.2f", X, Y,
      ms->WorldPos.X, ms->WorldPos.Y);
    PushTextAt(Mouse->X, Mouse->Y, StringBuffer, 8, V4(1,1,1,1));
  }
#endif
}