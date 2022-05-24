#include "component_camera.h"
#include "entity_components.h"
#include "breadboard_tile.h"

// Mouse Input in Screen Space
// Cam Pos in World Space
internal v2
GetMousePosInProjectionWindow(r32 MouseX, r32 MouseY, r32 Zoom, r32 AspectRatio)
{
  rect2f ScreenRect = GetCameraScreenRect(Zoom, AspectRatio);
  v2 Result = {};

  Result.X = 0.5f * MouseX * ScreenRect.W;
  Result.Y = 0.5f * MouseY * ScreenRect.H;
  return Result;
}

v2 GetMousePosInWorld(component_camera* Camera, v2 MouseScreenSpace)
{
  v3 CamPos = GetPositionFromMatrix(&Camera->V);
  v2 MousePosScreenSpace = GetMousePosInProjectionWindow(MouseScreenSpace.X, MouseScreenSpace.Y, Camera->OrthoZoom, GameGetAspectRatio());
  v2 MousePosWorldSpace = MousePosScreenSpace + V2(CamPos);
  return MousePosWorldSpace;
}

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

        v2 MouseScreenSpaceStart = CanonicalToScreenSpace(V2(Mouse->X - Mouse->dX,Mouse->Y - Mouse->dY));
        v2 MouseScreenSpace   = CanonicalToScreenSpace(V2(Mouse->X, Mouse->Y));
        v2 MouseDeltaScreenSpace = MouseScreenSpace - MouseScreenSpaceStart;

        HandleZoom(Camera, MouseScreenSpace.X, MouseScreenSpace.Y, Mouse->dZ);
        HandleTranslate(Camera, Mouse->Button[PlatformMouseButton_Left].Active, MouseDeltaScreenSpace.X, MouseDeltaScreenSpace.Y);

        tile_map* TileMap = &GlobalGameState->World->TileMap;

        v2 MousePosWorldSpace = GetMousePosInWorld(Camera, MouseScreenSpace);
        
        TileMap->MousePosition = CanonicalizePosition( TileMap, V3(MousePosWorldSpace.X, MousePosWorldSpace.Y,0));
        
        tile_contents ContentAtMouse = GetTileContents(TileMap, TileMap->MousePosition);

        if(!ContentAtMouse.Component)
        {
          electrical_component* Component = 0;
          if(Pushed(Keyboard->Key_S))
          {
            Component = PushStruct(World->Arena, electrical_component);
            Component->Type = ElectricalComponentType_Source;
          }
          if(Pushed(Keyboard->Key_R))
          {
            Component    = PushStruct(World->Arena, electrical_component);
            Component->Type = ElectricalComponentType_Resistor;
          }
          if(Pushed(Keyboard->Key_L))
          {
            Component      = PushStruct(World->Arena, electrical_component);
            Component->Type = ElectricalComponentType_Led_Red;
          }
          if(Pushed(Keyboard->Key_G))
          {
            Component     = PushStruct(World->Arena, electrical_component);
            Component->Type = ElectricalComponentType_Ground;
          }
          if(Component)
          {
            ContentAtMouse.Component = Component;
            SetTileContentsAbs(World->Arena, TileMap, TileMap->MousePosition.AbsTileX, TileMap->MousePosition.AbsTileY, 0, ContentAtMouse);
          }
        }else
        {
          if(Pushed(Keyboard->Key_S))
          {
            ContentAtMouse.Component->Type = ElectricalComponentType_Source;
          }
          if(Pushed(Keyboard->Key_R))
          {
            ContentAtMouse.Component->Type = ElectricalComponentType_Resistor;
          }
          if(Pushed(Keyboard->Key_L))
          {
            ContentAtMouse.Component->Type = ElectricalComponentType_Led_Red;
          }
          if(Pushed(Keyboard->Key_G))
          {
            ContentAtMouse.Component->Type = ElectricalComponentType_Ground;
          }
        }
          

        c8 StringBuffer[1024] = {};
        #if 0
        Platform.DEBUGFormatString(StringBuffer, 1024, 1024-1, "%d.%.0f %d.%.0f (%s)", TileMap->MousePosition.AbsTileX, 100.f*TileMap->MousePosition.RelTileX, TileMap->MousePosition.AbsTileY, 100.f*TileMap->MousePosition.RelTileY,
          ContentAtMouse.Component ? ComponentTypeToString(ContentAtMouse.Component->Type) : "empty");
        #else
        //Platform.DEBUGFormatString(StringBuffer, 1024, 1024-1, "%f %f", MousePosWorldSpace.X, MousePosWorldSpace.Y);
        Platform.DEBUGFormatString(StringBuffer, 1024, 1024-1, "%s", Pushed(Keyboard->Key_S) ? "Down" : "Up");
        #endif
        PushTextAt(Mouse->X, Mouse->Y, StringBuffer, 8, V4(0,0,0,1));
      }break;
      case ControllerType_Hero:
      {
        component_camera*   Camera   = GetCameraComponent(Components);
        // Handle Camera
      }break;
    }
  }
}
