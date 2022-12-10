#include "breadboard_entity_components.h"

entity_manager* CreateEntityManager()
{

  u32 CameraChunkCount = 4;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  entity_manager_definition Definitions[] =
  {
    {COMPONENT_FLAG_CAMERA,           COMPONENT_FLAG_NONE,     CameraChunkCount,     sizeof(component_camera)},
    {COMPONENT_FLAG_CONTROLLER,       COMPONENT_FLAG_NONE,     ControllerChunkCount, sizeof(component_controller)},
    {COMPONENT_FLAG_RENDER,           COMPONENT_FLAG_NONE,     EntityChunkCount,     sizeof(component_render)},
    {COMPONENT_FLAG_SPRITE_ANIMATION, COMPONENT_FLAG_NONE,     EntityChunkCount,     sizeof(component_sprite_animation)},
    {COMPONENT_FLAG_HITBOX,           COMPONENT_FLAG_NONE,     EntityChunkCount,     sizeof(component_hitbox)},
    {COMPONENT_FLAG_ELECTRICAL,       COMPONENT_FLAG_HITBOX,   EntityChunkCount,     sizeof(component_electrical)},
    {COMPONENT_FLAG_CONNECTOR_PIN,    COMPONENT_FLAG_HITBOX,   EntityChunkCount,     sizeof(component_connector_pin)}
  }; 
  entity_manager* Result = CreateEntityManager(EntityChunkCount, EntityChunkCount, ArrayCount(Definitions), Definitions);
  return Result;
}
