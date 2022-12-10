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

electrical_component_id_list GetElectricalComponentAt(entity_manager* EM, memory_arena* Arena, world_coordinate* WorldPos)
{
  filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_HITBOX);
  electrical_component_id_list Result = {};
  electrical_component_id_list_entry* Tail = 0;
  while(Next(&EntityIterator))
  {
    component_hitbox* Hitbox = GetHitboxComponent(&EntityIterator);
    if(Intersects(Hitbox, *WorldPos))
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

internal void CreateRectanglePinHitbox(ElectricalPinType Type, hitbox_rectangle* Rectangle, r32 RotationAroundCenter)
{
  switch(Type)
  {
    case ElectricalPinType::InputOutput:
    {

    }break;
    case ElectricalPinType::A:
    {

    }break;
    case ElectricalPinType::B:
    {

    }break;
    default:
    {
      // ElectricalPinType is not a rectangle
      Assert(0);
    }break;
  }
}


internal void UpdatePinHitboxPosition(ElectricalPinType Type, r32 X, r32 Y, r32 R, component_hitbox* PinHitbox)
{
  r32 Angle = 0;
  switch(Type)
  {
    case ElectricalPinType::Positive:
    case ElectricalPinType::Input:
    case ElectricalPinType::Sink:
    {
      Angle = PinHitbox->Triangle.Rotation + Pi32/6.f;
    }break;

    case ElectricalPinType::Negative:
    case ElectricalPinType::Output:
    case ElectricalPinType::Source:
    {
      Angle = PinHitbox->Triangle.Rotation - Pi32/6.f;
    }break;
    default:
    {
      Angle = PinHitbox->Triangle.Rotation;
    }break;
  }
  PinHitbox->Position.X = X + R * Cos(Angle);
  PinHitbox->Position.Y = Y + R * Sin(Angle);
}

internal component_hitbox CreateTrianglePinHitbox(ElectricalPinType Type, r32 RotationAroundCenter)
{
  component_hitbox Result = {};
  Result.Type = HitboxType::TRIANGLE;
  hitbox_triangle* Triangle = &Result.Triangle;
  Triangle->Base = 0.2;
  Triangle->Height = Triangle->Base * Sin(Pi32/3.f);
  Triangle->CenterPoint = 0.5;
  switch(Type)
  {
    case ElectricalPinType::Positive:
    case ElectricalPinType::Input:
    case ElectricalPinType::Sink:
    {
      Triangle->Rotation = RotationAroundCenter - Pi32/6.f;
    }break;

    case ElectricalPinType::Negative:
    case ElectricalPinType::Output:
    case ElectricalPinType::Source:
    {
      Triangle->Rotation = RotationAroundCenter + Pi32/6.f;
    }break;
    default:
    {
      // ElectricalPinType is not a triangle
      Assert(0);
    }break;
  }

  return Result;
}

internal void SetPositionOfConnectorPin(world_coordinate ElectricalComponentCenter, r32 ConnectorRadius, r32 Angle, component_hitbox* PinHitbox)
{
  PinHitbox->Position.X = ElectricalComponentCenter.X + ConnectorRadius * Cos(Angle);
  PinHitbox->Position.Y = ElectricalComponentCenter.Y + ConnectorRadius * Sin(Angle); 
}

entity_id CreateElectricalComponent(entity_manager* EM, keyboard_input* Keyboard, world_coordinate WorldPos)
{
  entity_id Result = NewEntity(EM, COMPONENT_FLAG_ELECTRICAL);
  component_electrical* Component = GetElectricalComponent(&Result);
  component_hitbox* ElectricalComponentHitbox = GetHitboxComponent(&Result);
  ElectricalComponentHitbox->Type = HitboxType::CIRCLE;
  ElectricalComponentHitbox->Position = WorldPos;
  ElectricalComponentHitbox->Circle.Radius = 0.5;

  r32 Thickness = 0.1;
  r32 ConnectorRadius = ElectricalComponentHitbox->Circle.Radius;

  if(Pushed(Keyboard->Key_S))
  {
    entity_id ConnectorPin = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Pin = GetConnectorPinComponent(&ConnectorPin);
    Pin->Type = ElectricalPinType::Source;
    Pin->Component = Component;

    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPin);
    r32 Angle = 0;
    *PinHitbox = CreateTrianglePinHitbox(Pin->Type, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, ConnectorRadius, Angle, PinHitbox);

    Component->Type = ElectricalComponentType_Source;
    Component->FirstPin = Pin;
  }
#if 0
  else if(Pushed(Keyboard->Key_R))
  {
    entity_id ConnectorPinA = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* PinA = GetConnectorPinComponent(&ConnectorPinA);
    PinA->Type = ElectricalPinType::InputOutput;
    PinA->Component = Component;

    r32 Angle = Pi32;
    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPinA);
    hitbox_rectangle* Rectangle = &PinHitbox->Rectangle;
    CreateRectanglePinHitbox(PinA->Type, Rectangle, Angle);

    PinHitbox->Position.X = ElectricalComponentHitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = ElectricalComponentHitbox->Position.Y + ConnectorRadius * Sin(Angle);


    entity_id ConnectorPinB = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* PinB = GetConnectorPinComponent(&ConnectorPinB);
    PinB->Type = ElectricalPinType::InputOutput;
    PinB->Component = Component;

    Angle = 0;
    PinHitbox = GetHitboxComponent(&ConnectorPinB);
    Rectangle = &PinHitbox->Rectangle;
    CreateRectanglePinHitbox(PinB->Type, Rectangle, Angle);

    PinHitbox->Position.X = ElectricalComponentHitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = ElectricalComponentHitbox->Position.Y + ConnectorRadius * Sin(Angle);

    PinA->NextPin = PinB;
    Component->Type = ElectricalComponentType_Resistor;
    Component->FirstPin = PinA;
  }
  else if(Pushed(Keyboard->Key_L))
  {

    entity_id ConnectorPinAnode = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Anode = GetConnectorPinComponent(&ConnectorPinAnode);
    Anode->Type = ElectricalPinType::Positive;
    Anode->Component = Component;
    Component->FirstPin = Anode;

    r32 Angle = Pi32;
    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPinAnode);
    *PinHitbox = CreateTrianglePinHitbox(Pin->Type, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, ConnectorRadius, Angle, PinHitbox);

    entity_id ConnectorPinCathode = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Cathode = GetConnectorPinComponent(&ConnectorPinCathode);
    Cathode->Type = ElectricalPinType::Negative;
    Cathode->Component = Component;

    Angle = 0;
    PinHitbox = GetHitboxComponent(&ConnectorPinCathode);
    *PinHitbox = CreateTrianglePinHitbox(Pin->Type, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, ConnectorRadius, Angle, PinHitbox);

    PinHitbox->Position.X = ElectricalComponentHitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = ElectricalComponentHitbox->Position.Y + ConnectorRadius * Sin(Angle);

    Anode->NextPin = Cathode;
    Component->Type = ElectricalComponentType_Diode;
    Component->FirstPin = Anode;
  }
  if(Pushed(Keyboard->Key_G))
  {
    entity_id ConnectorPin = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Pin = GetConnectorPinComponent(&ConnectorPin);
    Pin->Type = ElectricalPinType::Sink;
    Pin->Component = Component;
    
    r32 Angle = Pi32;
    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPin);
    hitbox_triangle* Triangle = &PinHitbox->Triangle;
    CreateTrianglePinHitbox(Pin->Type, Triangle, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, Angle, PinHitbox);

    PinHitbox->Position.X = Hitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = Hitbox->Position.Y + ConnectorRadius * Sin(Angle);

    Component->Type = ElectricalComponentType_Ground;
    Component->FirstPin = Pin;
  }
  #endif

  return Result;
}

void DeleteElectricalEntity(entity_manager* EM, entity_id ElectricalComponent)
{
  component_electrical* Component = GetElectricalComponent(&ElectricalComponent);
  component_connector_pin* Pin = Component->FirstPin;
  while(Pin)
  {
    component_connector_pin* NextPin = Pin->NextPin;
    entity_id PinID = GetEntityIDFromComponent( (bptr) Pin);
    DeleteEntity(EM, &PinID);
    Pin = NextPin;
  }

  DeleteEntity(EM, &ElectricalComponent);
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
  electrical_component_id_list HotSelectionList = GetElectricalComponentAt(EM, GlobalGameState->TransientArena, &MouseSelector->WorldPos);
  
  // Cursor is holding an electrical component
  if(IsValid(&MouseSelector->HotSelection))
  {
    // Actions:
    //  - Swap it with what's beneath (left mouse, occupied underneath, swaps with closest component as measured from rotation point)  
    //  - Place it on empty spot      (left mouse, empty underneath)
    //  - Turn into another           (S, R, L, G)
    // Update the position:
    {
      component_hitbox* Hitbox = GetHitboxComponent(&MouseSelector->HotSelection);
      Hitbox->Position = MousePosWorldSpace;

      if(HasComponents(EM, &MouseSelector->HotSelection, COMPONENT_FLAG_ELECTRICAL))
      {
        // Update position of a selected electrical component
        component_electrical* ElecticalComponent = GetElectricalComponent(&MouseSelector->HotSelection);
        component_connector_pin* Pin = ElecticalComponent->FirstPin;
        while(Pin)
        {
          entity_id PinID = GetEntityIDFromComponent((bptr)Pin);
          component_hitbox* PinHitbox = GetHitboxComponent(&PinID);
          
          UpdatePinHitboxPosition(Pin->Type, Hitbox->Position.X, Hitbox->Position.Y, Hitbox->Circle.Radius, PinHitbox);
          Pin = Pin->NextPin;
        }
      }
      else if(HasComponents(EM, &MouseSelector->HotSelection, COMPONENT_FLAG_CONNECTOR_PIN))
      {
        // Update position of a selected connector pin
        component_connector_pin* ConnectorPin = GetConnectorPinComponent(&MouseSelector->HotSelection);
        entity_id ComponentID = GetEntityIDFromComponent((bptr)ConnectorPin->Component);
        component_electrical* ElecticalComponent = GetElectricalComponent(&ComponentID);
        component_hitbox* ElectricalComponentHitbox = GetHitboxComponent(&ComponentID);
        world_coordinate ComponentCenter = ElectricalComponentHitbox->Position;
        world_coordinate HorizontalLine = V3(1,0,0);
        world_coordinate ComponentToMouse = Normalize(MousePosWorldSpace - ComponentCenter);
        r32 CosAngle = HorizontalLine*ComponentToMouse;
        r32 Angle = ACos(CosAngle);
        Assert(ElectricalComponentHitbox->Type == HitboxType::CIRCLE);

        component_hitbox* PinHitbox = GetHitboxComponent(&MouseSelector->HotSelection);
        PinHitbox->Triangle.Rotation = Angle;
        UpdatePinHitboxPosition(ConnectorPin->Type, ComponentCenter.X, ComponentCenter.Y, ElectricalComponentHitbox->Circle.Radius, PinHitbox);
      }
    }

    // Left Mouse:
    if(Pushed(MouseSelector->LeftButton))
    {
      if(HotSelectionList.Count > 1)
      {
        // Find closest component
        electrical_component_id_list_entry* Entry = HotSelectionList.First;
        entity_id ClosestComponent = {};
        component_hitbox* SelectedHitbox = GetHitboxComponent(&MouseSelector->HotSelection);
        v2 SelectedPosition = V2(SelectedHitbox->Position.X, SelectedHitbox->Position.Y);
        r32 ClosestDistance = R32Max;
        
        while(Entry)
        {
          if(!Compare(&MouseSelector->HotSelection, &Entry->ID))
          {
            component_hitbox* Hitbox = GetHitboxComponent(&Entry->ID);
            v2 Position = V2(Hitbox->Position.X, Hitbox->Position.Y);
            r32 Distance = Norm(Position - SelectedPosition);
            if(ClosestDistance > Distance)
            {
              Distance = ClosestDistance;
              ClosestComponent = Entry->ID;
            }
          }

          Entry = Entry->Next;
        }

        if(IsValid(&ClosestComponent))
        {
          component_electrical* ElecticalComponent = GetElectricalComponent(&MouseSelector->HotSelection);
          #if 1
          // Swap places of components if we have standardized component sizes?
          component_hitbox* Hitbox = GetHitboxComponent(&ClosestComponent);
          SelectedHitbox->Position = Hitbox->Position;
          Hitbox->Position = MousePosWorldSpace;
          #endif
          MouseSelector->HotSelection = ClosestComponent;
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
        
        if(HasComponents(EM, &MouseSelector->HotSelection, COMPONENT_FLAG_ELECTRICAL))
        {
          Hitbox->Position = MousePosWorldSpace;
        }
        else if(HasComponents(EM, &MouseSelector->HotSelection, COMPONENT_FLAG_CONNECTOR_PIN))
        {
          component_connector_pin* ConnectorPin = GetConnectorPinComponent(&MouseSelector->HotSelection);
          entity_id ComponentID = GetEntityIDFromComponent((bptr)ConnectorPin->Component);
          component_hitbox* ElectricalComponentHitbox = GetHitboxComponent(&ComponentID);
          world_coordinate ComponentCenter = ElectricalComponentHitbox->Position;
          world_coordinate HorizontalLine = V3(1,0,0);
          world_coordinate ComponentToMouse = Normalize(MousePosWorldSpace - ComponentCenter);
          r32 CosAngle = HorizontalLine*ComponentToMouse;
          r32 Angle = ACos(CosAngle);
          Platform.DEBUGPrint("Angle %f\n", Angle);

          Assert(ElectricalComponentHitbox->Type == HitboxType::CIRCLE);

          Hitbox->Triangle.Rotation = Angle;

          UpdatePinHitboxPosition(ConnectorPin->Type, ComponentCenter.X, ComponentCenter.Y, ElectricalComponentHitbox->Circle.Radius, Hitbox);
        }

        MouseSelector->HotSelection = {};
      }
    }
    // Delete Electrical component
    else if(Pushed(MouseSelector->RightButton))
    {
      DeleteEntity(EM, &MouseSelector->HotSelection);
      MouseSelector->HotSelection = {};
    }
    // Turn into another  
    else if(Pushed(Keyboard->Key_S) || Pushed(Keyboard->Key_R) || Pushed(Keyboard->Key_L) || Pushed(Keyboard->Key_G))
    {
      DeleteElectricalEntity(EM, MouseSelector->HotSelection);
      MouseSelector->HotSelection = CreateElectricalComponent(EM, Keyboard, MouseSelector->WorldPos);
    }
  }
  // Cursor is not holding an electrical component but is hovering over one
  else if(HotSelectionList.Count)
  {
    // Actions:
    //  - Create a new electrical component (S, R, L, G)
    //  - Pick it up  (left mouse)

    //  Create a new electrical component
    if(Pushed(Keyboard->Key_S) || Pushed(Keyboard->Key_R) || Pushed(Keyboard->Key_L) || Pushed(Keyboard->Key_G))
    {
      MouseSelector->HotSelection = CreateElectricalComponent(EM, Keyboard, MouseSelector->WorldPos);  
    }
    // Pick it up
    else if(Pushed(MouseSelector->LeftButton))
    {
      MouseSelector->HotSelection = HotSelectionList.First->ID;
    }

  }else{
    // Cursor is not holding an electrical component and is not is hovering over one
    // Actions:
    //  - Create a new electrical component (S, R, L, G)

    // (S, R, L, G)
    if(Pushed(Keyboard->Key_S) || Pushed(Keyboard->Key_R) || Pushed(Keyboard->Key_L) || Pushed(Keyboard->Key_G))
    {
      //  - Create a new electrical component
      MouseSelector->HotSelection = CreateElectricalComponent(EM, Keyboard, MouseSelector->WorldPos);  
    }
  }

  EndTemporaryMemory(TempMem);
}
