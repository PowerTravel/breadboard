#include "component_camera.h"
#include "entity_components.h"

v4 getOrtheCameraScreenSize(r32 Zoom, r32 AspectRatio)
{
  const r32 Right = Zoom*AspectRatio;
  const r32 Left  = -Right;
  const r32 Top   = Zoom;
  const r32 Bot   = -Top;
  v4 Result = V4(Left,Right,Top,Bot);
  return Result;
}

internal v2
GetMousePosInProjectionWindow(r32 MouseX, r32 MouseY, r32 Zoom, r32 AspectRatio)
{
  v4 size = getOrtheCameraScreenSize(Zoom, AspectRatio);
  v2 Result = {};
  Result.X = MouseX *  (size.Y - size.X)/2.f; // (Right - Left)
  Result.Y = MouseY * -(size.W - size.Z)/2.f;   // (Top - Bot)
  return Result;
}

void HandleZoom(component_camera* Camera, r32 MouseX, r32 MouseY, r32 ScrollWheel)
{
  if(ScrollWheel != 0)
  {
    game_window_size WindowSize = GameGetWindowSize();
    const r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;

    // TODO:  - Do some slerping here, possibly tied to the scroll wheel, zoom in/zoom out
    r32 ZoomSpeed = 20; 
    v2 MouseScreenSpace = CanonicalToScreenSpace(V2(MouseX, MouseY));
    v2 PrePos = GetMousePosInProjectionWindow(MouseScreenSpace.X, MouseScreenSpace.Y, Camera->OrthoZoom, AspectRatio);
    r32 ZoomPercentage = - ZoomSpeed * ScrollWheel * GlobalGameState->Input->dt;
    r32 Zoom = Camera->OrthoZoom * ZoomPercentage;
    Camera->OrthoZoom += Zoom;

    v4 Size = getOrtheCameraScreenSize(Camera->OrthoZoom, AspectRatio);
    r32 Near  = -1;
    r32 Far   = 10;
    r32 Left  = Size.X;
    r32 Right = Size.Y;
    r32 Top   = Size.Z;
    r32 Bot   = Size.W;
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

    game_window_size WindowSize = GameGetWindowSize();
    const r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;

    v4 Size = getOrtheCameraScreenSize(Camera->OrthoZoom, AspectRatio);
    r32 Near  = -1;
    r32 Far   = 10;
    r32 Left  = Size.X;
    r32 Right = Size.Y;
    r32 Top   = Size.Z;
    r32 Bot   = Size.W;

    v2 ScreenSpaceStart = CanonicalToScreenSpace(V2(MouseX-MouseDX,MouseY-MouseDY));
    v2 ScreenSpaceEnd = CanonicalToScreenSpace(V2(MouseX, MouseY));
    v2 DeltaScreenSpace = ScreenSpaceEnd - ScreenSpaceStart;
    DeltaScreenSpace.X = DeltaScreenSpace.X * (Right-Left)/2;
    DeltaScreenSpace.Y = DeltaScreenSpace.Y * (Top-Bot)/2;
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
