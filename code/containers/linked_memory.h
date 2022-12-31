#pragma once

#include "types.h"
#include "containers/red_black_tree.h"
#include "containers/chunk_list.h"

struct memory_link
{
  b32 Allocated; // Tells us if the link repressents allocated space or not.
  u32 ChunkIndex;
  // If a memory_link exists in the FreeMemoryTree. It repressents a free chunk of space.
  //   Memory variable points to the unallocated space and Size is the size of the free space chunk
  // If a memory_link does not exist in the MemoryLinkTree it exists within the linked memeory chunk.
  //   It is a header to the AllocatedSpace. Memory points to the allocated space and Size is the size of allocated space
  //   Thus the whole used memory is Size + sizeof(memory_link)
  //
  //   In the below example, 
  //     MemoryLinkA's address is MemoryBase
  //      -> Size is the size of AllocatedSpaceA.
  //      -> Memory points to the address of AllocatedSpaceA which is MemoryLinkA + sizeof(memory_link)
  //     FreeSpaceB is pointed to via Memory variable of a memory_link living in FreeMemoryTree who's Key is Size
  //   MemoryBase -> | MemoryLinkA | AllocatedSpaceA | FreeSpaceB | MemoryLinkC | AllocatedSpaceC | MemoryLinkD | AllocatedSpaceD| ...
  //
  midx Size; 
  bptr Memory;
  memory_link* Next;      // Memory link pointing to the next block of memory in address space.
  memory_link* Previous;  // Memory link pointing to the next block of memory in address space.
};

struct linked_memory_chunk
{
  bptr MemoryBase;
  memory_link Sentinel;
};

struct linked_memory
{
  midx ChunkSize;
  memory_arena* Arena;
  chunk_list MemoryChunks;       // linked_memory_chunk : Holds the base of allocated memory of size "ChunkSize".
  chunk_list MemoryLinkNodes;    // red_black_tree_node
  chunk_list MemoryLinkNodeData; // red_black_tree_node_data
  chunk_list MemoryLinks;        // memory_link : Holds memory links which repressents unallocated space in the FreeMemoryTree
  red_black_tree FreeMemoryTree; // Sorts the memory_links pointing to free memory. FreeSize is Keys
};

linked_memory NewLinkedMemory(memory_arena* Arena, midx ChunkMemSize, u32 ExpectedAllocationCount = 128);
void* Allocate(linked_memory* LinkedMemory, midx Size);
void FreeMemory(linked_memory* LinkedMemory, void * Payload);

void _VerifyLinkedMemory(linked_memory* LinkedMemory);

#if HANDMADE_SLOW
  #define DEBUGVerifyLinkedMemory(LinkedMemory) _VerifyLinkedMemory(LinkedMemory)
#endif