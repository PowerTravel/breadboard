#include "component_camera.h"
#include "breadboard_entity_components.h"
#include "breadboard_tile.h"

// Input in Screen Space
internal void HandleZoom(component_camera* Camera, screen_coordinate MouseScreenPos, r32 ScrollWheel)
{
  if(ScrollWheel != 0)
  {
    const r32 AspectRatio = GameGetAspectRatio();

    // TODO:  - Do some slerping here, possibly tied to the scroll wheel, zoom in/zoom out
    r32 ZoomSpeed = 20; 
    v2 PrePos = GetMousePosInProjectionWindow(MouseScreenPos, Camera->OrthoZoom, AspectRatio);
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

    v2 PostPos = GetMousePosInProjectionWindow(MouseScreenPos, Camera->OrthoZoom, AspectRatio);

    v2 CamDelta = PostPos - PrePos;
    TranslateCamera(Camera, -V3(CamDelta,0));  
  }  
}

internal void HandleTranslate(component_camera* Camera, b32 MouseActive, screen_coordinate DeltaMouse)
{
  if(MouseActive)
  {
    rect2f ScreenRect = GetCameraScreenRect(Camera->OrthoZoom);

    DeltaMouse.X = 0.5f * DeltaMouse.X * ScreenRect.W;
    DeltaMouse.Y = 0.5f * DeltaMouse.Y * ScreenRect.H;

    TranslateCamera(Camera, V3(-DeltaMouse.X, -DeltaMouse.Y,0));
  }
}

void InitiateElectricalComponent(electrical_component* Component, component_hitbox* HitboxComponent, u32 ElectricalType)
{
  Component->Type = ElectricalType;
  u32 SpriteTileType = ElectricalComponentToSpriteType(Component);
  bitmap_points TilePoint = GetElectricalComponentSpriteBitmapPoints(SpriteTileType);

  // WC = WorldCoordinate
  // PS = PixelCoordinate
  // Rh = RightHanded (increasing y goes up)
  // Rh = LeftHanded  (increasing y goes down)
  bitmap_coordinate_rh TopLeftPS          = ToRightHanded(&TilePoint.TopLeft);
  bitmap_coordinate_rh BotRightPS         = ToRightHanded(&TilePoint.BotRight);
  bitmap_coordinate_rh RotationalCenterPS = ToRightHanded(&TilePoint.Center);
  bitmap_coordinate_rh BotLeftPS          = BitmapCoordinatePxRh(TopLeftPS.x,  BotRightPS.y, BotRightPS.w, BotRightPS.h);
  bitmap_coordinate_rh TopRightPS         = BitmapCoordinatePxRh(BotRightPS.x,  TopLeftPS.y, TopLeftPS.w, TopLeftPS.h);

  v2 BoxDimensionsWS          = Subtract(&TopRightPS, &BotLeftPS) / PIXELS_PER_UNIT_LENGTH;
  v2 GeometricCenterWS        = Add(&BotLeftPS, &TopRightPS) * 0.5f / PIXELS_PER_UNIT_LENGTH;
  v2 RotationalCenterWS       = V2( (r32) RotationalCenterPS.x, (r32) RotationalCenterPS.y) / PIXELS_PER_UNIT_LENGTH;
  v2 RotationalCenterOffsetWS = RotationalCenterWS - GeometricCenterWS;

  hitbox* MainHitbox = HitboxComponent->Box; 

  *MainHitbox = {};
  MainHitbox->W   = BoxDimensionsWS.X;
  MainHitbox->H   = BoxDimensionsWS.Y;
  MainHitbox->RotationCenterOffset = RotationalCenterOffsetWS;

  r32 ConnectorSideLength = 0.1;

  Assert(ArrayCount(TilePoint.Points) >= (ArrayCount(HitboxComponent->Box)-1));
  for(u32 idx = 1; idx < ArrayCount(HitboxComponent->Box); idx++)
  {
    bitmap_coordinate_lh* BitmapPointLh = TilePoint.Points + idx - 1;
    if(BitmapPointLh->w > 0)
    {
      bitmap_coordinate_rh BitmapPointRh = ToRightHanded(BitmapPointLh);
      v2 BitmapPointWS = V2( (r32) BitmapPointRh.x, (r32) BitmapPointRh.y) / PIXELS_PER_UNIT_LENGTH;
      hitbox* Hitbox = HitboxComponent->Box + idx;
      *Hitbox = {};
      Hitbox->W     = ConnectorSideLength;
      Hitbox->H     = ConnectorSideLength;

      v2 PosRelativeGeometricCenterWS = BitmapPointWS - GeometricCenterWS;
      Hitbox->Pos.X = {};
      Hitbox->Pos.Y = {};
      Hitbox->RotationCenterOffset = RotationalCenterOffsetWS - PosRelativeGeometricCenterWS;
    } 
  }

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

  r32 AspectRatio = GameGetAspectRatio();
  canonical_screen_coordinate MouseCanPos      = CanonicalScreenCoordinate(Mouse->X, Mouse->Y, AspectRatio);
  canonical_screen_coordinate MouseCanPosStart = CanonicalScreenCoordinate(Mouse->X - Mouse->dX, Mouse->Y - Mouse->dY, AspectRatio);
  screen_coordinate MouseScreenSpaceStart      = CanonicalToScreenSpace(MouseCanPosStart);
  screen_coordinate MouseScreenSpace           = CanonicalToScreenSpace(MouseCanPos);
  screen_coordinate MouseDeltaScreenSpace      = MouseScreenSpace - MouseScreenSpaceStart;

  HandleZoom(Camera, MouseScreenSpace, Mouse->dZ);
  HandleTranslate(Camera, Mouse->Button[PlatformMouseButton_Left].Active, MouseDeltaScreenSpace);

  tile_map* TileMap = &GlobalGameState->World->TileMap;

  v3 CamPos = GetPositionFromMatrix(&Camera->V);
  r32 OrthoZoom = Camera->OrthoZoom;
  world_coordinate MousePosWorldSpace = GetMousePosInWorld(CamPos, OrthoZoom, MouseScreenSpace);

  MouseSelector->LeftButton = Mouse->Button[PlatformMouseButton_Left];
  MouseSelector->RightButton = Mouse->Button[PlatformMouseButton_Right];
  MouseSelector->CanPos = MouseCanPos;
  MouseSelector->ScreenPos = MouseScreenSpace;
  MouseSelector->WorldPos = MousePosWorldSpace;
  MouseSelector->TilePos = CanonicalizePosition(TileMap, V3(MousePosWorldSpace.X, MousePosWorldSpace.Y,0));
  
  tile_contents HotSelection = GetTileContents(TileMap, MouseSelector->TilePos);
  
  if(MouseSelector->SelectedContent.ElectricalComponentEntity.EntityID)
  {
    HotSelection = MouseSelector->SelectedContent;
  }

  if(HotSelection.ElectricalComponentEntity.EntityID)
  {
    component_hitbox* Hitbox = GetHitboxComponent(&HotSelection.ElectricalComponentEntity);
    MouseSelector->Rotation = Hitbox->Main.Rotation;

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

    Hitbox->Main.Rotation = MouseSelector->Rotation;
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
      Hitbox->Main.Pos.X = MouseSelector->WorldPos.X;
      Hitbox->Main.Pos.Y = MouseSelector->WorldPos.Y;
      Hitbox->Main.Rotation = MouseSelector->Rotation;

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

    Hitbox->Main.Pos.X = MouseSelector->WorldPos.X;
    Hitbox->Main.Pos.Y = MouseSelector->WorldPos.Y;
    Hitbox->Main.Rotation = MouseSelector->Rotation;
  }

  if(Pushed(MouseSelector->LeftButton))
  {
    tile_contents PreviousContent = MouseSelector->SelectedContent;
    MouseSelector->SelectedContent = GetTileContents(TileMap, MouseSelector->TilePos);
    if(PreviousContent.ElectricalComponentEntity.EntityID)
    {
      component_hitbox* Hitbox = GetHitboxComponent(&PreviousContent.ElectricalComponentEntity);
      v3 Pos = V3(Hitbox->Main.Pos.X, Hitbox->Main.Pos.Y, 0);
      tile_map_position TilePos = CanonicalizePosition(TileMap, Pos);
      // TODO: We are casting a negative uint (which is a huge number) into a signed and relying on it being cast down to 
      //       the small negative number again. This is compiler dependant behaviour, Maybe fix this?
      Hitbox->Main.Pos.X = Round( ( (r32) ( (s32) TilePos.AbsTileX))) * TileMap->TileWidth + 0.5f * TileMap->TileWidth;
      Hitbox->Main.Pos.Y = Round( ( (r32) ( (s32) TilePos.AbsTileY))) * TileMap->TileHeight + 0.5f * TileMap->TileHeight;
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
