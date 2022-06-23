#pragma once
#include "types.h"
#include "containers/chunk_list.h"

struct component_head;

// entity_component_mapping: A struct holding references between entities and components
// The list (indicated by a Next member) means we can add further components to the entity later
// Types is a bitmask where each set references a specific component
// NOTE: Extract the variable sized bitfield in chunk_list to its own type and use here
//       because at the moment we can only support 32 different types of components.
// TODO: Remember when implementing removing entities or chunks within entitis that we
//       have to clear this struct
struct entity_component_mapping
{
  u32 Types; // The number of set bits is the lenght of the  *Components array
  component_head** Components; // Not a 2D-array but an array of pointers to components
  entity_component_mapping* Next; // This is a list should we want to add more components later
};

struct entity
{
  u32 ID; // ID starts at 1. Index is ID-1
  bitmask32 ComponentFlags;
  entity_component_mapping* Components;
};

struct component_head
{
  entity* Entity;
  bitmask32 Type;
};

struct component_list
{
  bitmask32 Type;
  u32 Requirements;
  chunk_list Components;
};

struct entity_manager
{
  memory_arena Arena;
  temporary_memory TemporaryMemory;

  u32 EntityIdCounter; // Guarantees a nuique id for new entities
  // List filled with type entity
  chunk_list EntityList;

  u32 ComponentTypeCount;
  component_list* ComponentTypeVector;
};

u32 NewEntity( entity_manager* EM );
void NewComponents(entity_manager* EM, u32 EntityID, bitmask32 ComponentFlags);


struct filtered_entity_iterator
{
  entity_manager* EM;
  bitmask32 ComponentFilter;
  entity* CurrentEntity;
  chunk_list_iterator ComponentIterator;
};

b32 Next(filtered_entity_iterator* EntityIterator);
filtered_entity_iterator GetComponentsOfType(entity_manager* EM, bitmask32 ComponentFlagsToFilterOn);
u32 GetEntity( bptr Component )
{
  component_head* Base = (component_head*) RetreatByType(Component, component_head);
  return Base->Entity->ID;
}

bptr GetComponent(entity_manager* EM, u32 EntityID, bitmask32 ComponentFlag);

bptr GetComponent(entity_manager* EM, filtered_entity_iterator* ComponentList, bitmask32 ComponentFlag);


internal inline b32
IndexOfLeastSignificantSetBit( bitmask32 EntityFlags, u32* Index )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  *Index = BitScan.Index;
  return BitScan.Found;
}

inline u32 IndexOfLeastSignificantSetBit( bitmask32 EntityFlags )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  Assert(BitScan.Found);
  return BitScan.Index;
}



// Used to initiate a entity_manager
component_list CreateComponentList(memory_arena* Arena, bitmask32 TypeFlag, bitmask32 RequirmetFlags, u32 ComponentSize, u32 ComponentCountPerChunk);
