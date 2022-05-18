#include "component_camera.h"
#include "entity_components.h"

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

        r32 ScrollWheel = Mouse->dZ;
        // TODO:  - Do some slerping here, possibly tied to the scroll wheel, zoom in/zoom out
        //        - Zoom towards mouse position
        r32 ZoomPercentage = - 10 * Mouse->dZ * GlobalGameState->Input->dt;
        r32 Zoom = Camera->OrthoZoom * ZoomPercentage;
        Camera->OrthoZoom += Zoom;

        game_window_size WindowSize = GameGetWindowSize();
        r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;
        Camera->OrthoZoom += Zoom;
        r32 Near  = -1;
        r32 Far   = 10;
        r32 Right = Camera->OrthoZoom*AspectRatio;
        r32 Left  = -Right;
        r32 Top   = Camera->OrthoZoom;
        r32 Bot   = -Top;
        SetOrthoProj(Camera, Near, Far, Right, Left, Top, Bot );

        binary_signal_state MouseButtonLeft = Mouse->Button[PlatformMouseButton_Left];
        if(MouseButtonLeft.Active)
        {
          r32 MouseX = Mouse->X; // Canonical Space
          r32 MouseY = Mouse->Y;
          r32 MouseDX = Mouse->dX;
          r32 MouseDY = Mouse->dY;

          v2 ScreenSpaceStart = CanonicalToScreenSpace(V2(MouseX-MouseDX,MouseY-MouseDY));
          v2 ScreenSpaceEnd = CanonicalToScreenSpace(V2(MouseX, MouseY));
          v2 DeltaScreenSpace = ScreenSpaceEnd - ScreenSpaceStart;
          DeltaScreenSpace.X = DeltaScreenSpace.X * (Right-Left)/2;
          DeltaScreenSpace.Y = DeltaScreenSpace.Y * (Top-Bot)/2;
          TranslateCamera(Camera, V3(-DeltaScreenSpace,0));
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
