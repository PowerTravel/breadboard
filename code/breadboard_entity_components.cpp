#include "breadboard_entity_components.h"
#include "entity_components_backend.h"
#include "component_position.h"
#include "component_hitbox.h"
#include "component_breadboard_components.h"
#include "component_camera.h"
#include "component_controller.h"

entity_manager* CreateEntityManager()
{
  u32 CameraChunkCount = 4;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  entity_manager_definition Definitions[] = 
  {
    {COMPONENT_FLAG_POSITION,         COMPONENT_FLAG_NONE,                               EntityChunkCount,     sizeof(component_position)},
    {COMPONENT_FLAG_HITBOX,           COMPONENT_FLAG_NONE,                               EntityChunkCount,     sizeof(component_hitbox)},
    {COMPONENT_FLAG_ELECTRICAL,       COMPONENT_FLAG_HITBOX | COMPONENT_FLAG_POSITION,   EntityChunkCount,     sizeof(component_electrical)},
    {COMPONENT_FLAG_CONNECTOR_PIN,    COMPONENT_FLAG_HITBOX,                             EntityChunkCount,     sizeof(component_connector_pin)},
    {COMPONENT_FLAG_CAMERA,           COMPONENT_FLAG_NONE,                               CameraChunkCount,     sizeof(component_camera)},
    {COMPONENT_FLAG_CONTROLLER,       COMPONENT_FLAG_NONE,                               ControllerChunkCount, sizeof(component_controller)},
  };

  entity_manager* Result = CreateEntityManager(EntityChunkCount, EntityChunkCount, ArrayCount(Definitions), Definitions);
  return Result;
}
