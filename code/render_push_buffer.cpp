#include "render_push_buffer.h"
#include "entity_components.h"

push_buffer_header* PushNewHeader(render_group* RenderGroup, render_buffer_entry_type Type, u32 RenderState, u32 SortKey = 0)
{
  RenderGroup->ElementCount++;
  push_buffer_header* NewEntryHeader = PushStruct(&RenderGroup->Arena, push_buffer_header);
  NewEntryHeader->SortKey = SortKey;
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
  NewEntryHeader->RenderState = RenderState;
  RenderGroup->BufferCounts[(u32) Type]++;

  return NewEntryHeader;
}

void PushNewRenderLevel(render_group* RenderGroup)
{
  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::NEW_LEVEL, RENDER_STATE_NONE);
}

void PushTexturedOverlayQuad(rect2f QuadRect, rect2f TextureRect, bitmap_handle Handle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::OVERLAY_TEXTURED_QUAD, RENDER_STATE_FILL);
  entry_type_overlay_textured_quad* Body = PushStruct(&RenderGroup->Arena, entry_type_overlay_textured_quad);
  Body->TextureRect = TextureRect;
  QuadRect.X += QuadRect.W/2.f;
  QuadRect.Y += QuadRect.H/2.f;
  Body->QuadRect = QuadRect;
  Body->Handle = Handle;
}

void PushOverlayQuad(rect2f QuadRect, v4 Color)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::OVERLAY_COLORED_QUAD, RENDER_STATE_FILL);
  entry_type_overlay_color_quad* Body = PushStruct(&RenderGroup->Arena, entry_type_overlay_color_quad);
  Body->Colour = Color;
  Body->QuadRect = QuadRect;
}

void PushOverlayText(rect2f QuadRect, rect2f TextureRect, v4 Color, bitmap_handle BitmapHandle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::TEXT, RENDER_STATE_FILL);
  entry_type_text* Body = PushStruct(&RenderGroup->Arena, entry_type_text);
  Body->BitmapHandle = BitmapHandle;
  Body->QuadRect = QuadRect;
  Body->UVRect = TextureRect;
  Body->Colour = Color;
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
      PushOverlayText(GlyphOffset, TextureRect,Color, FontMap->BitmapHandle);
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

internal inline void
PushLine(render_group* RenderGroup, v3 Start, v3 End, v3 CameraPosition, r32 LineThickness, c8* MaterialName)
{
  v3 xAxis = End-Start;
  v3 cameraDir = CameraPosition - Start;

  m4 RotMat = GetRotationMatrix_X_ZHint(xAxis, cameraDir);

  r32 Length = Norm(End-Start);

  m4 ScaleMat = M4Identity();
  ScaleMat.E[0] = Length;
  ScaleMat.E[5] = LineThickness;

  v3 MidPoint = (Start + End) / 2;
  m4 TransMat = M4Identity();
  TransMat.E[3]  = MidPoint.X;
  TransMat.E[7]  = MidPoint.Y;
  TransMat.E[11] = MidPoint.Z;

  push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_CULL_BACK | RENDER_STATE_FILL );
  entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);

  GetHandle(GlobalGameState->AssetManager, "quad", &Body->Object);
  GetHandle(GlobalGameState->AssetManager, MaterialName, &Body->Material);
  GetHandle(GlobalGameState->AssetManager, "null", &Body->Bitmap);
  Body->M = TransMat*RotMat*ScaleMat;
  Body->NM = Transpose(RigidInverse(Body->M));
  Body->TM = M4Identity();
}

internal inline void
PushBoxFrame(render_group* RenderGroup, m4 M, aabb3f AABB, v3 CameraPosition, r32 LineThickness, c8* MaterialName)
{
  v3 P[8] = {};
  GetAABBVertices(&AABB, P);
  for(u32 i = 0; i < ArrayCount(P); ++i)
  {
    P[i] = V3(M*V4(P[i],1));
  }

  // Negative Z
  PushLine(RenderGroup, P[0], P[1], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[1], P[2], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[2], P[3], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[3], P[0], CameraPosition, LineThickness, MaterialName);
  // Positive Z
  PushLine(RenderGroup, P[4], P[5], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[5], P[6], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[6], P[7], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[7], P[4], CameraPosition, LineThickness, MaterialName);
  // Joining Lines
  PushLine(RenderGroup, P[0], P[4], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[1], P[5], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[2], P[6], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[3], P[7], CameraPosition, LineThickness, MaterialName);
}


void PushArrow(render_group* RenderGroup, v3 Start, v3 Vector, c8* Color)
{
  v4 Q = GetRotation( V3(1,0,0), Vector);
  const m4 ObjRotation = GetRotationMatrix(Q);

  r32 Length = Norm(Vector);
  m4 M = GetScaleMatrix(V4(Length, 0.05, 0.05, 1));
  Translate(V4(Length/2.f,0,0,1), M);
  M = ObjRotation * M;
  Translate(V4(Start), M);

  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_FILL | RENDER_STATE_CULL_BACK);
  entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
  GetHandle(GlobalGameState->AssetManager,"voxel", &Body->Object);
  GetHandle(GlobalGameState->AssetManager, Color, &Body->Material);

  Body->M = M;
  Body->NM = Transpose(RigidInverse(Body->M));
}

void PushCube(render_group* RenderGroup, v3 Position, r32 Scale, c8* Color)
{
  push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_FILL | RENDER_STATE_CULL_BACK );
  entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
  GetHandle(GlobalGameState->AssetManager, "voxel", &Body->Object);
  GetHandle(GlobalGameState->AssetManager, Color, &Body->Material);

  const m4 ScaleMatrix = GetScaleMatrix(V4(Scale,Scale,Scale,1));
  Body->M = GetTranslationMatrix( V4(Position,1))*ScaleMatrix;
  Body->NM = Transpose(RigidInverse(Body->M));
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

m4 GetModelMatrix(v3 Position, r32 Scale, r32 RotationAngle, v3 RotationAxis)
{
  v3 ScaleVec = V3(Scale, Scale, Scale);
  v4 Rotation = RotateQuaternion(RotationAngle, V4(RotationAxis,0));
  m4 ModelMatrix = GetModelMatrix(Position, Rotation, ScaleVec);
  return ModelMatrix;
}

void FillRenderPushBuffer(world* World)
{
  TIMED_FUNCTION();

  render_group* RenderGroup = GlobalGameState->RenderCommands->WorldGroup;
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  entity_manager* EM = GlobalGameState->EntityManager;
  game_asset_manager* AM = GlobalGameState->AssetManager;
  v3 CameraPosition = {};
  {
    BeginScopedEntityManagerMemory();
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    while(Next(EM, ComponentList))
    {
      component_camera* Camera = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
      RenderGroup->ProjectionMatrix = Camera->P;
      RenderGroup->ViewMatrix       = Camera->V;
      RenderGroup->CameraPosition = V3(Column(RigidInverse(Camera->V),3));
    }
  }

  bitmap_handle TileHandle;
  GetHandle(AssetManager, "TileSheet", &TileHandle);
  //rect2f Subtexture = Rect2f(0,0,64/512.f,64/512.f);
      


  r32 GridSide = 63.5/512.f;
  game_window_size WindowSize = GameGetWindowSize();
  
  rect2f Subtexture = Rect2f(0,1-GridSide, GridSide, GridSide); // UW Coordinates
  r32 AspectRatio = WindowSize.WidthPx / (r32) WindowSize.HeightPx;
  // |------------|
  // |            |
  // |            |
  // |------------|

  r32 MinY_Tiles = -10;
  r32 MaxY_Tiles = 10;

  r32 MinX_Tiles = -10;
  r32 MaxX_Tiles = 10;

  for(r32 Row_Tiles  = MinY_Tiles;
          Row_Tiles <= MaxY_Tiles;
          Row_Tiles++)
  {
    for(r32 Col_Tiles  = MinX_Tiles;
            Col_Tiles <= MaxX_Tiles;
            Col_Tiles++)
    {
      push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::TEXTURED_QUAD, RENDER_STATE_FILL);
      entry_type_textured_quad* Body = PushStruct(&RenderGroup->Arena, entry_type_textured_quad);

      r32 RotationAngle = 0;
      v3 RitationAxis = V3(0,0,1);
      r32 Scale = 1;
      Body->M = GetModelMatrix(V3(Col_Tiles, Row_Tiles, 0), Scale, RotationAngle, RitationAxis);

      s32 X0_pixel = 4;
      s32 Y0_pixel = 4;
      s32 Width_pixel = 60;
      s32 Height_pixel = 60;
      s32 TextureWidth_pixel = 512;
      s32 TextureHeight_pixel = 512;
      Body->TM = GetTextureTranslationMatrix_OriginTopLeft(X0_pixel, Y0_pixel, Width_pixel, Height_pixel, TextureWidth_pixel, TextureHeight_pixel);
      Body->Bitmap = TileHandle;
    }
  }
}