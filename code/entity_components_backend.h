#pragma once
#include "types.h"
#include "containers/chunk_list.h"

struct component_head;

//
//  Each entity_component_mapping only point to one component_head* stored in a chunk_list of entity_component_mapping_entry in entity manager
//    Pro: Components allocated togeather would be next to each other.
//         Easy to implement
//    Con: entity_component_mapping* Next; would be the only "wasted cache" memory
// Note: Can be improved later by useing the memory management strategy we use in the interface class/
//              Pro: Don't take the 50% cache hit from having a entity_component_mapping_entry* for each component_head*
//                   Very flexible
//                   Easy to implement
//                   Have to extract management strategy to its seaparate file which can be use elsewhere
//              Con: Have to extract management strategy to its seaparate file which can be alot of work


// entity_component_link: A struct holding references between entities and components
// Types is a bitmask where each set references a specific component
// NOTE: Extract the variable sized bitfield in chunk_list to its own type and use here
//       because at the moment we can only support 32 different types of components.
// TODO: Remember when implementing removing entities or chunks within entitis that we
//       have to clear this struct
struct entity_component_link
{
  component_head* Component;
  entity_component_link* Next;
};

struct entity
{
  u32 ID; // ID starts at 1. Index is ID-1
  bitmask32 ComponentFlags;
  entity_component_link* FirstComponentLink; // Points us to the associated components in the component list.
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
  // List filled with entity_component_link
  chunk_list EntityComponentLinks;

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

struct entity_manager_definition
{
  bitmask32 ComponentFlag;
  bitmask32 RequirementsFlag;
  u32 ComponentChunkCount;
  u32 ComponentByteSize;
};

entity_manager* CreateEntityManager(u32 EntityChunkCount, u32 EntityMapChunkCount, u32 ComponentCount, entity_manager_definition* DefinitionVector);

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
