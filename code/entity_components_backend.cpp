#include "entity_components_backend.h"
#include "utility_macros.h"



internal inline entity* GetEntityFromID(entity_manager* EM, u32 EntityID) 
{ 
  u32 EntityIndex = EntityID-1;
  entity* Entity = (entity*)  GetBlockIfItExists(&EM->EntityList, EntityIndex);
  return Entity;
}

entity_component_mapping* AllocateNewComponentMap(memory_arena* Arena, bitmask32 ComponentFlags)
{
  entity_component_mapping* NewChunk = PushStruct(Arena, entity_component_mapping);
  u32 ComponentCount = GetSetBitCount(ComponentFlags);
  NewChunk->Types = ComponentFlags;
  NewChunk->Components = PushArray(Arena, ComponentCount, component_head*);
  return NewChunk;
}

void CreateEntityComponentMapping(entity_manager* EM, entity* Entity, bitmask32 NewComponentFlags)
{
  entity_component_mapping* EntityComponentMap = AllocateNewComponentMap(&EM->Arena, NewComponentFlags);
  // Only "new" Entities allowed in here with Components not yet initialized
  Assert(Entity->Components == 0);
  u32 Index = 0;
  bitmask32 FlagsToAdd = EntityComponentMap->Types;
  u32 ComponentIndex = 0;
  while(IndexOfLeastSignificantSetBit(FlagsToAdd, &ComponentIndex))
  {
    Assert(ComponentIndex < EM->ComponentTypeCount);
    component_list* ComponentList = EM->ComponentTypeVector + ComponentIndex;
    // Assert we have the right list
    Assert(ComponentList->Type & (1<<ComponentIndex));
    
    component_head* ComponentHead = (component_head*) GetNewBlock(&EM->Arena, &ComponentList->Components);
    ComponentHead->Entity = Entity;
    ComponentHead->Type = ComponentList->Type;
    EntityComponentMap->Components[Index++] = ComponentHead;
    FlagsToAdd -= ComponentList->Type;
  }

  EntityComponentMap->Next = Entity->Components;
  Entity->Components = EntityComponentMap;
}

u32 GetTotalRequirements(entity_manager* EM, bitmask32 ComponentFlags)
{
  bitmask32 SummedFlags = ComponentFlags;
  u32 ComponentIndex = 0;
  while(IndexOfLeastSignificantSetBit(ComponentFlags, &ComponentIndex))
  {
    Assert(ComponentIndex < EM->ComponentTypeCount );
    component_list* ComponentList = EM->ComponentTypeVector + ComponentIndex;
    // Sum all required components
    SummedFlags = SummedFlags | ComponentList->Requirements;
    ComponentFlags -= ComponentList->Type;
  }
  return SummedFlags;
}

internal inline entity_component_mapping*
GetEntityComponentMap( entity* Entity, bitmask32 ComponentFlag )
{
  entity_component_mapping* EntityComponentMap = Entity->Components;
  while(!(EntityComponentMap->Types & ComponentFlag))
  {
    Assert(EntityComponentMap->Next);
    EntityComponentMap = EntityComponentMap->Next;
  }
  return EntityComponentMap;
}

// Enumerates each set bit in Bit and finds what index BitSet holds
// Example:
// BitMaskOfMap:   | 0 1 1 0 1 1 0 |
// BitToEnumerate: | 0 0 0 0 1 0 0 |
// Enumeration:    |   0 1   2 3   | < Order of components in entity_component_mapping->Components
// Result: (2)               ^     
u32 GetIndexOfBitInComponentMap(bitmask32 BitToEnumerate, bitmask32 BitMaskOfMap)
{
  Assert(BitMaskOfMap & BitToEnumerate);
  u32 Index=0;
  u32 IndexOfBitToEnumerate = 0;
  IndexOfLeastSignificantSetBit(BitToEnumerate,&IndexOfBitToEnumerate);

  u32 IndexOfBitInMap = 0;
  while(IndexOfLeastSignificantSetBit(BitMaskOfMap, &IndexOfBitInMap))
  {
    if(IndexOfBitToEnumerate == IndexOfBitInMap)
    {
      return Index;
    }
    BitMaskOfMap -= (1<<IndexOfBitInMap);
    ++Index;
  }
  INVALID_CODE_PATH;
  return 0;
}

internal bptr GetComponent(entity_manager* EM, entity* Entity, u32 ComponentFlag)
{
  if( !(Entity->ComponentFlags & ComponentFlag) )
  {
    return 0;
  }

  u8* Result = 0;
  entity_component_mapping* ComponentMap = GetEntityComponentMap( Entity, ComponentFlag);
  u32 Index = GetIndexOfBitInComponentMap(ComponentFlag, ComponentMap->Types);
  component_head* Head = ComponentMap->Components[Index];
  Assert(Head->Type == ComponentFlag);

  Result = AdvanceByType(Head, component_head);
  return Result;
}

internal component_list*
GetListWithLowestCount(entity_manager* EM, bitmask32 ComponentFlags)
{
  component_list* ListWithLowestCount = 0;
  u32 Index = 0;
  u32 SmallestSize = U32Max;
  bitmask32 FlagsToScan = ComponentFlags;
  while(IndexOfLeastSignificantSetBit(FlagsToScan, &Index))
  {
    component_list* List = EM->ComponentTypeVector + Index;
    u32 ListSize = GetBlockCount(&List->Components);
    if(ListSize < SmallestSize)
    {
      SmallestSize = ListSize;
      ListWithLowestCount = List;
    }
    FlagsToScan -= List->Type;
  }
  return ListWithLowestCount;
}

internal inline b32
DoesEntityHoldAllComponents(entity* Entity, bitmask32 Flags)
{
  b32 Result = (Entity->ComponentFlags & Flags) == Flags;
  return Result;
}

// Functions in header

component_list CreateComponentList(memory_arena* Arena, bitmask32 TypeFlag, bitmask32 RequirmetFlags, u32 ComponentSize, u32 ComponentCountPerChunk)
{
  component_list Result = {};
  Result.Type = TypeFlag;
  Result.Requirements = RequirmetFlags;
  u32 TotalSizePerBlock = sizeof(component_head) + ComponentSize;
  Result.Components = NewChunkList(Arena, TotalSizePerBlock, ComponentCountPerChunk);
  return Result;
}

u32 NewEntity( entity_manager* EM )
{
  // Not allowed to allocate new entity if we are looping through entities
  CheckArena(&EM->Arena);

  entity* NewEntity = (entity*) GetNewBlock(&EM->Arena, &EM->EntityList);
  NewEntity->ID = EM->EntityIdCounter++;
  NewEntity->Components = 0;

  return NewEntity->ID;
}

void NewComponents(entity_manager* EM, u32 EntityID, u32 ComponentFlags)
{
  CheckArena(&EM->Arena);
  Assert(EntityID != 0);

  entity* Entity = GetEntityFromID(EM, EntityID);

  // Always allocate memory for requirements not yet fullfilled
  u32 TotalRequirements = GetTotalRequirements(EM, ComponentFlags);
  u32 NewComponentFlags = (~Entity->ComponentFlags) & TotalRequirements;

  Entity->ComponentFlags = Entity->ComponentFlags | NewComponentFlags;
  CreateEntityComponentMapping(EM, Entity, NewComponentFlags);
}

// Get a single component from an entity
bptr GetComponent(entity_manager* EM, u32 EntityID, u32 ComponentFlag)
{
  Assert( GetSetBitCount(ComponentFlag) == 1);
  Assert( ComponentFlag != 0 );
  entity* Entity = GetEntityFromID(EM, EntityID);
  Assert(Entity); // If this is 0 it probably means that the Entity has been removed from the entity_manager at some point
  bptr Result = GetComponent(EM, Entity, ComponentFlag);
  Assert(Result); // If this is 0 it probably means that the Component has been removed from the entity_manager at some point
  return Result;
}

filtered_entity_iterator GetComponentsOfType(entity_manager* EM, bitmask32 ComponentFlagsToFilterOn)
{
  component_list* SmallestList = GetListWithLowestCount(EM, ComponentFlagsToFilterOn);

  filtered_entity_iterator Result = {};
  if(SmallestList)
  {
    Result.EM = EM;
    Result.ComponentFilter = ComponentFlagsToFilterOn;
    Result.ComponentIterator = BeginIterator(&SmallestList->Components);
  }
  return Result;
};

bptr GetComponent(entity_manager* EM, filtered_entity_iterator* EntityIterator, bitmask32 ComponentFlag)
{
  Assert(GetSetBitCount(ComponentFlag) == 1);
  if(!EntityIterator->CurrentEntity)
  {
    return 0;
  }
  bptr Result = GetComponent(EM, EntityIterator->CurrentEntity->ID, ComponentFlag);
  return Result;
}

b32 Next(filtered_entity_iterator* EntityIterator)
{  
  entity* Entity = 0;
  while(component_head* ComponentHead = (component_head*) Next(&EntityIterator->ComponentIterator))
  {
    if(DoesEntityHoldAllComponents(ComponentHead->Entity, EntityIterator->ComponentFilter))
    {
      Entity = ComponentHead->Entity;
      break;
    }
  }
  EntityIterator->CurrentEntity = Entity;

  return EntityIterator->CurrentEntity != 0;
}