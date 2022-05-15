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

        // TODO: Move with mouse, not keyboard
        v3 Direction = {};
        r32 Speed = 1 * GlobalGameState->Input->dt;
        if(Keyboard->Key_A.Active)
        {
          Direction.X -= Speed;
        }
        if(Keyboard->Key_D.Active)
        {
          Direction.X += Speed;
        }
        if(Keyboard->Key_S.Active)
        {
          Direction.Y -= Speed;
        }
        if(Keyboard->Key_W.Active)
        {
          Direction.Y += Speed;
        }


        // TODO: Do some slerping here, possibly tied to the scroll wheel, zoom in/zoom out
        r32 ZoomPercentageSpeed = 0.2f * GlobalGameState->Input->dt;
        r32 ZoomPercentage = 0;
        if(Keyboard->Key_Q.Active)
        {
          ZoomPercentage += ZoomPercentageSpeed;
        }
        if(Keyboard->Key_E.Active)
        {
          ZoomPercentage -= ZoomPercentageSpeed;
        }
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

        Direction = Speed * Normalize(Direction);

        TranslateCamera(Camera, Direction);
      }break;
      case ControllerType_Hero:
      {
        component_camera*   Camera   = GetCameraComponent(Components);
        // Handle Camera
      }break;
    }
  }
}
