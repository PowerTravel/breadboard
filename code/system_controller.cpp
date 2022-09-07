#include "component_camera.h"
#include "breadboard_entity_components.h"
#include "breadboard_tile.h"

// Input in Screen Space
void HandleZoom(component_camera* Camera, r32 MouseX, r32 MouseY, r32 ScrollWheel)
{
  if(ScrollWheel != 0)
  {
    const r32 AspectRatio = GameGetAspectRatio();

    // TODO:  - Do some slerping here, possibly tied to the scroll wheel, zoom in/zoom out
    r32 ZoomSpeed = 20; 
    v2 PrePos = GetMousePosInProjectionWindow(MouseX, MouseY, Camera->OrthoZoom, AspectRatio);
    r32 ZoomPercentage = - ZoomSpeed * ScrollWheel * GlobalGameState->Input->dt;
    r32 Zoom = Camera->OrthoZoom * ZoomPercentage;
    Camera->OrthoZoom += Zoom;

    rect2f ScreenRect = GetCameraScreenRect(Camera->OrthoZoom, AspectRatio);
    r32 Near  = -1;
    r32 Far   = 10;
    r32 Left  = ScreenRect.X;
    r32 Right = ScreenRect.X + ScreenRect.W;
    r32 Top   = ScreenRect.Y + ScreenRect.H;
    r32 Bot   = ScreenRect.Y;
    SetOrthoProj(Camera, Near, Far, Right, Left, Top, Bot );

    v2 PostPos = GetMousePosInProjectionWindow(MouseX, MouseY, Camera->OrthoZoom, AspectRatio);

    v2 CamDelta = PostPos - PrePos;
    TranslateCamera(Camera, -V3(CamDelta,0));  
  }  
}
// Input in Screen Space
void HandleTranslate(component_camera* Camera, b32 MouseActive, r32 MouseDX, r32 MouseDY)
{
  if(MouseActive)
  {
    rect2f ScreenRect = GetCameraScreenRect(Camera->OrthoZoom);

    MouseDX = 0.5f * MouseDX * ScreenRect.W;
    MouseDY = 0.5f * MouseDY * ScreenRect.H;

    TranslateCamera(Camera, V3(-MouseDX, -MouseDY,0));
  }
}

void InitiateElectricalComponent(electrical_component* Component, component_hitbox* Hitbox, u32 ElectricalType)
{
  r32 PixelsPerUnitLegth = 128;
  Component->Type = ElectricalType;
  u32 SpriteTileType = ElectricalComponentToSpriteType(Component);
  bitmap_points TilePoint = GetElectricalComponentSpriteBitmapPoints(SpriteTileType);
  
  Hitbox->RotationCenter = V2( (r32) TilePoint.Center.x, (r32) TilePoint.Center.y) / PixelsPerUnitLegth;
  Hitbox->Rect.W   = (TilePoint.BotRight.x - TilePoint.TopLeft.x) / PixelsPerUnitLegth;
  Hitbox->Rect.H   = (TilePoint.BotRight.y - TilePoint.TopLeft.y) / PixelsPerUnitLegth;
}

void ControllerSystemUpdate( world* World )
{
  TIMED_FUNCTION();

  entity_manager* EM = GlobalGameState->EntityManager;
  component_controller* Controller = 0;
  component_camera* Camera = 0;

  {
    filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA | COMPONENT_FLAG_CONTROLLER);
    while(Next(&EntityIterator))
    {
      Controller = GetControllerComponent(&EntityIterator);
      Camera = GetCameraComponent(&EntityIterator);
    }
  }
  Assert(Controller);
  Assert(Camera);

  keyboard_input* Keyboard = Controller->Keyboard;
  mouse_input* Mouse = Controller->Mouse;
  mouse_selector* MouseSelector = &GlobalGameState->World->MouseSelector;

  v2 MouseScreenSpaceStart = CanonicalToScreenSpace(V2(Mouse->X - Mouse->dX,Mouse->Y - Mouse->dY));
  v2 MouseScreenSpace   = CanonicalToScreenSpace(V2(Mouse->X, Mouse->Y));
  v2 MouseDeltaScreenSpace = MouseScreenSpace - MouseScreenSpaceStart;

  HandleZoom(Camera, MouseScreenSpace.X, MouseScreenSpace.Y, Mouse->dZ);
  HandleTranslate(Camera, Mouse->Button[PlatformMouseButton_Left].Active, MouseDeltaScreenSpace.X, MouseDeltaScreenSpace.Y);

  tile_map* TileMap = &GlobalGameState->World->TileMap;

  v3 CamPos = GetPositionFromMatrix(&Camera->V);
  r32 OrthoZoom = Camera->OrthoZoom;
  v2 MousePosWorldSpace = GetMousePosInWorld(CamPos, OrthoZoom, MouseScreenSpace);

  MouseSelector->LeftButton = Mouse->Button[PlatformMouseButton_Left];
  MouseSelector->RightButton = Mouse->Button[PlatformMouseButton_Right];
  MouseSelector->CanPos = V2(Mouse->X, Mouse->Y);
  MouseSelector->ScreenPos = MouseScreenSpace;
  MouseSelector->WorldPos = V3(MousePosWorldSpace,0);
  MouseSelector->TilePos = CanonicalizePosition(TileMap, V3(MousePosWorldSpace.X, MousePosWorldSpace.Y,0));
  
  tile_contents HotSelection = GetTileContents(TileMap, MouseSelector->TilePos);
  
  if(MouseSelector->SelectedContent.ElectricalComponentEntity.EntityID)
  {
    HotSelection = MouseSelector->SelectedContent;
  }

  if(HotSelection.ElectricalComponentEntity.EntityID)
  {
    component_hitbox* Hitbox = GetHitboxComponent(&HotSelection.ElectricalComponentEntity);
    MouseSelector->Rotation = Hitbox->Rotation;

    if(Pushed(Keyboard->Key_Q))
    {
      MouseSelector->Rotation += Tau32/4.f;
    }
    if(Pushed(Keyboard->Key_E))
    {
      MouseSelector->Rotation -= Tau32/4.f;
    }
    if(MouseSelector->Rotation > Pi32)
    {
      MouseSelector->Rotation -= Tau32;
    }else if(MouseSelector->Rotation < -Pi32)
    {
      MouseSelector->Rotation += Tau32;
    }

    Hitbox->Rotation = MouseSelector->Rotation;
  }

  if(!MouseSelector->SelectedContent.ElectricalComponentEntity.EntityID)
  {
    u32 Type = ElectricalComponentType_None;
    if(Pushed(Keyboard->Key_S))
    {
      Type = ElectricalComponentType_Source;
    }
    if(Pushed(Keyboard->Key_R))
    {
      Type = ElectricalComponentType_Resistor;
    }
    if(Pushed(Keyboard->Key_L))
    {
      Type = ElectricalComponentType_Led_Red;
    }
    if(Pushed(Keyboard->Key_G))
    {
      Type = ElectricalComponentType_Ground;
    }
    
    if(Type)
    {
      entity_id ElectricalEntity = NewEntity(EM);
      NewComponents(EM, &ElectricalEntity, COMPONENT_FLAG_ELECTRICAL);
      electrical_component* Component = GetElectricalComponent(&ElectricalEntity);
      component_hitbox* Hitbox = GetHitboxComponent(&ElectricalEntity);
      InitiateElectricalComponent(Component, Hitbox, Type);
      v2 CenterOffset = GetCenterOffset(Component);
      Hitbox->Rect.X   = MouseSelector->WorldPos.X - CenterOffset.X;
      Hitbox->Rect.Y   = MouseSelector->WorldPos.Y - CenterOffset.Y;
      Hitbox->Rotation = MouseSelector->Rotation;

      MouseSelector->SelectedContent.ElectricalComponentEntity = ElectricalEntity;
    }
  }else
  {
    entity_id* ElectricalEntity = &MouseSelector->SelectedContent.ElectricalComponentEntity;
    electrical_component* Component = GetElectricalComponent(ElectricalEntity);
    component_hitbox* Hitbox = GetHitboxComponent(ElectricalEntity);
    if(Pushed(Keyboard->Key_S))
    {
      InitiateElectricalComponent(Component, Hitbox, ElectricalComponentType_Source);
    }
    if(Pushed(Keyboard->Key_R))
    {
      InitiateElectricalComponent(Component, Hitbox, ElectricalComponentType_Resistor);
    }
    if(Pushed(Keyboard->Key_L))
    {
      InitiateElectricalComponent(Component, Hitbox, ElectricalComponentType_Led_Red);
    }
    if(Pushed(Keyboard->Key_G))
    {
      InitiateElectricalComponent(Component, Hitbox, ElectricalComponentType_Ground);
    }

    v2 CenterOffset = GetCenterOffset(Component);
    Hitbox->Rect.X = MouseSelector->WorldPos.X - CenterOffset.X;
    Hitbox->Rect.Y = MouseSelector->WorldPos.Y - CenterOffset.Y;
    Hitbox->Rotation = MouseSelector->Rotation;
  }

  if(Pushed(MouseSelector->LeftButton))
  {
    tile_contents PreviousContent = MouseSelector->SelectedContent;
    MouseSelector->SelectedContent = GetTileContents(TileMap, MouseSelector->TilePos);
    if(PreviousContent.ElectricalComponentEntity.EntityID)
    {
      component_hitbox* Hitbox = GetHitboxComponent(&PreviousContent.ElectricalComponentEntity);
      v3 Pos = V3(Hitbox->Rect.X, Hitbox->Rect.Y, 0);
      tile_map_position TilePos = CanonicalizePosition(TileMap, Pos);
      // TODO: We are casting a negative uint (which is a huge number) into a signed and relying on it being cast down to 
      //       the small negative number again. This is compiler dependant behaviour, Maybe fix this?
      Hitbox->Rect.X = Round( ( (r32) ( (s32) TilePos.AbsTileX))) * TileMap->TileWidth + 0.5f * TileMap->TileWidth;
      Hitbox->Rect.Y = Round( ( (r32) ( (s32) TilePos.AbsTileY))) * TileMap->TileHeight + 0.5f * TileMap->TileHeight;
      //Pos->Hitbox.Z = Round( ( (r32) ( (s32) TilePos.AbsTileZ))) * TileMap->TileDepth + 0.5f * TileMap->TileDepth;
    }
    // TODO: Set all the tiles covered by the component.
    SetTileContents(World->Arena, TileMap, &MouseSelector->TilePos, PreviousContent);
    
  }
  

  if(Pushed(MouseSelector->RightButton) && MouseSelector->SelectedContent.ElectricalComponentEntity.EntityID > 0)
  {
    DeleteEntity(EM, &MouseSelector->SelectedContent.ElectricalComponentEntity);
    MouseSelector->SelectedContent = {};
  }


}
