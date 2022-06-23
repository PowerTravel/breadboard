#include "entity_components_backend.h"

namespace entity_components_backend_tests
{

enum component_type_test
{
  TEST_COMPONENT_FLAG_NONE  = 0,
  TEST_COMPONENT_FLAG_A     = 1<<0,
  TEST_COMPONENT_FLAG_B     = 1<<1,
  TEST_COMPONENT_FLAG_C     = 1<<2,
  TEST_COMPONENT_FLAG_D     = 1<<3,
  TEST_COMPONENT_FLAG_FINAL = 1<<4,
};

struct test_component_a
{
  u32 a;
};

struct test_component_b
{
  u32 a;
  u32 b;
};

struct test_component_c
{
  u32 a;
  u32 b;
  u32 c;
};

struct test_component_d
{
  u32 a;
  u32 b;
  u32 c;
  u32 d;
};


entity_manager* CreateEntityManager(u32 EntityChunkCount, u32 ChunkSizeA, u32 ChunkSizeB, u32 ChunkSizeC, u32 ChunkSizeD)
{
  entity_manager* Result = BootstrapPushStruct(entity_manager, Arena);

  Result->ComponentTypeCount = IndexOfLeastSignificantSetBit(TEST_COMPONENT_FLAG_FINAL);
  Result->ComponentTypeVector = PushArray(&Result->Arena, Result->ComponentTypeCount, component_list);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(TEST_COMPONENT_FLAG_A)] = CreateComponentList(&Result->Arena, TEST_COMPONENT_FLAG_A, COMPONENT_FLAG_NONE, sizeof(test_component_a), ChunkSizeA);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(TEST_COMPONENT_FLAG_B)] = CreateComponentList(&Result->Arena, TEST_COMPONENT_FLAG_B, TEST_COMPONENT_FLAG_A, sizeof(test_component_b), ChunkSizeB);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(TEST_COMPONENT_FLAG_C)] = CreateComponentList(&Result->Arena, TEST_COMPONENT_FLAG_C, TEST_COMPONENT_FLAG_A | TEST_COMPONENT_FLAG_B, sizeof(test_component_c), ChunkSizeC);
  Result->ComponentTypeVector[IndexOfLeastSignificantSetBit(TEST_COMPONENT_FLAG_D)] = CreateComponentList(&Result->Arena, TEST_COMPONENT_FLAG_D, TEST_COMPONENT_FLAG_A | TEST_COMPONENT_FLAG_B | TEST_COMPONENT_FLAG_C, sizeof(test_component_d), ChunkSizeD);

  for(s32 i = 0; i<IndexOfLeastSignificantSetBit(TEST_COMPONENT_FLAG_FINAL); i++)
  {
    Assert(Result->ComponentTypeVector[i].Type);
  }

  Result->EntityIdCounter = 1;
  Result->EntityList = NewChunkList(&Result->Arena, sizeof(entity), EntityChunkCount);

  Assert(Result->ComponentTypeCount == 4);
  Assert(GetCapacity(&(Result->EntityList)) == EntityChunkCount);
  Assert(GetCapacity(&Result->ComponentTypeVector[0].Components) == ChunkSizeA);
  Assert(GetCapacity(&Result->ComponentTypeVector[1].Components) == ChunkSizeB);
  Assert(GetCapacity(&Result->ComponentTypeVector[2].Components) == ChunkSizeC);
  Assert(GetCapacity(&Result->ComponentTypeVector[3].Components) == ChunkSizeD);
  Assert(GetBlockCount(&Result->ComponentTypeVector[0].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[1].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[2].Components) == 0);
  Assert(GetBlockCount(&Result->ComponentTypeVector[3].Components) == 0);

  return Result;
}

void RunUnitTestsA(memory_arena* Arena)
{
  ScopedMemory ScopedMem = ScopedMemory(Arena);
  u32 EntityChunkCount = 10;
  u32 ChunkSizeA = 2;
  u32 ChunkSizeB = 4;
  u32 ChunkSizeC = 8;
  u32 ChunkSizeD = 16;
  entity_manager* EntityManager = CreateEntityManager(EntityChunkCount, ChunkSizeA, ChunkSizeB, ChunkSizeC, ChunkSizeD);

  // Test creating alot of TEST_COMPONENT_FLAG_A and loop
  u32 ComponentCountA = ChunkSizeA * 5-1; // 9
  u32* EntityIDs = PushArray(Arena, ComponentCountA, u32);
  for(u32 i = 0; i<ComponentCountA; i++)
  {
    u32 EntityID = NewEntity( EntityManager );
    NewComponents(EntityManager, EntityID, TEST_COMPONENT_FLAG_A);
    test_component_a* A = (test_component_a*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_A);
    EntityIDs[i] = EntityID;
    A->a = EntityID;
  }
  
  // Capacity is the number of chunks needed to hold all allocated components. In our case ChunkSizeA x 5 = 10;
  Assert(GetCapacity(&EntityManager->ComponentTypeVector[0].Components) == ComponentCountA+1);
  Assert(GetBlockCount(&EntityManager->ComponentTypeVector[0].Components) == ComponentCountA);

  // Normal loop of all entitiy indexes
  for(u32 i = 0; i<ComponentCountA; i++)
  {
    u32 EntityID = EntityIDs[i];
    test_component_a* A = (test_component_a*) GetComponent(EntityManager, EntityID, TEST_COMPONENT_FLAG_A);
    Assert(A->a == EntityID);
  }

  // Testing iterator functionality
  filtered_entity_iterator Iterator = GetComponentsOfType(EntityManager, TEST_COMPONENT_FLAG_A);
  u32 index = 0;
  while(Next(&Iterator))
  {
    test_component_a* A = (test_component_a*) GetComponent(EntityManager, &Iterator, TEST_COMPONENT_FLAG_A);
    u32 EntityID = EntityIDs[index++];
    Assert(A->a == EntityID);
  }
}

void RunUnitTests(memory_arena* Arena)
{
  RunUnitTestsA(Arena);
}

}