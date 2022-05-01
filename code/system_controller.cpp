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
        component_camera* Camera   = GetCameraComponent(Components);
        // Handle Camera
      }break;
      case ControllerType_Hero:
      {
        component_camera*   Camera   = GetCameraComponent(Components);
        // Handle Camera
      }break;
    }
  }
}
