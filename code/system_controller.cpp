#include "component_camera.h"
#include "entity_components.h"

void ControllerSystemUpdate( world* World )
{
  TIMED_FUNCTION();

  entity_manager* EM = GlobalGameState->EntityManager;
  BeginScopedEntityManagerMemory();
  component_result* Components = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
  while(Next(EM, Components))
  {
    component_controller* Controller = GetControllerComponent(Components);
    switch (Controller->Type)
    {
      case ControllerType_FlyingCamera:
      {
        component_camera* Camera = GetCameraComponent(Components);
        keyboard_input* Keyboard = Controller->Keyboard;

        v3 Direction = {};
        r32 Speed = 1;
        if(Keyboard->Key_A.Active)
        {
          Direction.X -= 1;
        }
        if(Keyboard->Key_D.Active)
        {
          Direction.X += 1;
        }
        if(Keyboard->Key_S.Active)
        {
          Direction.Y -= 1;
        }
        if(Keyboard->Key_W.Active)
        {
          Direction.Y += -1;
        }

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
