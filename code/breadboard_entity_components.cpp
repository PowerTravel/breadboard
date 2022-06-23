#include "breadboard_entity_components.h"

entity_manager* CreateEntityManager()
{
  entity_manager* Result = BootstrapPushStruct(entity_manager, Arena);

  u32 CameraChunkCount = 4;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;
  Result->ComponentTypeCount = IndexOfLeastSignificantSetBit(COMPONENT_FLAG_FINAL);
  Result->ComponentTypeVector = PushArray(&Result->Arena, Result->ComponentTypeCount, component_list);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_CAMERA)] = CreateComponentList(&Result->Arena, COMPONENT_FLAG_CAMERA, COMPONENT_FLAG_NONE, sizeof(component_camera), CameraChunkCount);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_CONTROLLER)] = CreateComponentList(&Result->Arena, COMPONENT_FLAG_CONTROLLER, COMPONENT_FLAG_NONE, sizeof(component_controller), ControllerChunkCount);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_RENDER)] = CreateComponentList(&Result->Arena, COMPONENT_FLAG_RENDER, COMPONENT_FLAG_NONE, sizeof(component_render), EntityChunkCount);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_SPRITE_ANIMATION)] = CreateComponentList(&Result->Arena, COMPONENT_FLAG_SPRITE_ANIMATION, COMPONENT_FLAG_NONE, sizeof(component_sprite_animation), EntityChunkCount);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_POSITION)] = CreateComponentList(&Result->Arena, COMPONENT_FLAG_POSITION, COMPONENT_FLAG_NONE, sizeof(component_position), EntityChunkCount);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_ELECTRICAL)] = CreateComponentList(&Result->Arena, COMPONENT_FLAG_ELECTRICAL, COMPONENT_FLAG_POSITION, sizeof(electrical_component), EntityChunkCount);

  for(s32 i = 0; i<IndexOfLeastSignificantSetBit(COMPONENT_FLAG_FINAL); i++)
  {
    Assert(Result->ComponentTypeVector[i].Type);
  }

  Result->EntityIdCounter = 1;
  Result->EntityList = NewChunkList(&Result->Arena, sizeof(entity), EntityChunkCount);

  return Result;
}
