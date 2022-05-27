#include "render_push_buffer.h"
#include "entity_components.h"
#include "component_camera.h"
#include "breadboard_tile.h"
#include "random.h"


#define GetBody(Header, Type) ((Type*) (((push_buffer_header*) Header ) + 1))

u32 RenderTypeToBodySize(render_buffer_entry_type Type)
{
  switch(Type)
  {
    case render_buffer_entry_type::NEW_LEVEL: return 0;
    case render_buffer_entry_type::QUAD_2D: return sizeof(entry_type_2d_quad); 
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

void Push2DColoredQuad(render_group* RenderGroup, rect2f QuadRect, v4 Color)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D_COLOR);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->Colour = Color;
  Body->QuadRect = QuadRect;
}

void Push2DQuad(render_group* RenderGroup, rect2f QuadRect, rect2f UVRect, v4 Color, bitmap_handle BitmapHandle)
{
  push_buffer_header* Header = PushNewEntry(RenderGroup, render_buffer_entry_type::QUAD_2D);
  entry_type_2d_quad* Body = GetBody(Header, entry_type_2d_quad);
  Body->UVRect = UVRect;
  Body->QuadRect = QuadRect;
  Body->BitmapHandle = BitmapHandle;
  Body->Colour = Color;
}

void PushOverlayQuad(rect2f QuadRect, v4 Color)
{
  QuadRect.X += QuadRect.W*0.5f;
  QuadRect.Y += QuadRect.H*0.5f;
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  Push2DColoredQuad(RenderGroup, QuadRect, Color);
}

void PushTexturedOverlayQuad(rect2f QuadRect,  rect2f UVRect,  bitmap_handle BitmapHandle)
{
  QuadRect.X += QuadRect.W*0.5f;
  QuadRect.Y += QuadRect.H*0.5f;
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  Push2DQuad(RenderGroup, QuadRect, UVRect, V4(1,1,1,1), BitmapHandle);
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

  const r32 X = Floor(xPosPx + 0.5f) + GlyphWidth*0.5f + GlyphOffsetX;
  const r32 Y = Floor(yPosPx + 0.5f) - GlyphHeight*0.5f + GlyphOffsetY;
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
      Push2DQuad(RenderGroup, GlyphOffset, TextureRect,Color, FontMap->BitmapHandle);
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

// Our Bitmap Origin is [0,0]->Left,Bottom, [TextureWidth_pixel,TextureHeight_pixel] -> TopRight
// Input origin is [0,0]->Left,Top,  [TextureWidth_pixel,TextureHeight_pixel] -> BotRight
m3 GetTextureTranslationMatrix_OriginTopLeft(s32 X0_pixel, s32 Y0_pixel, s32 Width_pixel, s32 Height_pixel, s32 TextureWidth_pixel, s32 TextureHeight_pixel)
{
  r32 XMin = (r32) X0_pixel;
  r32 Xoffset = XMin / (r32) TextureWidth_pixel;
  r32 ScaleX  =  (r32) (Width_pixel-1) / (r32) TextureWidth_pixel;

  // Note: Picture is stored and read from bottom left to up and right but
  //     The coordinates given were top left to bottom right so we need to
  //     Invert the Y-Axis;
  r32 YMin = (r32) TextureHeight_pixel - (Y0_pixel + Height_pixel-1);
  r32 Yoffset = YMin / (r32) TextureHeight_pixel;
  r32 ScaleY  =  (r32) (Height_pixel) / (r32) TextureHeight_pixel;
  m3 Result = GetTranslationMatrix(V3(Xoffset,Yoffset,1)) * GetScaleMatrix(V3(ScaleX,ScaleY,1));

  return Result;
}

void PushElectricalComponent(r32 xPos, r32 yPos, r32 SizeX, r32 SizeY,
  u32 TileType, r32 BitmapWidth, r32 BitmapHeight, bitmap_handle TileHandle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->WorldGroup;
  bitmap_coordinate TileCoordinate = GetElectricalComponentSpriteBitmapCoordinate(TileType);
  rect2f UVRect = GetTextureRect(&TileCoordinate, BitmapWidth, BitmapHeight);
  v4 Color = V4(1,1,1,1);
  Push2DQuad(RenderGroup, Rect2f(xPos, yPos, SizeX, SizeY), UVRect, Color, TileHandle);
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
    BeginScopedEntityManagerMemory();
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    while(Next(EM, ComponentList))
    {
      component_camera* Camera = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
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
  PushElectricalComponent(RenderGroup->CameraPosition.X, RenderGroup->CameraPosition.Y, ScreenRect.W, ScreenRect.H,
  ElectricalComponentSprite_Empty, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
  
  electrical_component* Component = World->Source;
  r32 xPos = 0;
  while(Component)
  {
    switch(Component->Type)
    {
      case ElectricalComponentType_Source:
      {
        PushElectricalComponent(xPos, 0, 1, 1, ElectricalComponentSprite_Source, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        Component = GetComponentConnectedAtPin(Component, ElectricalPinType_Output);
      }break;
      case ElectricalComponentType_Ground:
      {
        PushElectricalComponent(xPos, 0, 1, 1, ElectricalComponentSprite_Ground, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
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
        PushElectricalComponent(xPos, 0, 1, 1, ElectricalComponentSprite_WireBlack, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        PushElectricalComponent(xPos, 0, 1, 1, ElectricalComponentSprite_WireBlack, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        PushElectricalComponent(xPos, 0, 1, 1, LedState, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        Component = GetComponentConnectedAtPin(Component, ElectricalPinType_Negative);
      }break;
      case ElectricalComponentType_Resistor:
      {
        PushElectricalComponent(xPos, 0, 1, 1, ElectricalComponentSprite_Resistor, SpriteSheetWidth, SpriteSheetHeight, TileHandle);
        Component = GetComponentConnectedAtPin(Component, ElectricalPinType_B);
      }break;
      case ElectricalComponentType_Wire:
      {
      }break;
    }
    xPos++;
  }

  r32 MinY_Tiles = Floor(ScreenRect.Y + RenderGroup->CameraPosition.Y);
  r32 MaxY_Tiles = Ciel(ScreenRect.Y + ScreenRect.H + RenderGroup->CameraPosition.Y);

  r32 MinX_Tiles = Floor(ScreenRect.X + RenderGroup->CameraPosition.X);
  r32 MaxX_Tiles = Ciel(ScreenRect.X + ScreenRect.W + RenderGroup->CameraPosition.X);

  tile_map* TileMap = &GlobalGameState->World->TileMap;
  for(r32 Row_Tiles  = MinY_Tiles;
          Row_Tiles <= MaxY_Tiles;
          Row_Tiles++)
  {
    for(r32 Col_Tiles  = MinX_Tiles;
            Col_Tiles <= MaxX_Tiles;
            Col_Tiles++)
    {
      tile_map_position TilePos = CanonicalizePosition(TileMap, V3( Col_Tiles, Row_Tiles, 0 ) );
      tile_contents Content = GetTileContents(TileMap, TilePos);
      if(Content.Component)
      {
        PushElectricalComponent(Col_Tiles, Row_Tiles, 1, 1, Content.Component->Type, SpriteSheetWidth, SpriteSheetHeight, TileHandle);  
      }   
    }
  }
}