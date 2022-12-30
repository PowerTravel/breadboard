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
      if(Link->Previous != &Chunk->Sentinel)
      {
        memory_link* PreviousLink = Link->Previous;
        Assert(Link->Memory == (PreviousLink->Memory + PreviousLink->Size));
      }else{
        Assert(Link->Memory == Chunk->MemoryBase);
      }
      Link = Link->Next;
    }
  }

  RedBlackTreeVerify(&LinkedMemory->AllocatedMemoryTree);
  RedBlackTreeVerify(&LinkedMemory->FreeMemoryTree);
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

  LinkedMemory.AllocatedMemoryTree = NewRedBlackTree();

  // Allocate first memory chunk
  u32 IndexOfChunk = 0;
  linked_memory_chunk* Chunk = (linked_memory_chunk*) GetNewBlock(LinkedMemory.Arena, &LinkedMemory.MemoryChunks, &IndexOfChunk);
  Chunk->MemoryBase = (bptr) PushSize(LinkedMemory.Arena, ChunkMemSize);
  ListInitiate(&Chunk->Sentinel);

  // Create a new memory_link repressenting free space
  memory_link* Link = (memory_link*) GetNewBlock(LinkedMemory.Arena, &LinkedMemory.MemoryLinks);
  Link->Allocated = false;
  Link->ChunkIndex = IndexOfChunk;
  Link->Size = ChunkMemSize;
  Link->Memory = Chunk->MemoryBase;
  ListInsertAfter(&Chunk->Sentinel, Link);

  // Attach the link to a node_data struct
  red_black_tree_node_data* NodeData = (red_black_tree_node_data*) GetNewBlock(LinkedMemory.Arena, &LinkedMemory.MemoryLinkNodeData);
  *NodeData = NewRedBlackTreeNodeData(Link);

  // Attach the node_data to a node struct
  red_black_tree_node* Node = (red_black_tree_node*) GetNewBlock(LinkedMemory.Arena, &LinkedMemory.MemoryLinkNodes);
  *Node = NewRedBlackTreeNode(Link->Size, NodeData);

  // Insert the link repressenting free space into the tree
  RedBlackTreeInsert(&LinkedMemory.FreeMemoryTree, Node);

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
    u32 IndexOfChunk = 0;
    linked_memory_chunk* Chunk = (linked_memory_chunk*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryChunks, &IndexOfChunk);
    Chunk->MemoryBase = (bptr) PushSize(LinkedMemory->Arena, LinkedMemory->ChunkSize);
    ListInitiate(&Chunk->Sentinel);

    // Create a new memory_link repressenting free space
    memory_link* Link = (memory_link*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinks);
    Link->Allocated = false;
    Link->ChunkIndex = IndexOfChunk;
    Link->Size = LinkedMemory->ChunkSize;
    Link->Memory = Chunk->MemoryBase;
    ListInsertAfter(&Chunk->Sentinel, Link);

    // Attach the link to a node_data struct
    red_black_tree_node_data* NodeData = (red_black_tree_node_data*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodeData);
    *NodeData = NewRedBlackTreeNodeData(Link);

    // Attach the node_data to a node struct
    Node = (red_black_tree_node*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodes);
    *Node = NewRedBlackTreeNode(Link->Size, NodeData);

    // Insert the link repressenting free space into the tree
    RedBlackTreeInsert(FreeTree, Node);
  }

  return Node;
}

void* Allocate(linked_memory* LinkedMemory, midx Size)
{
  TIMED_FUNCTION();
  // For now we only allow allocations smaller than chunksize. This can be handled by allocating a new chunk which is big enough.
  // But linked_memory should not be used in such a manner. We should know the approx space we require beforehand when creating the Linked Memory.
  // But this can be fixed later if a case arrives where we truly don't know how big the chunk should be.
  Assert(Size < LinkedMemory->ChunkSize);

  red_black_tree* FreeTree = &LinkedMemory->FreeMemoryTree;
  red_black_tree* AllocatedTree = &LinkedMemory->AllocatedMemoryTree;

  red_black_tree_node* Node = FindNodeWithFreeSpace(LinkedMemory, Size);
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
  red_black_tree_node_data* NodeData = Node->Data;
  memory_link* Link = (memory_link*) NodeData->Data;
  Assert(Link->Next && Link->Previous);

  Assert(Size <= Link->Size);
  midx Key = Link->Size;
  midx FreeSpaceLeft = Link->Size - Size;
  Link->Size = Size;
  Link->Allocated = true;

  // Remove the Link from the tree, but keep node_data and memory_link
  red_black_tree_node_data* PoppedData = RedBlackTreePopData(FreeTree, Node, (void*) Link);
  Assert(NodeData == PoppedData);

  if(!Node->Data)
  {
    // Node is now empty. Remove from FreeTree, but keep the node memory
    Node = RedBlackTreeDelete(FreeTree, Node->Key);
  }else{
    // If we don't remove the node from the tree, allocate a new node for the allocated tree
    Node = (red_black_tree_node*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodes);
  }

  // Push The allocated Node to the allocated tree
  *NodeData = NewRedBlackTreeNodeData(Link);
  *Node = NewRedBlackTreeNode( (size_t) Link->Memory, NodeData);
  b32 WasInserted = RedBlackTreeInsert(AllocatedTree, Node);
  Assert(WasInserted);

  // If there is space left we create a new memory_link repressenting free space left after the allocated space
  if(FreeSpaceLeft > 0)
  {
    memory_link* NewLink = (memory_link*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinks);
    NewLink->Allocated = false;
    NewLink->ChunkIndex = Link->ChunkIndex;
    NewLink->Size = FreeSpaceLeft;
    NewLink->Memory = Link->Memory + Size;
    ListInsertAfter(Link, NewLink);

    // Attach the link to a node_data struct
    red_black_tree_node_data* NewNodeData = (red_black_tree_node_data*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodeData);
    *NewNodeData = NewRedBlackTreeNodeData(NewLink);

    // Attach the node_data to a node struct
    red_black_tree_node* NewNode = (red_black_tree_node*) GetNewBlock(LinkedMemory->Arena, &LinkedMemory->MemoryLinkNodes);
    *NewNode = NewRedBlackTreeNode(NewLink->Size, NewNodeData);

    // Insert the link repressenting free space into the tree
    WasInserted = RedBlackTreeInsert(FreeTree, NewNode);
    if(!WasInserted)
    {
      // NodeData was inserted into an already existing node. So we free the node we just allocated.
      FreeBlock(&LinkedMemory->MemoryLinkNodes, (bptr) NewNode);
    }
  }

  utils::ZeroSize(Size, Link->Memory);

  return Link->Memory;
}

void DeleteFreeLink(linked_memory* LinkedMemory, red_black_tree* Tree, memory_link* Link)
{
  red_black_tree_node* Node = RedBlackTreeFind(Tree, Link->Size);
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

void FreeMemory(linked_memory* LinkedMemory, void * Payload)
{
  TIMED_FUNCTION();
  red_black_tree* AllocatedTree = &LinkedMemory->AllocatedMemoryTree;
  red_black_tree* FreeTree = &LinkedMemory->FreeMemoryTree;

  // Remove the node from the allocated tree
  // We don't delete the node and nodedata from the chunklist because we will reuse them for the new free link
  red_black_tree_node* Node = RedBlackTreeDelete(AllocatedTree, (midx) Payload);
  Assert(Node);
  red_black_tree_node_data* NodeData = Node->Data;
  memory_link* Link = (memory_link*) NodeData->Data;

  Link->Allocated = false;

  // If the next link in the memory_link chain is a free link, we need to  remove that link from the freed memory tree and add
  // its size to the Link
  linked_memory_chunk* Chunk = (linked_memory_chunk*) GetBlockIfItExists(&LinkedMemory->MemoryChunks, Link->ChunkIndex);
  Assert(Chunk); // The chunk associated with the link must exist
  
  // If Link is not the first link and the link before is unallocated space
  // we want to merge them
  memory_link* PreviousLink = Link->Previous;
  if(PreviousLink != &Chunk->Sentinel && !PreviousLink->Allocated)
  {
    Link->Size += PreviousLink->Size;
    Link->Memory = PreviousLink->Memory;
    DeleteFreeLink(LinkedMemory, FreeTree, PreviousLink);
  }

  memory_link* NextLink = Link->Next;
  // If Link is not the Last link and the link before is unallocated space
  // we want to merge them
  if(NextLink != &Chunk->Sentinel && !NextLink->Allocated)
  {
    Link->Size += NextLink->Size;
    DeleteFreeLink(LinkedMemory, FreeTree, NextLink); 
  }

  *NodeData = NewRedBlackTreeNodeData( (void*) Link);
  *Node = NewRedBlackTreeNode(Link->Size, NodeData);
  b32 WasInserted = RedBlackTreeInsert(FreeTree, Node);
  if(!WasInserted)
  {
    FreeBlock(&LinkedMemory->MemoryLinkNodes, (bptr) Node);
  }

}
