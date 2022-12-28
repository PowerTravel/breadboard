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
  Assert(RedBlackTreeNodeCount(&LinkedMemory->MemoryLinkTree) == MemoryLinkNodeCount);
}

unit_test_linked_memory_tree_verification_struct CreateTreeVerificationStruct(u32 LinkCount, const u32 * FreeLinkIndeces, const memory_link* Links)
{
  unit_test_linked_memory_tree_verification_struct Result = {};
  Result.LinkCount = LinkCount;
  for (int Index = 0; Index < LinkCount; ++Index)
  {
    u32 FreeLinkIndex = FreeLinkIndeces[Index];
    Result.Link[Index] = Links[FreeLinkIndex];
    Assert(!Result.Link[Index].Allocated);
  }
  return Result;
}

void VerifyTreeFunction(red_black_tree_node const * Node, void* CustomData)
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
  
void Verify(linked_memory* LinkedMemory, u32 LinkCount, memory_link* LinkGroundTruth, u32 ChunkCount, const u32* LinkCountPerChunk, u32 FreeLinkCount, const u32* FreeLinkIndeces,  u32 MemoryTreeNodeCount)
{
  unit_test_linked_memory_tree_verification_struct TeeGroundTruth = CreateTreeVerificationStruct(FreeLinkCount, FreeLinkIndeces, LinkGroundTruth);

  VerifyLinkedMemory(LinkedMemory, ChunkCount, LinkCountPerChunk, LinkGroundTruth);
  InOrderTraverse(&LinkedMemory->MemoryLinkTree, (void*) &TeeGroundTruth, VerifyTreeFunction);
  VerifyTree(&TeeGroundTruth);
  Assert(GetBlockCount(&LinkedMemory->MemoryChunks) == ChunkCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinkNodes) == MemoryTreeNodeCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinkNodeData) == FreeLinkCount);
  Assert(GetBlockCount(&LinkedMemory->MemoryLinks) == LinkCount);
  Assert(RedBlackTreeNodeCount(&LinkedMemory->MemoryLinkTree) == MemoryTreeNodeCount);
}

void LinkedMemoryUnitTests(memory_arena* Arena)
{
  temporary_memory TempMem = BeginTemporaryMemory(Arena);
//      |     1     |
//      |   0 - 1024|


  linked_memory LinkedMemory = NewLinkedMemory(Arena, 1024);
  
  memory_link GroundTruth_0[] = 
  {
    CreateGroundTruthLink(false,0,1024,0)
  };
  u32 LinkCountPerChunk_0[] = {1};
  u32 FreeLinkIndeces_0[] = {0};
  Verify(
    &LinkedMemory,              // LinkedMemory
    ArrayCount(GroundTruth_0),  // LinkCount            : Count of LinkGroundTruth
    GroundTruth_0,              // LinkGroundTruth      : Ordered links as they appear in the chunks link-list
    1,                          // ChunkCount           : Number of allocated chunks
    LinkCountPerChunk_0,        // LinkCountPerChunk    : Number of links in each chunk
    1,                          // FreeLinkCount        : Number of links repressenting free memory
    FreeLinkIndeces_0,          // FreeLinkIndeces      : The index of  the free links in LinkGroundTruth
    1                           // MemoryTreeNodeCount  : Number of nodes in the tree
  ); 


  // Allocate new memory which fits.
  //  |           1           |
  //  |     Mem0  |           |
  //  |   0 - 254 | 255 - 1024|
  void* Mem0 = Allocate(&LinkedMemory, 256);

  memory_link GroundTruth_1[] = {
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(false,0,768,0),
  };
  u32 LinkCountPerChunk_1[] = {2};
  u32 FreeLinkIndeces_1[] = {1};
  Verify(
    &LinkedMemory,              // LinkedMemory
    ArrayCount(GroundTruth_1),  // LinkCount            : Count of LinkGroundTruth
    GroundTruth_1,              // LinkGroundTruth      : Ordered links as they appear in the chunks link-list
    1,                          // ChunkCount           : Number of allocated chunks
    LinkCountPerChunk_1,        // LinkCountPerChunk    : Number of links in each chunk
    1,                          // FreeLinkCount        : Number of links repressenting free memory
    FreeLinkIndeces_1,          // FreeLinkIndeces      : The index of  the free links in LinkGroundTruth
    1                           // MemoryTreeNodeCount  : Number of nodes in the tree
  ); 

  
  // Allocate new memory which fits.
  //  |               1                   | 
  //  |    Mem0   |    Mem1   |           |
  //  |   0 -  264| 256 -  511| 512 - 1024|
  void* Mem1 = Allocate(&LinkedMemory, 256);

  memory_link GroundTruth_2[] = {
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(true,0,256, Mem1),
    CreateGroundTruthLink(false,0,512,0),
  };
  u32 LinkCountPerChunk_2[] = {3};
  u32 FreeLinkIndeces_2[] = {2};
  Verify(
    &LinkedMemory,            // LinkedMemory
    ArrayCount(GroundTruth_2),// LinkCount            : Count of LinkGroundTruth
    GroundTruth_2,            // LinkGroundTruth      : Ordered links as they appear in the chunks link-list
    1,                        // ChunkCount           : Number of allocated chunks
    LinkCountPerChunk_2,      // LinkCountPerChunk    : Number of links in each chunk
    1,                        // FreeLinkCount        : Number of links repressenting free memory
    FreeLinkIndeces_2,        // FreeLinkIndeces      : The index of  the free links in LinkGroundTruth
    1                         // MemoryTreeNodeCount  : Number of nodes in the tree
  );   



  // Allocate new memory which fits.
  //  |                       1                       |
  //  |    Mem0   |    Mem1   |    Mem2   |           |
  //  |   0 -  264| 256 -  511| 512 -  767| 768 - 1024|
  void* Mem2 = Allocate(&LinkedMemory, 256);
  
  
  memory_link GroundTruth_3[] = {
    CreateGroundTruthLink(true,0,256, Mem0),
    CreateGroundTruthLink(true,0,256, Mem1),
    CreateGroundTruthLink(true,0,256, Mem2),
    CreateGroundTruthLink(false,0,256,0),
  };
  u32 LinkCountPerChunk_3[] = {4};
  u32 FreeLinkIndeces_3[] = {3};
  Verify(
    &LinkedMemory,            // LinkedMemory
    ArrayCount(GroundTruth_3),// LinkCount            : Count of LinkGroundTruth
    GroundTruth_3,            // LinkGroundTruth      : Ordered links as they appear in the chunks link-list
    1,                        // ChunkCount           : Number of allocated chunks
    LinkCountPerChunk_3,      // LinkCountPerChunk    : Number of links in each chunk
    1,                        // FreeLinkCount        : Number of links repressenting free memory
    FreeLinkIndeces_3,        // FreeLinkIndeces      : The index of  the free links in LinkGroundTruth
    1                         // MemoryTreeNodeCount  : Number of nodes in the tree
  );   


  // Allocate new memory which does not fit. Allocates new block.
  //  |                       1                       |  |           2           |
  //  |    Mem0   |    Mem1   |    Mem2   |           |->|     Mem3  |           |
  //  |   0 -  264| 256 -  511| 512 -  767| 768 - 1024|  |   0 - 511 | 512 - 1023|
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
  u32 FreeLinkIndeces_4[] = {3,5};
  Verify(
    &LinkedMemory,            // LinkedMemory
    ArrayCount(GroundTruth_4),// LinkCount            : Count of LinkGroundTruth
    GroundTruth_4,            // LinkGroundTruth      : Ordered links as they appear in the chunks link-list
    2,                        // ChunkCount           : Number of allocated chunks
    LinkCountPerChunk_4,      // LinkCountPerChunk    : Number of links in each chunk
    2,                        // FreeLinkCount        : Number of links repressenting free memory
    FreeLinkIndeces_4,        // FreeLinkIndeces      : The index of  the free links in LinkGroundTruth
    2                         // MemoryTreeNodeCount  : Number of nodes in the tree
  );   


  // Allocate new memory which fits in first.
  //  |                       1                       |  |           2           |
  //  |    Mem0   |    Mem1   |    Mem2   |    Mem4   |->|     Mem3  |           |
  //  |   0 -  264| 256 -  511| 512 -  767| 768 - 1024|  |   0 - 254 | 255 - 1024|
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
  u32 FreeLinkIndeces_5[] = {5};
  Verify(
    &LinkedMemory,            // LinkedMemory
    ArrayCount(GroundTruth_5),// LinkCount            : Count of LinkGroundTruth
    GroundTruth_5,            // LinkGroundTruth      : Ordered links as they appear in the chunks link-list
    2,                        // ChunkCount           : Number of allocated chunks
    LinkCountPerChunk_5,      // LinkCountPerChunk    : Number of links in each chunk
    1,                        // FreeLinkCount        : Number of links repressenting free memory
    FreeLinkIndeces_5,        // FreeLinkIndeces      : The index of  the free links in LinkGroundTruth
    1                         // MemoryTreeNodeCount  : Number of nodes in the tree
  );        

  EndTemporaryMemory(TempMem);
}