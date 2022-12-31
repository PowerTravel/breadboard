#include "containers/linked_memory.h"
#include "memory.h"
#include "utility_macros.h"

extern platform_api Platform;

void _VerifyLinkedMemory(linked_memory* LinkedMemory)
{
  chunk_list_iterator It = BeginIterator(&LinkedMemory->MemoryChunks);
  while( linked_memory_chunk* Chunk = (linked_memory_chunk*) Next(&It))
  {
    memory_link* Link = Chunk->Sentinel.Next;
    while(Link != &Chunk->Sentinel)
    {
      if(Link->Allocated)
      {
        Assert(Link->Memory == ((bptr)Link + sizeof(memory_link)));
      }
      if(Link->Previous != &Chunk->Sentinel)
      {
        memory_link* PreviousLink = Link->Previous;
        if(Link->Allocated)
        {
          Assert((bptr) Link == (PreviousLink->Memory + PreviousLink->Size));
        }else{
          Assert(Link->Memory == (PreviousLink->Memory + PreviousLink->Size));
        }
      }else{
        if(Link->Allocated)
        {
          Assert(Link->Memory == Chunk->MemoryBase + sizeof(memory_link));
        }else{
          Assert(Link->Memory == Chunk->MemoryBase);
        }
      }
      Link = Link->Next;
    }
  }

  RedBlackTreeVerify(&LinkedMemory->FreeMemoryTree);
}

internal inline red_black_tree_node* GetNewNode(linked_memory* LinkedMemory, memory_link* Link, midx Key)
{
  red_black_tree_node_data* NodeData = (red_black_tree_node_data*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodeData);
  *NodeData = NewRedBlackTreeNodeData(Link);

  red_black_tree_node* Node = (red_black_tree_node*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodes);
  *Node = NewRedBlackTreeNode(Key, NodeData);
  return Node;
}

internal inline red_black_tree_node* PushNodeToTree(linked_memory* LinkedMemory, red_black_tree* Tree, red_black_tree_node* Node)
{
  // Insert the link repressenting free space into the tree
  red_black_tree_node* InsertedNode = RedBlackTreeInsert(Tree, Node);

  // If node was not inserted, it means we inserted a duplicate key, and node_data was pushed to the already
  // inserted node. The allocated node can thus be freed.
  if(InsertedNode != Node)
  {
    FreeBlock(&LinkedMemory->MemoryLinkNodes, (bptr) Node);
  }

  return InsertedNode;
}

internal memory_link* GetNewLink(linked_memory* LinkedMemory, memory_link* PreviousLink, u32 ChunkIndex, midx Size, b32 Allocated, bptr Memory)
{
  // Create a new memory_link repressenting free space
  memory_link* Link = (memory_link*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinks);
  Link->Allocated = Allocated;
  Link->ChunkIndex = ChunkIndex;
  Link->Size = Size;
  Link->Memory = Memory;

  ListInsertAfter(PreviousLink, Link);
  return Link;
}

internal red_black_tree_node* CreateNewChunk(linked_memory* LinkedMemory)
{
  u32 IndexOfChunk = 0;
  linked_memory_chunk* Chunk = (linked_memory_chunk*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryChunks, &IndexOfChunk);
  Chunk->MemoryBase = (bptr) PushSize(LinkedMemory->Arena, LinkedMemory->ChunkSize);
  ListInitiate(&Chunk->Sentinel);

  // Create a new memory_link repressenting free space
  memory_link* Link = GetNewLink(LinkedMemory, &Chunk->Sentinel, IndexOfChunk, LinkedMemory->ChunkSize, false, Chunk->MemoryBase);

  // Attach Get new node with the attached link as data. Key is Size
  red_black_tree_node* Node = GetNewNode(LinkedMemory, Link, Link->Size);
  
  red_black_tree_node* InsertedNode = PushNodeToTree(LinkedMemory, &LinkedMemory->FreeMemoryTree, Node);
  return InsertedNode;
}

linked_memory NewLinkedMemory(memory_arena* Arena, midx ChunkMemSize, u32 ExpectedAllocationCount)
{
  linked_memory LinkedMemory = {};
  LinkedMemory.Arena = Arena;
  LinkedMemory.ChunkSize = ChunkMemSize;

  u32 MemoryChunkCount = 16; // Should be on order 1, but has small foot print so lets take 16.
  LinkedMemory.MemoryChunks = NewChunkList(Arena, sizeof(linked_memory_chunk), MemoryChunkCount);
  LinkedMemory.MemoryLinkNodes = NewChunkList(Arena, sizeof(red_black_tree_node), ExpectedAllocationCount);
  LinkedMemory.MemoryLinkNodeData = NewChunkList(Arena, sizeof(red_black_tree_node_data), ExpectedAllocationCount);
  LinkedMemory.MemoryLinks = NewChunkList(Arena, sizeof(memory_link), ExpectedAllocationCount);
  LinkedMemory.FreeMemoryTree = NewRedBlackTree();

  // Allocate first memory chunk
  CreateNewChunk(&LinkedMemory);
 
  return LinkedMemory;
}

internal red_black_tree_node* FindNodeWithFreeSpace(linked_memory* LinkedMemory, midx Size)
{
  red_black_tree* FreeTree = &LinkedMemory->FreeMemoryTree;
  Assert(RedBlackTreeNodeCount(FreeTree) > 0);

  red_black_tree_node* Node = FreeTree->Root;
  while(Node)
  {
    // Node is not big enough to hold our size, 
    if(Size > Node->Key)
    {
      // move to larger node
      if(Node->Right)
      {
        Node = Node->Right;
      }else{
        // No node big enough was found
        Node = NULL;
        break;
      }
    }
    // Node is big enough to hold our size, See if there is a smaller node which can also hold our size
    else if(Size < Node->Key)
    {
      // There is no smaller node
      if(!Node->Left)
      {
        // Choose this one
        break;
      }else{
        // Left node is not big enough
        if(Size > Node->Left->Key)
        {
          // Choose this one
          break;
        }else{
          Assert(Size <= Node->Left->Key);
          Node = Node->Left;
        }
      }
    }else{
      break;
    }
  }

  if(!Node)
  {
    // There is no continous space chunk left in any chunk to hold the required memory. We allocate a new chunk.
    Node = CreateNewChunk(LinkedMemory);
  }

  return Node;
}

internal void DeleteFreeLink(linked_memory* LinkedMemory, red_black_tree* Tree, red_black_tree_node* Node, memory_link* Link)
{
  red_black_tree_node_data* PoppedData = RedBlackTreePopData(Tree, Node, Link);
  Assert(PoppedData);
  // If that was the only data, remove the node
  if(!Node->Data)
  {
    Node = RedBlackTreeDelete(Tree, Link->Size);
    FreeBlock(&LinkedMemory->MemoryLinkNodes, (bptr) Node);
  }

  FreeBlock(&LinkedMemory->MemoryLinkNodeData, (bptr) PoppedData);

  ListRemove(Link);
  FreeBlock(&LinkedMemory->MemoryLinks, (bptr) Link);
}


internal void DeleteFreeLink(linked_memory* LinkedMemory, red_black_tree* Tree, memory_link* Link)
{
  red_black_tree_node* Node = RedBlackTreeFind(Tree, Link->Size);
  DeleteFreeLink(LinkedMemory, Tree, Node, Link); 
}

void* Allocate(linked_memory* LinkedMemory, midx Size)
{
  TIMED_FUNCTION();
  // For now we only allow allocations smaller than chunksize. This can be handled by allocating a new chunk which is big enough.
  // But linked_memory should not be used in such a manner. We should know the approx space we require beforehand when creating the Linked Memory.
  // But this can be fixed later if a case arrives where we truly don't know how big the chunk should be.
  Assert(Size < LinkedMemory->ChunkSize);

  red_black_tree* FreeTree = &LinkedMemory->FreeMemoryTree;

  midx EffectiveSize = Size + sizeof(memory_link);

  red_black_tree_node* Node = FindNodeWithFreeSpace(LinkedMemory, EffectiveSize);

  Assert(Node); // Node should exist in tree

  // Note: The red_black_tree stores duplicate keys with the latest inserted key being
  //       first in the node_data list. Therefore, the node-data chosen here will be the
  //       last freed node of that size. It doesn't necessarily choose the first node in
  //       the memory_link list. Should not be an issue outside of unit-tests. But good
  //       to keep in mind.
  //       For example: Given this memory-link-chain where each mem is of equal size:
  //       | mem1 | mem2 | mem3 | mem4 | mem6 |
  //       Free Mem 3
  //       | mem1 | mem2 |      | mem4 | mem6 |
  //       Free Mem 6
  //       | mem1 | mem2 |      | mem4 |      |
  //       Allocate mem7 of same size
  //       | mem1 | mem2 |      | mem4 | mem7 |
  //       Mem 7 is inserted in the slot previously held by mem 6
  memory_link* Link = (memory_link*) Node->Data->Data;
  u32 ChunkIndex = Link->ChunkIndex;
  Assert(Link->Next && Link->Previous);
  Assert(Size <= Link->Size);

  midx FreeSpaceLeft = Link->Size - EffectiveSize;

  memory_link* NewAllocatedLink = (memory_link*) Link->Memory;
  *NewAllocatedLink = *Link;
  NewAllocatedLink->Allocated = true;
  NewAllocatedLink->Size = Size;
  NewAllocatedLink->Memory += sizeof(memory_link);

  ListInsertAfter(Link, NewAllocatedLink);

  DeleteFreeLink(LinkedMemory, FreeTree, Node, Link);

  // If there is space left we create a new memory_link repressenting free space left after the allocated space
  if(FreeSpaceLeft > 0)
  {
    bptr FreeMemory = NewAllocatedLink->Memory + Size;
    memory_link* NewFreeLink = GetNewLink(LinkedMemory, NewAllocatedLink, ChunkIndex, FreeSpaceLeft, false, FreeMemory);
    red_black_tree_node* NewFreeNode = GetNewNode(LinkedMemory, NewFreeLink, NewFreeLink->Size);
    PushNodeToTree(LinkedMemory, FreeTree, NewFreeNode);    
  }

  utils::ZeroSize(Size, NewAllocatedLink->Memory);

  return NewAllocatedLink->Memory;
}


void FreeMemory(linked_memory* LinkedMemory, void * Payload)
{
  TIMED_FUNCTION();
  red_black_tree* FreeTree = &LinkedMemory->FreeMemoryTree;

  memory_link* LinkToFree = (memory_link*) RetreatByType(Payload, memory_link);

  // Sanity check that we got the right input Payload
  Assert( ((bptr) LinkToFree + sizeof(memory_link)) == LinkToFree->Memory );

  memory_link* PreviousLink = LinkToFree->Previous;
  memory_link* NextLink = LinkToFree->Next;
  ListRemove(LinkToFree);
  u32 ChunkIndex = LinkToFree->ChunkIndex;
  midx FreeSpace = LinkToFree->Size + sizeof(memory_link);
  bptr Memory = (bptr) LinkToFree;

  // If the next link in the memory_link chain is a free link, we need to  remove that link from the freed memory tree and add
  // its size to the Link
  linked_memory_chunk* Chunk = (linked_memory_chunk*) GetBlockIfItExists(&LinkedMemory->MemoryChunks, ChunkIndex);
  Assert(Chunk); // The chunk associated with the link must exist
  
  // If LinkToFree was not the first link and the link before is unallocated space
  // we want to merge them
  if(PreviousLink != &Chunk->Sentinel && !PreviousLink->Allocated)
  {
    FreeSpace += PreviousLink->Size;
    Memory = PreviousLink->Memory;
    memory_link* Tmp = PreviousLink;
    PreviousLink = PreviousLink->Previous;
    DeleteFreeLink(LinkedMemory, FreeTree, Tmp);
  }

  // If LinkToFree was not the Last link and the link before is unallocated space
  // we want to merge them
  if(NextLink != &Chunk->Sentinel && !NextLink->Allocated)
  {
    FreeSpace += NextLink->Size;
    DeleteFreeLink(LinkedMemory, FreeTree, NextLink); 
  }

  memory_link* NewFreeLink = GetNewLink(LinkedMemory, PreviousLink, ChunkIndex, FreeSpace, false, Memory);
  red_black_tree_node* NewFreeNode = GetNewNode(LinkedMemory, NewFreeLink, NewFreeLink->Size);
  PushNodeToTree(LinkedMemory, FreeTree, NewFreeNode);

}
