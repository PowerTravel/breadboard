#include "component_camera.h"
#include "entity_components.h"

internal v2
GetMousePosInProjectionWindow(r32 MouseX, r32 MouseY, r32 Zoom, r32 AspectRatio)
{
  rect2f ScreenRect = GetCameraScreenRect(Zoom, AspectRatio);
  v2 Result = {};
  Result.X = 0.5f * MouseX * ScreenRect.W;
  Result.Y = 0.5f * MouseY * ScreenRect.H;
  return Result;
}

void HandleZoom(component_camera* Camera, r32 MouseX, r32 MouseY, r32 ScrollWheel)
{
  if(ScrollWheel != 0)
  {
    const r32 AspectRatio = GameGetAspectRatio();

    // TODO:  - Do some slerping here, possibly tied to the scroll wheel, zoom in/zoom out
    r32 ZoomSpeed = 20; 
    v2 MouseScreenSpace = CanonicalToScreenSpace(V2(MouseX, MouseY));
    v2 PrePos = GetMousePosInProjectionWindow(MouseScreenSpace.X, MouseScreenSpace.Y, Camera->OrthoZoom, AspectRatio);
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

    v2 PostPos = GetMousePosInProjectionWindow(MouseScreenSpace.X, MouseScreenSpace.Y, Camera->OrthoZoom, AspectRatio);

    v2 CamDelta = PostPos - PrePos;
    TranslateCamera(Camera, -V3(CamDelta,0));  
  }  
}

void HandleTranslate(component_camera* Camera, b32 MouseActive, r32 MouseX, r32 MouseY, r32 MouseDX, r32 MouseDY)
{
  if(MouseActive)
  {
    rect2f ScreenRect = GetCameraScreenRect(Camera->OrthoZoom);
    r32 Near  = -1;
    r32 Far   = 10;
    r32 Left  = ScreenRect.X;
    r32 Right = ScreenRect.X + ScreenRect.W;
    r32 Top   = ScreenRect.Y + ScreenRect.H;
    r32 Bot   = ScreenRect.Y;

    v2 ScreenSpaceStart = CanonicalToScreenSpace(V2(MouseX-MouseDX,MouseY-MouseDY));
    v2 ScreenSpaceEnd   = CanonicalToScreenSpace(V2(MouseX, MouseY));
    v2 DeltaScreenSpace = ScreenSpaceEnd - ScreenSpaceStart;

    
    DeltaScreenSpace.X = 0.5f * DeltaScreenSpace.X * ScreenRect.W;
    DeltaScreenSpace.Y = 0.5f * DeltaScreenSpace.Y * ScreenRect.H;

    TranslateCamera(Camera, V3(-DeltaScreenSpace,0));
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

        HandleZoom(Camera, Mouse->X, Mouse->Y, Mouse->dZ);
        HandleTranslate(Camera, Mouse->Button[PlatformMouseButton_Left].Active, Mouse->X, Mouse->Y, Mouse->dX, Mouse->dY);

      }break;
      case ControllerType_Hero:
      {
        component_camera*   Camera   = GetCameraComponent(Components);
        // Handle Camera
      }break;
    }
  }
}
