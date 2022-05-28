#include "component_camera.h"
#include "entity_components.h"
#include "breadboard_tile.h"
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

void ControllerSystemUpdate( world* World )
{
  TIMED_FUNCTION();

  entity_manager* EM = GlobalGameState->EntityManager;
  BeginScopedEntityManagerMemory();
  component_result* Components = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA | COMPONENT_FLAG_CONTROLLER);
  while(Next(EM, Components))
  {
    component_controller* Controller = GetControllerComponent(Components);
    switch (Controller->Type)
    {
      case ControllerType_FlyingCamera:
      {
        component_camera* Camera = GetCameraComponent(Components);
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
        
        TileMap->MousePosition = CanonicalizePosition( TileMap, V3(MousePosWorldSpace.X, MousePosWorldSpace.Y,0));

        MouseSelector->LeftButton = Mouse->Button[PlatformMouseButton_Left];
        MouseSelector->RightButton = Mouse->Button[PlatformMouseButton_Right];
        MouseSelector->CanPos = V2(Mouse->X, Mouse->Y);
        MouseSelector->ScreenPos = MouseScreenSpace;
        MouseSelector->WorldPos = MousePosWorldSpace;
        MouseSelector->TilePos = TileMap->MousePosition;

        tile_contents HotSelection = GetTileContents(TileMap, TileMap->MousePosition);

        if(MouseSelector->SelectedContent.Component)
        {
          HotSelection = MouseSelector->SelectedContent;
        }

        if(Released(MouseSelector->LeftButton))
        {
          tile_contents PreviousContent = MouseSelector->SelectedContent;
          MouseSelector->SelectedContent = GetTileContents(TileMap, TileMap->MousePosition);
          SetTileContents(World->Arena, TileMap, &MouseSelector->TilePos, PreviousContent);
        }
        if(Pushed(MouseSelector->RightButton))
        {
          // TODO: At the moment this leaks memory. Fix this with some smart memory system.
          //       Or something dumb, like have a separate arena for tile-map stuff and after
          //       a set amount of fragmentation from "leaked" memory just reallocate everything
          //       in that map or chunk or whaterver.
          //       Anyway, do something. This is just for demoing atm.
          MouseSelector->SelectedContent = {};
        }

        if(Pushed(Keyboard->Key_Q))
        {
          // Rotate Left
          if(HotSelection.Component)
          {
            HotSelection.Component->Rotation += Tau32/4.f;
            if(HotSelection.Component->Rotation > Pi32)
            {
              HotSelection.Component->Rotation -= Tau32;
            }else if(HotSelection.Component->Rotation < -Pi32)
            {
              HotSelection.Component->Rotation += Tau32;
            }
          }
        }
        if(Pushed(Keyboard->Key_W))
        {
          // Rotate Right
          if(HotSelection.Component)
          {
            HotSelection.Component->Rotation -= Tau32/4.f;
            if(HotSelection.Component->Rotation > Pi32)
            {
              HotSelection.Component->Rotation -= Tau32;
            }else if(HotSelection.Component->Rotation < -Pi32)
            {
              HotSelection.Component->Rotation += Tau32;
            }
          }
        }


        electrical_component** Component = &MouseSelector->SelectedContent.Component;
        if(!*Component)
        {

          if(Pushed(Keyboard->Key_S))
          {
            *Component = PushStruct(World->Arena, electrical_component);
            (*Component)->Type = ElectricalComponentType_Source;
          }
          if(Pushed(Keyboard->Key_R))
          {
            *Component    = PushStruct(World->Arena, electrical_component);
            (*Component)->Type = ElectricalComponentType_Resistor;
          }
          if(Pushed(Keyboard->Key_L))
          {
            *Component      = PushStruct(World->Arena, electrical_component);
            (*Component)->Type = ElectricalComponentType_Led_Red;
          }
          if(Pushed(Keyboard->Key_G))
          {
            *Component     = PushStruct(World->Arena, electrical_component);
            (*Component)->Type = ElectricalComponentType_Ground;
          }
        }else
        {
          if(Pushed(Keyboard->Key_S))
          {
            (*Component)->Type = ElectricalComponentType_Source;
          }
          if(Pushed(Keyboard->Key_R))
          {
            (*Component)->Type = ElectricalComponentType_Resistor;
          }
          if(Pushed(Keyboard->Key_L))
          {
            (*Component)->Type = ElectricalComponentType_Led_Red;
          }
          if(Pushed(Keyboard->Key_G))
          {
            (*Component)->Type = ElectricalComponentType_Ground;
          }
        }
        
      }break;
      case ControllerType_Hero:
      {
        component_camera*   Camera   = GetCameraComponent(Components);
        // Handle Camera
      }break;
    }
  }
}
