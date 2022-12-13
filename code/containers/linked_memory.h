#pragma once

#include "types.h"

#define GetMemoryLinkFromPayload( Payload ) (memory_link*) (((u8*) (Payload)) - sizeof(memory_link))

#define DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION 0
#define DEBUG_PRINT_MENU_MEMORY_ALLOCATION 0

struct memory_link
{
  u32 Size;
  memory_link* Next;
  memory_link* Previous;
};

struct linked_memory
{
  u32 ActiveMemory;
  u32 MaxMemSize;
  u8* MemoryBase;
  u8* Memory;
  memory_link Sentinel;
};


#ifdef HANDMADE_SLOW
void __CheckMemoryListIntegrity(linked_memory* LinkedMemory);
#define CheckMemoryListIntegrity(Interface) __CheckMemoryListIntegrity(Interface)
#elif 
#define CheckMemoryListIntegrity(Interface)
#endif

struct memory_arena;
void InitiateLinkedMemory(memory_arena* Arena, linked_memory* LinkedMemory, midx MaxMemSize);
void* PushSize(linked_memory* LinkedMemory, u32 RequestedSize);
void FreeMemory(linked_memory* LinkedMemory, void * Payload);