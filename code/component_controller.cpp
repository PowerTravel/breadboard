#include "component_controller.h"
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
    world_coordinate PrePos = ToWorldCoordinateRelativeCameraPosition(MouseScreenPos, Camera->OrthoZoom, AspectRatio);
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

    world_coordinate PostPos = ToWorldCoordinateRelativeCameraPosition(MouseScreenPos, Camera->OrthoZoom, AspectRatio);

    world_coordinate CamDelta = PostPos - PrePos;
    TranslateCamera(Camera, -CamDelta);  
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

struct electrical_component_id_list_entry
{
  entity_id ID;
  electrical_component_id_list_entry* Next;
};

struct electrical_component_id_list
{
  u32 Count; 
  electrical_component_id_list_entry* First;
};

inline internal b32 EntryInBlacklist(entity_id* ID, u32 BlacklistCount, entity_id* BlackList)
{
  for (u32 Index = 0; Index < BlacklistCount; ++Index)
  {
    if(Compare(BlackList + Index, ID))
    {
      return true;
    }
  }
  return false;
}

electrical_component_id_list GetElectricalComponentAt(entity_manager* EM, memory_arena* Arena, world_coordinate* WorldPos, u32 BlacklistCount, entity_id* BlackList)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_HITBOX);
  electrical_component_id_list Result = {};
  electrical_component_id_list_entry* Tail = 0;
  while(Next(&EntityIterator))
  {
    component_hitbox* Hitbox = GetHitboxComponent(&EntityIterator);
    entity_id ID = GetEntityID(&EntityIterator);
    b32 Blacklisted = EntryInBlacklist(&ID, BlacklistCount, BlackList);
    
    if(!Blacklisted && Intersects(Hitbox, *WorldPos))
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
      Tail->ID = ID;
    }
  }

  return Result;
}

internal void FilterOutBlacklistedEntries(electrical_component_id_list* List, u32 BlacklistCount, entity_id* BlackList)
{
  electrical_component_id_list Result = {};
  electrical_component_id_list_entry* Entry = List->First;
  electrical_component_id_list_entry* PreviousEntry = 0;
  while(Entry)
  {
    if(EntryInBlacklist(&Entry->ID, BlacklistCount, BlackList))
    {
      if(PreviousEntry)
      {
        PreviousEntry->Next = Entry->Next;
      }else{
        List->First = Entry->Next;
      }
    }else{
      PreviousEntry = Entry;
    }
    Entry = Entry->Next;
  }
}

internal entity_id GetClosestComponent(electrical_component_id_list* HotSelectionList, world_coordinate PositionToQuery)
{
  entity_id Result = {};
  electrical_component_id_list_entry* Entry = HotSelectionList->First;
  r32 ClosestDistance = R32Max;

  while(Entry)
  {
    component_hitbox* Hitbox = GetHitboxComponent(&Entry->ID);
    Assert(Hitbox);
    v3 Position = V3(Hitbox->Position->AbsolutePosition.X, Hitbox->Position->AbsolutePosition.Y, 0);
    r32 Distance = Norm(Position - PositionToQuery);
    if(Distance < ClosestDistance)
    {
      ClosestDistance = Distance;
      Result = Entry->ID;
    }

    Entry = Entry->Next;
  }

  return Result;
}

entity_id GetClosestComponentOfType(entity_manager* EM, electrical_component_id_list* HotSelectionList, world_coordinate PositionToQuery, u32 ComponentFlag)
{
  entity_id Result = {};
  electrical_component_id_list_entry* Entry = HotSelectionList->First;
  r32 ClosestDistance = R32Max;

  while(Entry)
  {
    if(HasComponents(EM, &Entry->ID, ComponentFlag))
    {
      component_hitbox* Hitbox = GetHitboxComponent(&Entry->ID);
      Assert(Hitbox);
      v3 Position = V3(Hitbox->Position->AbsolutePosition.X, Hitbox->Position->AbsolutePosition.Y, 0);
      r32 Distance = Norm(Position - PositionToQuery);
      if(Distance < ClosestDistance)
      {
        ClosestDistance = Distance;
        Result = Entry->ID;
      }
    }

    Entry = Entry->Next;
  }

  return Result;
}

internal inline entity_id GetElectricalComponentIDFromPin(component_connector_pin* ConnectorPin)
{
  entity_id Result = GetEntityIDFromComponent((bptr)ConnectorPin->Component);
  return Result;
}

internal void UpdatePinHitboxPosition(r32 R, r32 Angle, component_hitbox* PinHitbox)
{
  Assert(PinHitbox->Position->Parent);
  r32 X = R * Cos(Angle);
  r32 Y = R * Sin(Angle);
  SetRelativePosition(PinHitbox->Position->Parent, V3(X, Y, 0), Angle);
}

internal r32 GetAngleOfMouseRelativeElectricalComponent(world_coordinate MousePosWorldSpace, entity_id ElectricalComponentID)
{
  component_position* ElectricalComponentPosition = GetPositionComponent(&ElectricalComponentID);
  world_coordinate RelativeMousePosition = Normalize(GetPositionRelativeTo(ElectricalComponentPosition, MousePosWorldSpace));
  world_coordinate HorizontalLine = V3(1,0,0);
  r32 HorizontalCos = HorizontalLine*RelativeMousePosition;
  r32 VerticalCos = V3(0,1,0)*RelativeMousePosition;
  r32 Angle = ACos(HorizontalCos);  
  if(VerticalCos < 0)
  {
    Angle = Tau32 - Angle;
  }

  return Angle;
}

internal void UpdatePinHitboxPosition(entity_id* ConnectorPinID, world_coordinate Position)
{
  component_connector_pin* ConnectorPin = GetConnectorPinComponent(ConnectorPinID);
  Assert(ConnectorPin);
  entity_id ElectricalComponentID = GetElectricalComponentIDFromPin(ConnectorPin);
  r32 Angle = GetAngleOfMouseRelativeElectricalComponent(Position, ElectricalComponentID);

  component_hitbox* ElectricalComponentHitbox = GetHitboxComponent(&ElectricalComponentID);
  Assert(ElectricalComponentHitbox->Type == HitboxType::CIRCLE);

  component_hitbox* PinHitbox = GetHitboxComponent(ConnectorPinID);

  UpdatePinHitboxPosition(ElectricalComponentHitbox->Circle.Radius, Angle, PinHitbox);
}

ElectricalComponentType KeyPressToElectricalComponentType(keyboard_input* Keyboard)
{
  ElectricalComponentType Result = ElectricalComponentType::None;
  if(Pushed(Keyboard->Key_S))
  {
    Result = ElectricalComponentType::Source;
  }
  else if(Pushed(Keyboard->Key_R))
  {
    Result = ElectricalComponentType::Resistor;
  }
  else if(Pushed(Keyboard->Key_L))
  {
    Result = ElectricalComponentType::Diode;
  }
  else if(Pushed(Keyboard->Key_G))
  {
    Result = ElectricalComponentType::Ground;
  }
  return Result;
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
  world_coordinate MousePosWorldSpace = ToWorldCoordinate(MouseScreenSpace, CamPos, OrthoZoom, GameGetAspectRatio());

  MouseSelector->LeftButton = Mouse->Button[PlatformMouseButton_Left];
  MouseSelector->RightButton = Mouse->Button[PlatformMouseButton_Right];
  MouseSelector->CanPos = MouseCanPos;
  MouseSelector->ScreenPos = MouseScreenSpace;
  MouseSelector->WorldPos = MousePosWorldSpace;
  MouseSelector->TilePos = CanonicalizePosition(TileMap, V3(MousePosWorldSpace.X, MousePosWorldSpace.Y,0));

  temporary_memory TempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);
  

  u32 BlacklistCount = 0;
  entity_id* Blacklist = 0;

  if(IsValid(&MouseSelector->HotSelection))
  {
    BlacklistCount = 1;
    Blacklist = &MouseSelector->HotSelection;
  }

  electrical_component_id_list HotSelectionList = GetElectricalComponentAt(EM, GlobalGameState->TransientArena, &MouseSelector->WorldPos, BlacklistCount, Blacklist);
  
  const ElectricalComponentType EComponentType = KeyPressToElectricalComponentType(Keyboard);

  // Cursor is holding a hitbox
  if(IsValid(&MouseSelector->HotSelection))
  {
    entity_id * SelectedEntityID = &MouseSelector->HotSelection;

    // Actions:
    //  - Swap it with what's beneath (left mouse, occupied underneath, swaps with closest component as measured from rotation point)  
    //  - Place it on empty spot      (left mouse, empty underneath)
    //  - Turn into another           (S, R, L, G)
    // Update the position:
    {
      component_hitbox* SelectedHitbox = GetHitboxComponent(SelectedEntityID);
      if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_ELECTRICAL))
      {
        // Update position of a selected electrical component
        SetRelativePosition(SelectedHitbox->Position, MousePosWorldSpace, 0);
      }
      else if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_CONNECTOR_PIN))
      {
        // Update position of a selected connector pin relative Electrical component center
        UpdatePinHitboxPosition(SelectedEntityID, MousePosWorldSpace);
      }

      UpdateAbsolutePosition(GlobalGameState->TransientArena, SelectedHitbox->Position);
    }

    // Left Mouse:
    if(Pushed(MouseSelector->LeftButton))
    {
      // Holding something in the mouse and clicking left Mouse while Mousing over a component
      if(HotSelectionList.Count)
      {
        // Selected component is an Electrical Component
        if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_ELECTRICAL))
        {
          component_hitbox* SelectedHitbox = GetHitboxComponent(SelectedEntityID);
          world_coordinate SelectedPosition = GetAbsolutePosition(SelectedHitbox->Position);
          entity_id ClosestComponent = GetClosestComponentOfType(EM, &HotSelectionList, SelectedPosition, COMPONENT_FLAG_ELECTRICAL);

          if(IsValid(&ClosestComponent))
          {
            Assert(IsValid(&ClosestComponent) && HasComponents(EM, &ClosestComponent, COMPONENT_FLAG_ELECTRICAL));

            component_electrical* ElecticalComponent = GetElectricalComponent(SelectedEntityID);

            // Swap places of components if we have standardized component sizes?
            component_hitbox* ClosestComponentHitbox = GetHitboxComponent(&ClosestComponent);
            world_coordinate ClosestComponentAbsolutePosition = GetAbsolutePosition(ClosestComponentHitbox->Position);
            r32 ClosestComponentAbsoluteRotation =  GetAbsoluteRotation(ClosestComponentHitbox->Position);
            

            world_coordinate SelectedComponentAbsolutePosition = GetAbsolutePosition(SelectedHitbox->Position); 
            r32 SelectedComponentAbsoluteRotation = GetAbsoluteRotation(SelectedHitbox->Position);

            SetRelativePosition(ClosestComponentHitbox->Position, SelectedComponentAbsolutePosition, SelectedComponentAbsoluteRotation);
            SetRelativePosition(SelectedHitbox->Position, ClosestComponentAbsolutePosition, ClosestComponentAbsoluteRotation);
            
            MouseSelector->HotSelection = ClosestComponent;
          }
        }
        // Selected component is a connector pin
        else if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_CONNECTOR_PIN))
        {
          UpdatePinHitboxPosition(SelectedEntityID, MousePosWorldSpace);
          MouseSelector->HotSelection = {};
        }
      // Holding something in the mouse and clicking left Mouse while NOT Mousing over a component
      }else{
        // Place        
        if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_ELECTRICAL))
        {
          component_hitbox* ElectricalComponentHitbox = GetHitboxComponent(SelectedEntityID);
          ElectricalComponentHitbox->Position->AbsolutePosition = MousePosWorldSpace;
          MouseSelector->HotSelection = {};
        }
        else if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_CONNECTOR_PIN))
        {
          UpdatePinHitboxPosition(SelectedEntityID, MousePosWorldSpace);
          MouseSelector->HotSelection = {};
        }
      }
    }
    // Delete Electrical component
    else if(Pushed(MouseSelector->RightButton))
    {
      if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_ELECTRICAL))
      {
        DeleteElectricalEntity(EM, MouseSelector->HotSelection);
        MouseSelector->HotSelection = {};
      }else if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_CONNECTOR_PIN)){
        MouseSelector->HotSelection = {};
      }
    }
    // Turn into another  
    else if(EComponentType != ElectricalComponentType::None)
    {
      if(HasComponents(EM, SelectedEntityID, COMPONENT_FLAG_ELECTRICAL))
      {
        DeleteElectricalEntity(EM, MouseSelector->HotSelection);
        MouseSelector->HotSelection = CreateElectricalComponent(EM, EComponentType, MousePosWorldSpace);
      }
    }
  }
  // Cursor is not holding an electrical component but is hovering over one
  else if(HotSelectionList.Count)
  {
    // Actions:
    //  - Create a new electrical component (S, R, L, G)
    //  - Pick it up  (left mouse)

    //  Create a new electrical component
    if(EComponentType != ElectricalComponentType::None)
    {
      MouseSelector->HotSelection = CreateElectricalComponent(EM, EComponentType, MouseSelector->WorldPos);  
    }
    // Pick it up
    else if(Pushed(MouseSelector->LeftButton))
    {
      MouseSelector->HotSelection = GetClosestComponent(&HotSelectionList, MousePosWorldSpace);
    }

  }else{
    // Cursor is not holding an electrical component and is not is hovering over one
    // Actions:
    //  - Create a new electrical component (S, R, L, G)

    // (S, R, L, G)
    if(EComponentType != ElectricalComponentType::None)
    {
      //  - Create a new electrical component
      MouseSelector->HotSelection = CreateElectricalComponent(EM, EComponentType, MouseSelector->WorldPos);
    }
  }

  EndTemporaryMemory(TempMem);
}
