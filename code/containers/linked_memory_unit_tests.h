#include "containers/linked_memory.h"


memory_link CreateGroundTruthLink(b32 Allocated, u32 ChunkIndex, midx Size, void* Memory)
{
  memory_link Result = {};
  Result.Allocated = Allocated;
  Result.ChunkIndex = ChunkIndex;
  Result.Size = Size; 
  Result.Memory = (bptr) Memory;
  return Result;
}

struct unit_test_linked_memory_tree_verification_struct
{
  u32 LinkCount;
  memory_link Link[32];
  b32 Checked[32];
};

void VerifyLinkedMemoryCounts(linked_memory* LinkedMemory, u32 MemoryChunkCount, u32 MemoryLinkNodeCount, u32 MemoryLinkNodeDatCount, u32 MemoryLinkCount)
{
  Assert(GetBlockCount(&LinkedMemory->MemoryChunks) == MemoryChunkCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinkNodes) == MemoryLinkNodeCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinkNodeData) == MemoryLinkNodeDatCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinks) == MemoryLinkCount);
  Assert(RedBlackTreeNodeCount(&LinkedMemory->FreeMemoryTree) == MemoryLinkNodeCount);
}

unit_test_linked_memory_tree_verification_struct CreateTreeVerificationStruct(u32 LinkCount, const memory_link* Links, b32 Allocated)
{
  unit_test_linked_memory_tree_verification_struct Result = {};
  u32 LinkIndex = 0;
  for (int Index = 0; Index < LinkCount; ++Index)
  {
    if(Links[Index].Allocated == Allocated)
    {
      Result.Link[LinkIndex] = Links[Index];
      LinkIndex++;
    }
  }
  Result.LinkCount = LinkIndex;
  return Result;
}

void VerifyFreeTreeFunction(red_black_tree_node const * Node, void* CustomData)
{
  unit_test_linked_memory_tree_verification_struct* GroundTruth = (unit_test_linked_memory_tree_verification_struct*) CustomData;
  red_black_tree_node_data* Data = Node->Data;
  while(Data)
  {
    memory_link* Link = (memory_link*) Data->Data;

    b32 Found = false;
    u32 Index = 0;
    while(!Found)
    {
      memory_link* GT_Link = &GroundTruth->Link[Index];
      b32* Checked = &GroundTruth->Checked[Index];
      if(!*Checked)
      {
        if(GT_Link->Size == Link->Size && GT_Link->ChunkIndex == Link->ChunkIndex)
        {
          Assert(Link->Allocated == false);
          *Checked = true;
          Found = true;
        }
      }
      Assert(Index < GroundTruth->LinkCount);
      Index++;
    }

    Data = Data->Next;
  }
}


void VerifyAllocatedTreeFunction(red_black_tree_node const * Node, void* CustomData)
{
  unit_test_linked_memory_tree_verification_struct* GroundTruth = (unit_test_linked_memory_tree_verification_struct*) CustomData;
  red_black_tree_node_data* Data = Node->Data;

  memory_link* Link = (memory_link*) Data->Data;
  Assert(!Data->Next);

  b32 Found = false;
  u32 Index = 0;
  while(!Found)
  {
    memory_link* GT_Link = &GroundTruth->Link[Index];
    b32* Checked = &GroundTruth->Checked[Index];
    if(!*Checked)
    {
      if(GT_Link->Size == Link->Size && GT_Link->ChunkIndex == Link->ChunkIndex && GT_Link->Memory == Link->Memory && Node->Key == ((midx) Link->Memory))
      {
        Assert(Link->Allocated);
        *Checked = true;
        Found = true;
      }
    }
    Assert(Index < GroundTruth->LinkCount);
    Index++;
  }
}

void VerifyTree(unit_test_linked_memory_tree_verification_struct* GroundTruth)
{
  for (u32 Index = 0; Index < GroundTruth->LinkCount; ++Index)
  {
    Assert(GroundTruth->Checked[Index]);
  }
}

void VerifyLinkedMemory(linked_memory* LinkedMemory, u32 ChunkCount, const u32* LinkCount, const memory_link GroundTruth[])
{
  chunk_list_iterator It = BeginIterator(&LinkedMemory->MemoryChunks);
  u32 ChunkIndex = 0;
  u32 GroundTruthLinkIndex = 0;
  while(linked_memory_chunk* Chunk = (linked_memory_chunk*) Next(&It))
  {
    u32 LinkMaxCount = LinkCount[ChunkIndex];
    memory_link* Link = Chunk->Sentinel.Next;
    u32 LinkIndex = 0;
    while(Link != &Chunk->Sentinel)
    {
      memory_link GroundTruthLink = GroundTruth[GroundTruthLinkIndex];

      Assert(Link->Allocated == GroundTruthLink.Allocated);
      Assert(Link->Size == GroundTruthLink.Size);
      Assert(Link->ChunkIndex == GroundTruthLink.ChunkIndex);

      if(Link->Allocated)
      {
        Assert(Link->Memory == GroundTruthLink.Memory);
      }
      else
      {
        memory_link* PreviousLink = Link->Previous;
        if(PreviousLink != &Chunk->Sentinel)
        {
          Assert(Link->Memory == (PreviousLink->Memory + PreviousLink->Size));  
        }
      }
      LinkIndex++;
      GroundTruthLinkIndex++;
      Link = Link->Next;
    }
    ChunkIndex++;
    Assert(LinkIndex == LinkMaxCount);
  }

  Assert(ChunkIndex == ChunkCount);
}
  
void Verify(linked_memory* LinkedMemory, memory_link* LinkGroundTruth, u32 ChunkCount, const u32* LinkCountPerChunk, u32 AllocatedNodeCount, u32 FreeNodeCount)
{
  u32 LinkCount = 0;
  for (int Index = 0; Index < ChunkCount; ++Index)
  {
    LinkCount += LinkCountPerChunk[Index];
  }

  u32 TotalNodeCount = AllocatedNodeCount + FreeNodeCount;

  VerifyLinkedMemory(LinkedMemory, ChunkCount, LinkCountPerChunk, LinkGroundTruth);

  unit_test_linked_memory_tree_verification_struct AllocatedTeeGroundTruth = CreateTreeVerificationStruct(LinkCount, LinkGroundTruth, true);
  InOrderTraverse(&LinkedMemory->AllocatedMemoryTree, (void*) &AllocatedTeeGroundTruth, VerifyAllocatedTreeFunction);
  VerifyTree(&AllocatedTeeGroundTruth);

  unit_test_linked_memory_tree_verification_struct FreeTeeGroundTruth = CreateTreeVerificationStruct(LinkCount, LinkGroundTruth, false);
  InOrderTraverse(&LinkedMemory->FreeMemoryTree, (void*) &FreeTeeGroundTruth, VerifyFreeTreeFunction);
  VerifyTree(&FreeTeeGroundTruth);

  Assert(GetBlockCount(&LinkedMemory->MemoryChunks) == ChunkCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinkNodes) == TotalNodeCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinkNodeData) == LinkCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinks) == LinkCount);
  Assert(RedBlackTreeNodeCount(&LinkedMemory->FreeMemoryTree) == FreeNodeCount);
  Assert(RedBlackTreeNodeCount(&LinkedMemory->AllocatedMemoryTree) == AllocatedNodeCount);

  _VerifyLinkedMemory(LinkedMemory);

}

void LinkedMemoryUnitTests(memory_arena* Arena)
{
  temporary_memory TempMem = BeginTemporaryMemory(Arena);
//      |   1  |
//      | 1024 |


  linked_memory LinkedMemory = NewLinkedMemory(Arena, 1024);
  
  memory_link GroundTruth_0[] = 
  {
    CreateGroundTruthLink(false,0,1024,0)
  };
  u32 LinkCountPerChunk_0[] = {1};
  
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_0,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_0),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_0,            // LinkCountPerChunk     : Number of links in each chunk
    0,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    1                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 


  // Allocate new memory which fits.
  //  |      1      |
  //  | Mem0 |      |
  //  |  256 |  768 |
  void* Mem0 = Allocate(&LinkedMemory, 256);

  memory_link GroundTruth_1[] = {
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(false,0,768,0),
  };
  u32 LinkCountPerChunk_1[] = {2};
  
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_1,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_1),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_1,            // LinkCountPerChunk     : Number of links in each chunk
    1,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    1                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 


  // Allocate new memory which fits.
  //  |          1        | 
  //  | Mem0 | Mem1 |     |
  //  |  256 |  256 | 512 |
  // Note: Mem0 and Mem1 does not share the same node in allocated tree because their keys are always unique (memory address)
  void* Mem1 = Allocate(&LinkedMemory, 256);
  memory_link GroundTruth_2[] = {
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(true,0,256, Mem1),
    CreateGroundTruthLink(false,0,512,0),
  };
  u32 LinkCountPerChunk_2[] = {3};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_2,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_2),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_2,            // LinkCountPerChunk     : Number of links in each chunk
    2,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    1                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 


  // Allocate new memory which fits.
  //  |             1             |
  //  | Mem0 | Mem1 | Mem2 |      |
  //  |  256 |  256 |  256 |  256 |
  void* Mem2 = Allocate(&LinkedMemory, 256);
  
  
  memory_link GroundTruth_3[] = {
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(true,0,256, Mem1),
    CreateGroundTruthLink(true,0,256, Mem2),
    CreateGroundTruthLink(false,0,256,0),
  };
  u32 LinkCountPerChunk_3[] = {4};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_3,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_3),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_3,            // LinkCountPerChunk     : Number of links in each chunk
    3,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    1                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 




  // Allocate new memory which does not fit. Allocates new block.
  //  |             1             |  |      2      |
  //  | Mem0 | Mem1 | Mem2 |      |->| Mem3 |      |
  //  |  256 |  256 |  256 |  256 |  |  512 |  512 |
  void* Mem3 = Allocate(&LinkedMemory, 512);
  memory_link GroundTruth_4[] = {
    // Buffer 1
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(true,0,256, Mem1),
    CreateGroundTruthLink(true,0,256, Mem2),
    CreateGroundTruthLink(false,0,256,0),

    // Buffer 2
    CreateGroundTruthLink(true,1,512, Mem3),
    CreateGroundTruthLink(false,1,512,0),
  };
  u32 LinkCountPerChunk_4[] = {4,2};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_4,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_4),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_4,            // LinkCountPerChunk     : Number of links in each chunk
    4,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    2                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 




  // Allocate new memory which fits in first.
  //  |             1             |  |      2      |
  //  | Mem0 | Mem1 | Mem2 | Mem4 |->| Mem3 |      |
  //  |  256 |  256 |  256 |  256 |  |  512 |  512 |
  void* Mem4 = Allocate(&LinkedMemory, 256);
  memory_link GroundTruth_5[] = {
      // Chunk 1
      CreateGroundTruthLink(true,0,256, Mem0),
      CreateGroundTruthLink(true,0,256, Mem1),
      CreateGroundTruthLink(true,0,256, Mem2),
      CreateGroundTruthLink(true,0,256, Mem4),

      // Chunk 2
      CreateGroundTruthLink(true,1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_5[] = {4,2};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_5,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_5),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_5,            // LinkCountPerChunk     : Number of links in each chunk
    5,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    1                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 

  // Free Mem4
  //  |             1             |  |      2      |
  //  | Mem0 | Mem1 | Mem2 |      |->| Mem3 |      |
  //  |  256 |  256 |  256 |  256 |  |  512 |  512 |
  FreeMemory(&LinkedMemory, Mem4);
  memory_link GroundTruth_6[] = {
      // Chunk 1
      CreateGroundTruthLink(true, 0,256,  Mem0),
      CreateGroundTruthLink(true, 0,256,  Mem1),
      CreateGroundTruthLink(true, 0,256,  Mem2),
      CreateGroundTruthLink(false,0,256,  0),

      // Chunk 2
      CreateGroundTruthLink(true,1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_6[] = {4,2};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_6,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_6),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_6,            // LinkCountPerChunk     : Number of links in each chunk
    4,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    2                               // FreeNodeCount         : Total Number of nodes in Free link tree
  );

  // Free Mem1
  // Note: Number of free nodes in the freeNodeTree is  still two, since both slots 
  //       in first chunk share a node
  //  |             1             |  |      2      |
  //  | Mem0 |      | Mem2 |      |->| Mem3 |      |
  //  |  256 |  256 |  256 |  256 |  |  512 |  512 |
  FreeMemory(&LinkedMemory, Mem1);
  memory_link GroundTruth_7[] = {
      // Chunk 1
      CreateGroundTruthLink(true,0,256, Mem0),
      CreateGroundTruthLink(false,0,256, 0),
      CreateGroundTruthLink(true,0,256, Mem2),
      CreateGroundTruthLink(false,0,256, 0),

      // Chunk 2
      CreateGroundTruthLink(true,1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_7[] = {4,2};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_7,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_7),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_7,            // LinkCountPerChunk     : Number of links in each chunk
    3,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    2                               // FreeNodeCount         : Total Number of nodes in Free link tree
  );


  // Allocate new memory which fits in first chunk.
  // Note: The red_black_tree stores duplicate keys with the
  //       latest inserted key being first in the node_data list,
  //       the chosen slot will be the one previously beloning to mem2.
  //       since it was the most recently freed slot.
  //       since Mem1 and Mem4 were both of equal size, but Mem1 was the
  //       last one freed, mem5 will be placed in the slot of Mem1
  //  |                1                |  |      2      |
  //  | Mem0 | Mem5 |     | Mem2 |      |->| Mem3 |      |
  //  |  256 |  200 |  56 |  256 |  256 |  |  512 |  512 |
  void* Mem5 = Allocate(&LinkedMemory, 200 );
  memory_link GroundTruth_8[] = {
      // Chunk 1
      CreateGroundTruthLink(true, 0,256, Mem0),
      CreateGroundTruthLink(true, 0,200, Mem5),
      CreateGroundTruthLink(false,0, 56, 0   ),
      CreateGroundTruthLink(true, 0,256, Mem2),
      CreateGroundTruthLink(false,0,256, 0   ),

      // Chunk 2
      CreateGroundTruthLink(true, 1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_8[] = {5,2};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_8,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_8),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_8,            // LinkCountPerChunk     : Number of links in each chunk
    4,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    3                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 


  // Allocate new memory which fits in first chunk.
  //  |                   1                   |  |      2      |
  //  | Mem0 | Mem5 |     | Mem2 | Mem6 |     |->| Mem3 |      |
  //  |  256 |  200 |  56 |  256 |  200 |  56 |  |  512 |  512 |
  // Note: Number of free nodes in the freeNodeTree is  still two, since both slots 
  //       in first chunk share a node
  void* Mem6 = Allocate(&LinkedMemory, 200 );
  memory_link GroundTruth_9[] = {
      // Chunk 1
      CreateGroundTruthLink(true, 0,256, Mem0),
      CreateGroundTruthLink(true, 0,200, Mem5),
      CreateGroundTruthLink(false,0, 56, 0   ),
      CreateGroundTruthLink(true, 0,256, Mem2),
      CreateGroundTruthLink(true, 0,200, Mem6),
      CreateGroundTruthLink(false,0, 56, 0   ),

      // Chunk 2
      CreateGroundTruthLink(true, 1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_9[] = {6,2};
  Verify(
    &LinkedMemory,                  // LinkedMemory
    GroundTruth_9,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_9),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_9,            // LinkCountPerChunk     : Number of links in each chunk
    5,                              // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    2                               // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 



  // Free Mem2
  //  |                 1               |  |      2      |
  //  | Mem0 | Mem5 |      | Mem6 |     |->| Mem3 |      |
  //  |  256 |  200 |  312 |  200 |  56 |  |  512 |  512 |
  FreeMemory(&LinkedMemory, Mem2);
   memory_link GroundTruth_10[] = {
      // Chunk 1
      CreateGroundTruthLink(true, 0,256, Mem0),
      CreateGroundTruthLink(true, 0,200, Mem5),
      CreateGroundTruthLink(false,0,312, 0   ),
      CreateGroundTruthLink(true, 0,200, Mem6),
      CreateGroundTruthLink(false,0, 56, 0   ),

      // Chunk 2
      CreateGroundTruthLink(true, 1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_10[] = {5,2};
  Verify(
    &LinkedMemory,                   // LinkedMemory
    GroundTruth_10,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_10),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_10,            // LinkCountPerChunk     : Number of links in each chunk
    4,                               // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    3                                // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 

  // Free Mem6
  //  |          1         |  |      2      |
  //  | Mem0 | Mem5 |      |->| Mem3 |      |
  //  |  256 |  200 |  568 |  |  512 |  512 |
  FreeMemory(&LinkedMemory, Mem6);
   memory_link GroundTruth_11[] = {
      // Chunk 1
      CreateGroundTruthLink(true, 0,256, Mem0),
      CreateGroundTruthLink(true, 0,200, Mem5),
      CreateGroundTruthLink(false,0,568, 0   ),

      // Chunk 2
      CreateGroundTruthLink(true, 1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_11[] = {3,2};
  Verify(
    &LinkedMemory,                   // LinkedMemory
    GroundTruth_11,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_11),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_11,            // LinkCountPerChunk     : Number of links in each chunk
    3,                               // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    2                                // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 

  // Free Mem0
  //  |          1         |  |      2      |
  //  |      | Mem5 |      |->| Mem3 |      |
  //  |  256 |  200 |  568 |  |  512 |  512 |
  FreeMemory(&LinkedMemory, Mem0);
   memory_link GroundTruth_12[] = {
      // Chunk 1
      CreateGroundTruthLink(false, 0,256, 0   ),
      CreateGroundTruthLink(true,  0,200, Mem5),
      CreateGroundTruthLink(false, 0,568, 0   ),

      // Chunk 2
      CreateGroundTruthLink(true, 1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_12[] = {3,2};
  Verify(
    &LinkedMemory,                   // LinkedMemory
    GroundTruth_12,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_12),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_12,            // LinkCountPerChunk     : Number of links in each chunk
    2,                               // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    3                                // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 

  // Free Mem5
  //  |   1  |  |      2      |
  //  |      |->| Mem3 |      |
  //  | 1024 |  |  512 |  512 |
  FreeMemory(&LinkedMemory, Mem5);
  memory_link GroundTruth_13[] = {
      // Chunk 1
      CreateGroundTruthLink(false, 0,1024, 0),

      // Chunk 2
      CreateGroundTruthLink(true, 1,512, Mem3),
      CreateGroundTruthLink(false,1,512,0)
    };
  u32 LinkCountPerChunk_13[] = {1,2};
  Verify(
    &LinkedMemory,                   // LinkedMemory
    GroundTruth_13,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_13),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_13,            // LinkCountPerChunk     : Number of links in each chunk
    1,                               // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    2                                // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 

  // Free Mem3
  //  |   1  |  |   2  |
  //  |      |->|      |
  //  | 1024 |  | 1024 |
  FreeMemory(&LinkedMemory, Mem3);
   memory_link GroundTruth_14[] = {
      // Chunk 1
      CreateGroundTruthLink(false, 0,1024, 0),

      // Chunk 2
      CreateGroundTruthLink(false, 1,1024, 0),
    };
  u32 LinkCountPerChunk_14[] = {1,1};
  Verify(
    &LinkedMemory,                   // LinkedMemory
    GroundTruth_14,                  // LinkGroundTruth       : Ordered links as they appear in the chunks link-list
    ArrayCount(LinkCountPerChunk_14),// ChunkCount            : Number of allocated chunks
    LinkCountPerChunk_14,            // LinkCountPerChunk     : Number of links in each chunk
    0,                               // AllocatedNodeCount    : Total Number of nodes in Allocated link tree
    1                                // FreeNodeCount         : Total Number of nodes in Free link tree
  ); 


  EndTemporaryMemory(TempMem);

}