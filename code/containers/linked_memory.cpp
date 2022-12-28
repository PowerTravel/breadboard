#include "containers/linked_memory.h"
#include "memory.h"
#include "utility_macros.h"

extern platform_api Platform;


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
  LinkedMemory.MemoryLinkTree = NewRedBlackTree();

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
  RedBlackTreeInsert(&LinkedMemory.MemoryLinkTree, Node);

  return LinkedMemory;
}

internal red_black_tree_node* FindNodeWithFreeSpace(linked_memory* LinkedMemory, midx Size)
{
  red_black_tree* Tree = &LinkedMemory->MemoryLinkTree;
  Assert(RedBlackTreeNodeCount(Tree) > 0);

  red_black_tree_node* Node = Tree->Root;
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
    RedBlackTreeInsert(&LinkedMemory->MemoryLinkTree, Node);
  }

  return Node;
}

void* Allocate(linked_memory* LinkedMemory, midx Size)
{
  // For now we only allow allocations smaller than chunksize. This can be handled by allocating a new chunk which is big enough.
  // But linked_memory should not be used in such a manner. We should know the approx space we require beforehand when creating the Linked Memory.
  // But this can be fixed later if a case arrives where we truly don't know how big the chunk should be.
  Assert(Size < LinkedMemory->ChunkSize);

  red_black_tree_node* Node = FindNodeWithFreeSpace(LinkedMemory, Size);
  Assert(Node); // Node should exist in tree

  red_black_tree_node_data* NodeData = Node->Data;
  memory_link* Link = (memory_link*) NodeData->Data;
  Assert(Link->Next && Link->Previous);

  Assert(Size <= Link->Size);
  midx Key = Link->Size;
  midx FreeSpaceLeft = Link->Size - Size;
  Link->Size = Size;
  Link->Allocated = true;

  // Remove the Link from the tree, but keep the memory_link in the chunks link-list
  red_black_tree_node_data* Data = Node->Data;
  RedBlackTreeRemoveSingleDataFromDataList(&LinkedMemory->MemoryLinkTree, &Node->Data, (void*) Link);
  FreeBlock(&LinkedMemory->MemoryLinkNodeData, (bptr) Data);
  Data = 0;

  if(!Node->Data)
  {
    // Node is now empty. Remove from tree.
    RedBlackTreeDelete(&LinkedMemory->MemoryLinkTree, Node->Key);
    FreeBlock(&LinkedMemory->MemoryLinkNodes, (bptr) Node);
    Node = 0;
  }

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
    b32 WasInserted = RedBlackTreeInsert(&LinkedMemory->MemoryLinkTree, NewNode);
    if(!WasInserted)
    {
      // NodeData was inserted into an already existing node. So we free the node we just allocated.
      FreeBlock(&LinkedMemory->MemoryLinkNodes, (bptr) NewNode);
    }
  }

  return Link->Memory;
}


#if 0
linked_memory_chunk* NewLinkedMemoryChunk(linked_memory* LinkedMemory)
{
  linked_memory_chunk* LinkedMemoryChunk = PushStruct(LinkedMemory->Arena, linked_memory_chunk);

  LinkedMemoryChunk->MaxMemSize = (u32) LinkedMemory->ChunkSize;
  LinkedMemoryChunk->MemoryBase = (u8*) PushSize(LinkedMemory->Arena, LinkedMemory->ChunkSize);
  LinkedMemoryChunk->ChunkIndex = LinkedMemory->ChunkCount++;

  ListInsertBefore(&LinkedMemory->Sentinel, LinkedMemoryChunk);

  memory_link* FirstLink = (memory_link*) LinkedMemoryChunk->MemoryBase;
  LinkedMemoryChunk->LargestFreeSlot = LinkedMemoryChunk->MaxMemSize - sizeof(memory_link);
  FirstLink->Size = 0;
  FirstLink->ChunkIndex = LinkedMemoryChunk->ChunkIndex;
  FirstLink->FreeSizeAfter = LinkedMemoryChunk->LargestFreeSlot;

  // Need to verify if this is a okay ammount of memory needed for the tree
  // Depends on the number of allocations we make.
  u32 NodeAndDataCount = 128; 
  LinkedMemoryChunk->MemoryLinkTree = NewRBTree(LinkedMemory->Arena, NodeAndDataCount, NodeAndDataCount);
  Insert(&LinkedMemoryChunk->MemoryLinkTree, FirstLink->FreeSizeAfter, FirstLink);

  return LinkedMemoryChunk;
}

linked_memory_chunk* GetMemoryChunk(linked_memory* LinkedMemory, midx Size)
{
  midx ActualSize = Size + sizeof(memory_link);
  Assert(ActualSize <= LinkedMemory->ChunkSize);

  linked_memory_chunk* Chunk = LinkedMemory->Sentinel.Next;
  while(Chunk != &LinkedMemory->Sentinel)
  {
    if(Chunk->LargestFreeSlot < ActualSize)
    {
      break;
    }
  }

  if(Chunk == &LinkedMemory->Sentinel)
  {
    Chunk = NewLinkedMemoryChunk(LinkedMemory);
  }

  return Chunk;
}

void* GetMemoryFromChunk(linked_memory* LinkedMemory, linked_memory_chunk* MemoryChunk, midx Size)
{
  red_black_tree* Tree = &MemoryChunk->MemoryLinkTree.Tree;
  Assert(NodeCount(&MemoryChunk->MemoryLinkTree) > 0);
  red_black_tree_node* Node = Tree->Root;
  midx ActualSize = Size + sizeof(memory_link);
  while(Node)
  {
    // Node is not big enough to hold our size, 
    if(ActualSize > Node->Key)
    {
      // move to larger node
      Assert(Node->Right);
      Node = Node->Right;
    }
    // Node is big enough to hold our size, See if there is a smaller node which can also hold our size
    else if(ActualSize < Node->Key)
    {
      // There is no smaller node
      if(!Node->Left)
      {
        // Choose this one
        break;
      }else{
        // Left node is not big enough
        if(ActualSize > Node->Left->Key)
        {
          // Choose this one
          break;
        }else{
          Assert(ActualSize <= Node->Left->Key);
          Node = Node->Left;
        }
      }
    }else{
      break;
    }
  }

  // We are guaranteed to have a node large enough;
  Assert(Node);

  red_black_tree_node_data* Data = Node->Data;
  memory_link* Link = (memory_link*) Node->Data;
  Delete(&MemoryChunk->MemoryLinkTree, Node, (void*)Link);

  Assert(Link->Size == 0);
  Assert(Link->ChunkIndex == MemoryChunk->ChunkIndex);
  Link->Size = Size;

  Assert(ActualSize < Link->FreeSizeAfter);
  midx NewFreeSizeAfter = Link->FreeSizeAfter - ActualSize;
  Link->FreeSizeAfter = 0;
  memory_link* NewLink = (memory_link*) ((bptr) Link + ActualSize);
  Assert(NewLink->Size == 0);
  Assert(NewLink->ChunkIndex == 0);
  Assert(NewLink->FreeSizeAfter == 0);
  NewLink->ChunkIndex = MemoryChunk->ChunkIndex;
  NewLink->FreeSizeAfter = NewFreeSizeAfter;
  Assert(NewLink == (memory_link*) ((bptr) Link + Size) );
  Insert(&MemoryChunk->MemoryLinkTree, NewLink->FreeSizeAfter, NewLink);

  void* Memory = AdvanceByType(Link, memory_link);
  return Memory;
}
#endif

void FreeMemory(linked_memory* LinkedMemory, void * Payload)
{
  #if 0
  memory_link* MemoryLink = (memory_link*) RetreatByType(Payload, memory_link);
  Assert(MemoryLink->ChunkIndex < LinkedMemory->ChunkCount);

  linked_memory_chunk* Chunk = LinkedMemory->Sentinel.Next;

  while(Chunk->ChunkIndex != MemoryLink->ChunkIndex)
  {
    Assert(Chunk != &LinkedMemory->Sentinel);
    Chunk = Chunk->Next;
  }

  memory_link* LinkToRemove = (memory_link*) AdvanceBytePointer(MemoryLink, sizeof(memory_link) + MemoryLink->Size);
  if(LinkToRemove->FreeSizeAfter > 0)
  {
    Delete(&Chunk->MemoryLinkTree, LinkToRemove->FreeSizeAfter, (void*) LinkToRemove);
  }

  MemoryLink->FreeSizeAfter += MemoryLink->Size;
  MemoryLink->Size = 0;
  Insert(&Chunk->MemoryLinkTree, MemoryLink->FreeSizeAfter, MemoryLink);
  #endif
}