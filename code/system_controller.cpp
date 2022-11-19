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

struct electrical_component_id_list_entry
{
  entity_id* ID;
  electrical_component_id_list_entry* Next;
};

struct electrical_component_id_list
{
  u32 Count; 
  electrical_component_id_list_entry* First;
};

electrical_component_id_list GetElectricalComponentAt(entity_manager* EM, memory_arena* Arena, world_coordinate* WorldPos)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_ELECTRICAL);
  electrical_component_id_list Result = {};
  electrical_component_id_list_entry* Tail = 0;
  while(Next(&EntityIterator))
  {
    component_hitbox* Hitbox = GetHitboxComponent(&EntityIterator);
    if(Intersects_XYPlane(&Hitbox->Main, WorldPos))
    {
      ++Result.Count;
      if(!Result.First)
      {
        Result.First = PushStruct(Arena, electrical_component_id_list_entry);
        Tail = Result.First;
      }else{
        Tail->Next = PushStruct(Arena, electrical_component_id_list_entry);
        Tail = Tail->Next; 
      }
      Tail->ID = GetEntityID(&EntityIterator);
    }
  }

  return Result;
}

entity_id InitiateElectricalEntity(entity_manager* EM, entity_id ElectricalEntity, keyboard_input* Keyboard, world_coordinate WorldPos, r32 Rotation)
{
  entity_id Result = ElectricalEntity;
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
  
  if(Type != ElectricalComponentType_None)
  {
    if(!IsValid(&Result))
    {
      Result = NewEntity(EM);
      NewComponents(EM, &Result, COMPONENT_FLAG_ELECTRICAL);
    }

    electrical_component* Component = GetElectricalComponent(&Result);
    component_hitbox* Hitbox = GetHitboxComponent(&Result);
    InitiateElectricalComponent(Component, Hitbox, Type);
    Hitbox->Main.Pos = WorldPos;
    Hitbox->Main.Rotation = Rotation;
  }

  return Result;
}

internal void RotateElectricalComponent(entity_id* ElectricalComponent, mouse_selector* MouseSelector, keyboard_input* Keyboard)
{
  component_hitbox* Hitbox = GetHitboxComponent(ElectricalComponent);
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

  electrical_component_id_list HotSelectionList = GetElectricalComponentAt(EM, GlobalGameState->TransientArena, &MouseSelector->WorldPos);
  
  // Cursor is holding an electrical component
  if(IsValid(&MouseSelector->HotSelection))
  {
    // Actions:
    //  - Swap it with what's beneath (left mouse, occupied underneath, swaps with closest component as measured from rotation point)  
    //  - Place it on empty spot      (left mouse, empty underneath)
    //  - Turn into another           (S, R, L, G)
    //  - Rotate                      (Q or E)
    // Update the position:
    {
      component_hitbox* Hitbox = GetHitboxComponent(&MouseSelector->HotSelection);
      Hitbox->Main.Pos = MousePosWorldSpace;
    }

    // Left Mouse:
    if(Pushed(MouseSelector->LeftButton))
    {
      if(HotSelectionList.Count > 1)
      {
        // Find closest component
        electrical_component_id_list_entry* Entry = HotSelectionList.First;
        entity_id* ClosestComponent = 0;
        component_hitbox* SelectedHitbox = GetHitboxComponent(&MouseSelector->HotSelection);
        v2 SelectedPosition = V2(SelectedHitbox->Main.Pos.X, SelectedHitbox->Main.Pos.Y);
        r32 ClosestDistance = R32Max;
        
        while(Entry)
        {
          if(!Compare(&MouseSelector->HotSelection, Entry->ID))
          {
            component_hitbox* Hitbox = GetHitboxComponent(Entry->ID);
            v2 Position = V2(Hitbox->Main.Pos.X, Hitbox->Main.Pos.Y);
            r32 Distance = Norm(Position - SelectedPosition);
            if(ClosestDistance > Distance)
            {
              Distance = ClosestDistance;
              ClosestComponent = Entry->ID;
            }
          }

          Entry = Entry->Next;
        }

        if(ClosestComponent)
        {
          #if 0
          // Swap places of components if we have standardized component sizes?
          component_hitbox* Hitbox = GetHitboxComponent(ClosestComponent);
          SelectedHitbox->Main.Pos = Hitbox->Main.Pos;
          Hitbox->Main.Pos = MousePosWorldSpace;
          #endif
          MouseSelector->HotSelection = *ClosestComponent;
        }

      }else{
        // Place
        world_coordinate PlacementSpot = MousePosWorldSpace;
        #if 0
        // TODO: Hold <ALT> to bring up alignment grids to snap the placement
        if(Keyboard->Key_ALT.Active)
        {
          PlacementSpot = FindNearestGrid();
        }
        #endif
        
        component_hitbox* Hitbox = GetHitboxComponent(&MouseSelector->HotSelection);
        Hitbox->Main.Pos = MousePosWorldSpace;
        MouseSelector->HotSelection = {};
      }
    }
    // Delete Electrical component
    else if(Pushed(MouseSelector->RightButton))
    {
      DeleteEntity(EM, &MouseSelector->HotSelection);
      MouseSelector->HotSelection = {};
    }
    // Rotate 
    else if(Pushed(Keyboard->Key_Q) || Pushed(Keyboard->Key_E))
    {
      RotateElectricalComponent(&MouseSelector->HotSelection, MouseSelector, Keyboard);
    }
    // Turn into another  
    else if(Pushed(Keyboard->Key_S) || Pushed(Keyboard->Key_R) || Pushed(Keyboard->Key_L) || Pushed(Keyboard->Key_G))
    {
      MouseSelector->HotSelection = InitiateElectricalEntity(EM ,MouseSelector->HotSelection, Keyboard, MouseSelector->WorldPos, MouseSelector->Rotation);  
    }
  }
  // Cursor is not holding an electrical component but is hovering over one
  else if(HotSelectionList.Count)
  {
    // Actions:
    //  - Create a new electrical component (S, R, L, G)
    //  - Rotate it   (Q, E)
    //  - Pick it up  (left mouse)

    //  Create a new electrical component
    if(Pushed(Keyboard->Key_S) || Pushed(Keyboard->Key_R) || Pushed(Keyboard->Key_L) || Pushed(Keyboard->Key_G))
    {
      MouseSelector->HotSelection = InitiateElectricalEntity(EM ,MouseSelector->HotSelection, Keyboard, MouseSelector->WorldPos, MouseSelector->Rotation);  
    }
    // Rotate
    else if(Pushed(Keyboard->Key_Q) || Pushed(Keyboard->Key_E))
    {
      RotateElectricalComponent(HotSelectionList.First->ID, MouseSelector, Keyboard);
    }
    // Pick it up
    else if(Pushed(MouseSelector->LeftButton))
    {
      MouseSelector->HotSelection = *HotSelectionList.First->ID;
    }

  }else{
    // Cursor is not holding an electrical component and is not is hovering over one
    // Actions:
    //  - Create a new electrical component (S, R, L, G)

    // (S, R, L, G)
    if(Pushed(Keyboard->Key_S) || Pushed(Keyboard->Key_R) || Pushed(Keyboard->Key_L) || Pushed(Keyboard->Key_G))
    {
      //  - Create a new electrical component
      MouseSelector->HotSelection = InitiateElectricalEntity(EM ,MouseSelector->HotSelection, Keyboard, MouseSelector->WorldPos, MouseSelector->Rotation);  
    }
  }
}
