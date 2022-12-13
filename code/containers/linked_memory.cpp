#include "containers/linked_memory.h"
#include "memory.h"

extern platform_api Platform;

#if HANDMADE_SLOW
void __CheckMemoryListIntegrity(linked_memory* LinkedMemory)
{
  memory_link* IntegrityLink = LinkedMemory->Sentinel.Next;
  u32 ListCount = 0;
  while(IntegrityLink != &LinkedMemory->Sentinel)
  {
    Assert(IntegrityLink == IntegrityLink->Next->Previous);
    Assert(IntegrityLink == IntegrityLink->Previous->Next);
    IntegrityLink = IntegrityLink->Next;
    ListCount++;
  }
}
#endif

void InitiateLinkedMemory(memory_arena* Arena, linked_memory* LinkedMemory, midx MaxMemSize)
{
  Assert(!LinkedMemory->Sentinel.Next);
  Assert(!LinkedMemory->Sentinel.Previous);
  *LinkedMemory = {};
  LinkedMemory->ActiveMemory = 0;
  LinkedMemory->MaxMemSize = (u32) MaxMemSize;
  LinkedMemory->MemoryBase = (u8*) PushSize(Arena, LinkedMemory->MaxMemSize);
  LinkedMemory->Memory = LinkedMemory->MemoryBase;

  ListInitiate(&LinkedMemory->Sentinel);
}

// TODO (Add option to align by a certain byte?)
void* PushSize(linked_memory* LinkedMemory, u32 RequestedSize)
{
  #if DEBUG_PRINT_MENU_MEMORY_ALLOCATION
  {
    u32 RegionUsed = (u32)(LinkedMemory->Memory - LinkedMemory->MemoryBase);
    u32 TotSize = (u32) LinkedMemory->MaxMemSize;
    r32 Percentage = RegionUsed / (r32) TotSize;
    u32 ActiveMemory = LinkedMemory->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed;
    Platform.DEBUGPrint("--==<< Pre Memory Insert >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed, TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed );
  }
  #endif
  memory_link* NewLink = 0;
  u32 ActualSize = RequestedSize + sizeof(memory_link);
  // Get memory from the middle if we have continous space that is big enough
  u32 RegionUsed = (u32)(LinkedMemory->Memory - LinkedMemory->MemoryBase);
  r32 MemoryFragmentation = LinkedMemory->ActiveMemory/(r32)RegionUsed;
  b32 MemoryTooFragmented = MemoryFragmentation < 0.8;
  if( MemoryTooFragmented || RegionUsed == LinkedMemory->MaxMemSize )
  {
    #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
    u32 Slot = 0;
    u32 SlotSpace = 0;
    u32 SlotSize = 0;
    #endif

    memory_link* CurrentLink = LinkedMemory->Sentinel.Next;
    while( CurrentLink->Next != &LinkedMemory->Sentinel)
    {
      midx Base = (midx) CurrentLink + CurrentLink->Size;
      midx NextAddress = (midx)  CurrentLink->Next;
      Assert(Base <= NextAddress);

      midx OpenSpace = NextAddress - Base;

      if(OpenSpace >= ActualSize)
      {
        NewLink = (memory_link*) Base;
        NewLink->Size = ActualSize;
        ListInsertAfter(CurrentLink, NewLink);
        Assert(CurrentLink < CurrentLink->Next);
        Assert(NewLink < NewLink->Next);
        Assert(CurrentLink->Next == NewLink);

        Assert(((u8*)NewLink - (u8*)CurrentLink) == CurrentLink->Size);
        Assert(((u8*)NewLink->Next - (u8*)NewLink) >= ActualSize);
        #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
        SlotSpace = (u32) Slot;
        SlotSize  = (u32) OpenSpace;
        #endif

        break;
      }
      #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
      Slot++;
      #endif
      CurrentLink =  CurrentLink->Next;
    }

    #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
    {
      u32 SlotCount = 0;
      memory_link* CurrentLink2 = LinkedMemory->Sentinel.Next;

      while( CurrentLink2->Next != &LinkedMemory->Sentinel)
      {
        SlotCount++;
        CurrentLink2 = CurrentLink2->Next;
      }
      
      Platform.DEBUGPrint("--==<< Middle Inset >>==--\n");
      Platform.DEBUGPrint(" - Slot: [%d,%d]\n", SlotSpace, SlotCount);
      Platform.DEBUGPrint(" - Size: [%d,%d]\n", ActualSize, SlotSize);
    }
    #endif
  }

  // Otherwise push it to the end
  if(!NewLink)
  {
    // At the moment we don't allow the linked memory to grow further than originally allocated
    Assert(RegionUsed+ActualSize < LinkedMemory->MaxMemSize);
    #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
    Platform.DEBUGPrint("--==<< Post Inset >>==--\n");
    Platform.DEBUGPrint(" - Memory Left  : %d\n", LinkedMemory->MaxMemSize - (u32)RegionUsed + ActualSize);
    Platform.DEBUGPrint(" - Size: %d\n\n", ActualSize);
    #endif

    NewLink = (memory_link*) LinkedMemory->Memory;
    NewLink->Size = ActualSize;
    LinkedMemory->Memory += ActualSize;
    ListInsertBefore( &LinkedMemory->Sentinel, NewLink );
  }
  
  LinkedMemory->ActiveMemory += ActualSize;
  Assert(NewLink);
  void* Result = (void*)(((u8*)NewLink)+sizeof(memory_link));
  utils::ZeroSize(RequestedSize, Result);

  #if DEBUG_PRINT_MENU_MEMORY_ALLOCATION
  {
    u32 RegionUsed2 = (u32)(LinkedMemory->Memory - LinkedMemory->MemoryBase);
    u32 TotSize = (u32) LinkedMemory->MaxMemSize;
    r32 Percentage = RegionUsed2 / (r32) TotSize;
    u32 ActiveMemory = LinkedMemory->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed2;
    
    Platform.DEBUGPrint("--==<< Post Memory Insert >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed2 , TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed2 );
    
  }
  #endif
  return Result;
}

void FreeMemory(linked_memory* LinkedMemory, void * Payload)
{
  memory_link* MemoryLink = GetMemoryLinkFromPayload(Payload);
  MemoryLink->Previous->Next = MemoryLink->Next;
  MemoryLink->Next->Previous = MemoryLink->Previous;
  LinkedMemory->ActiveMemory -= MemoryLink->Size;
  CheckMemoryListIntegrity(LinkedMemory);
}