#include "chunk_list.h"
namespace chunk_list_tests {

chunk_list CreateAndPushToList(memory_arena* Arena, u32 BlockCountToPush, u32 BlocksPerChunk, u32 AvailableChunks, u32 BitFieldCountPerChunk)
{ 
  chunk_list List = NewChunkList(Arena, sizeof(u32), BlocksPerChunk);
  
  for(u32 i = 0; i < BlockCountToPush; ++i)
  {
    *((u32*)GetNewBlock(Arena, &List)) = i+1;
  }
  
  Assert(List.AvailableChunks == AvailableChunks);
  Assert(List.BlockCountPerChunk == BlocksPerChunk);
  Assert(GetCapacity(&List) == AvailableChunks * BlocksPerChunk);
  Assert(List.BitFieldCountPerChunk == BitFieldCountPerChunk);
  Assert(GetBlockCount(&List) == BlockCountToPush);

  for(u32 i = 0; i<BlockCountToPush; ++i)
  {
    u32* Data = (u32*) GetBlock(&List, i);
    Assert(*Data == i+1);
  }

  return List;
}

void TestListGrowAndShrink(memory_arena* Arena)
{
  ScopedMemory ScopedMem = ScopedMemory(Arena);
  // Create a chunk list with size < 32  to make sure the bitflag logic works as expected.  

  // Test Params
  u32 BlockCountToPush = 5;
  u32 BlocksPerChunk = 2;
  
  // Veification parameters
  u32 AvailableChunks = 3;
  u32 BitFieldCountPerChunk = 1;
  chunk_list List = CreateAndPushToList(Arena, BlockCountToPush, BlocksPerChunk, AvailableChunks, BitFieldCountPerChunk);

  // Remove last entry, and push a new one make sure it's the same address and is zeroed out upon Getting
  // | 1 2 | 3 4 | 5 - |
  u32* Data = (u32*) GetBlock(&List, 4);
  Assert(GetBlockCount(&List) == 5);
  Assert(*Data == 5);
  Assert(List.Last == List.FirstFree);

  Data = (u32*) GetBlockIfItExists(&List, 5);
  Assert(Data == 0);

  Data = (u32*) GetBlockIfItExists(&List, 3);
  Assert(*Data == 4);

  // | 1 2 | 3 4 | - - |
  FreeBlock(&List, (bptr) Data);
  Assert(GetBlockCount(&List) == 4);

  // | 1 2 | 3 4 | 5 - |
  u32* Tmp = (u32*) GetNewBlock(Arena, &List);
  Assert(Tmp == Data);
  Assert(GetBlockCount(&List) == 5);
  Assert(*Tmp == 0);

  // Remove entry from second block, and make sure the new first free is in the second block
  // | 1 2 | 3 - | 5 - |
  FreeBlock(&List, 3);
  Assert(GetBlockCount(&List) == 4);
  Assert(List.FirstFree == List.First->Next);

  // Remove entry from first block, and make sure the new first free is in the first block
  // | - 2 | 3 - | 5 - |
  FreeBlock(&List, (u32)0);
  Assert(GetBlockCount(&List) == 3);
  Assert(List.FirstFree == List.First);


  {
    /// test iterating set blocks
    // | - 2 | 3 - | 5 - |
    u32 Contents[] = {2,3,5};
    u32 idx = 0;
    chunk_list_iterator Iterator = BeginIterator(&List);
    while(Valid(&Iterator))
    {
      bptr val = Next(&Iterator);
      Assert(val);
      u32* num = (u32*) val;
      Assert(*num == Contents[idx++]);
    }
    Assert(!Iterator.ActiveChunk);
  }

  // Get block at location 0 and verify first free is set to second block
  // | 0 2 | 3 - | 0 - |
  Data = (u32*) GetBlock(&List, 0);
  Assert(*Data == 0);
  Assert(GetBlockCount(&List) == 4);
  Assert(List.FirstFree == List.First->Next);

  // Get block at location 3 and verify first free is set to last block
  // | 0 2 | 3 0 | 0 - |
  Data = (u32*) GetBlock(&List, 3);
  Assert(*Data == 0);
  Assert(GetBlockCount(&List) == 5);
  Assert(List.FirstFree == List.First->Next->Next);

  // Get block at location 5 and verify first free is set to nullptr
  // | 0 2 | 3 0 | 0 0 |
  Data = (u32*) GetBlock(&List, 5);
  Assert(*Data == 0);
  Assert(GetBlockCount(&List) == 6);
  Assert(List.FirstFree == 0);

  // Get block at location 6 which is outside of list-capacity and verify we get nullptr back
  // | 0 3 | 2 0 | 0 0 |
  Data = (u32*) GetBlock(&List, 6);
  Assert(Data == 0);
  Assert(GetBlockCount(&List) == 6);
  Assert(List.FirstFree == 0);
  Assert(GetCapacity(&List) == 6);

  // Get new block and and verify a new block was allocated
  // | 0 2 | 3 0 | 0 0 | 0 - |
  Data = (u32*) GetNewBlock(Arena, &List);
  Assert(*Data == 0);
  Assert(GetBlockCount(&List) == 7);
  Assert(List.FirstFree == List.Last);
  Assert(GetCapacity(&List) == 8);

  // Fill the List and then remove a early block
  // | 0 2 | 3 0 | 0 0 | 0 0 |
  Data = (u32*) GetNewBlock(Arena, &List);
  Assert(*Data == 0);
  Assert(GetBlockCount(&List) == 8);
  Assert(!List.FirstFree);
  Assert(GetCapacity(&List) == 8);
  //->  | 0 2 | - 0 | 0 0 | 0 0 |
  FreeBlock(&List, 2);
  Assert(GetBlockCount(&List) == 7);
  Assert(List.FirstFree == List.First->Next);
  Assert(GetCapacity(&List) == 8);


  // Clear the list
  // | - - | - - | - - | - - |
  Clear(&List);
  Assert(GetBlockCount(&List) == 0);
  Assert(List.FirstFree == List.First);
  Assert(GetCapacity(&List) == 8);

  // Get new block and and verify a new block was allocated to the beginning
  // | 1 - | - - | - - | - - |
  Data = (u32*) GetNewBlock(Arena, &List);
  *Data = 1;
  Assert(*Data == 1);
  Assert(GetBlockCount(&List) == 1);
  Assert(List.FirstFree == List.First);
  Assert(GetCapacity(&List) == 8);

  // Get block at location 6 and verify that first free is still 1
  // | 1 - | - - | - - | 2 - |
  Data = (u32*) GetBlock(&List, 6);
  *Data = 2;
  Assert(GetBlockCount(&List) == 2);
  Assert(List.FirstFree == List.First);
  Assert(GetCapacity(&List) == 8);

  Data = (u32*) GetBlockIfItExists(&List, 0);
  Assert(*Data == 1);

  Data = (u32*) GetBlockIfItExists(&List, 6);
  Assert(*Data == 2);

  Data = (u32*) GetBlockIfItExists(&List, 3);
  Assert(Data == 0);

  {
    /// test iterating set blocks
    // | 1 - | - - | - - | 2 - |
    u32 Contents[] = {1,2};
    u32 idx = 0;
    chunk_list_iterator Iterator = BeginIterator(&List);
    while(Valid(&Iterator))
    {
      bptr val = Next(&Iterator);
      Assert(val);
      u32* num = (u32*) val;
      Assert(*num == Contents[idx++]);
    }
    Assert(!Iterator.ActiveChunk);
  }

  
}

// Test Bitflag logic
// Create a chunk list with size > 32 and < 64 to make sure the bitflag logic works as expected.
void DenseAndSparseLooping(memory_arena* Arena)
{
  ScopedMemory ScopedMem = ScopedMemory(Arena);
  // Test Params
  u32 BlockCountToPush = 50;
  u32 BlocksPerChunk = 40;
  
  // Veification parameters
  u32 AvailableChunks = 2;
  u32 BitFieldCountPerChunk = 2;
  chunk_list List = CreateAndPushToList(Arena, BlockCountToPush, BlocksPerChunk, AvailableChunks, BitFieldCountPerChunk);

  {
    /// test iterating a dense 
    u32 idx = 0;
    chunk_list_iterator Iterator = BeginIterator(&List);
    while(Valid(&Iterator))
    {
      bptr val = Next(&Iterator);
      Assert(val);
      u32* num = (u32*) val;
      Assert(*num == ++idx);
    }
    Assert(!Iterator.ActiveChunk);
    Assert(idx == BlockCountToPush);
  }

  // Create som gaps in the list
  u32 BlocksLeft = BlockCountToPush;
  u32 Stride = 10;
  for(u32 idx = 0; idx < BlockCountToPush-Stride; idx+=Stride)
  {
    FreeBlock(&List, idx);
    FreeBlock(&List, idx+1);
    FreeBlock(&List, idx+2);
    FreeBlock(&List, idx+6);
    FreeBlock(&List, idx+8);
    BlocksLeft -= 5;
  }
  Assert(GetBlockCount(&List) == BlocksLeft);

  {
    /// test iterating a dense list
    u32 idx = 0;
    chunk_list_iterator Iterator = BeginIterator(&List);
    while(bptr val = Next(&Iterator))
    {
      Assert(val);
      ++idx;
    }
    Assert(!Iterator.ActiveChunk);
    Assert(idx == BlocksLeft);
  }


  Clear(&List);
  Assert(GetBlockCount(&List) == 0);
  Assert(List.FirstFree == List.First);
  Assert(GetCapacity(&List) == 80);

  // Loop a sparse list
  *((u32*)GetBlock(&List, 6)) = 1;
  *((u32*)GetBlock(&List, 40)) = 2;
  *((u32*)GetBlock(&List, 30)) = 3;
  chunk_list_iterator Iterator = BeginIterator(&List);
  u32* Data = (u32*)Next(&Iterator);
  Assert(*Data == 1);
  Data = (u32*)Next(&Iterator);
  Assert(*Data == 3);
  Data = (u32*)Next(&Iterator);
  Assert(*Data == 2);
  Data = (u32*)Next(&Iterator);
  Assert(Data == 0);

}

// Test Bitflag logic 2
// Create a chunk list with size > 64 and < 96 to make sure the bitflag logic works as expected.
void LargeList(memory_arena* Arena)
{
  ScopedMemory ScopedMem = ScopedMemory(Arena);
  // Create a chunk list with size < 32  to make sure the bitflag logic works as expected.  

  // Test Params
  u32 BlockCountToPush = 200;
  u32 BlocksPerChunk = 72;
  
  // Veification parameters
  u32 AvailableChunks = 3;
  u32 BitFieldCountPerChunk = 3;

  //chunk_list CreateAndPushToList(memory_arena* Arena, u32 BlockCountToPush, u32 BlocksPerChunk, u32 AvailableChunks, u32 BitFieldCountPerChunk)

  CreateAndPushToList(Arena, BlockCountToPush, BlocksPerChunk, AvailableChunks, BitFieldCountPerChunk);


}

void RunUnitTests(memory_arena* Arena)
{
  TestListGrowAndShrink(Arena);
  DenseAndSparseLooping(Arena);
  LargeList(Arena);
}
};