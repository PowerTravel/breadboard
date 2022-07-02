#include "containers/chunk_list.h"
#include "utility_macros.h"  

/*
 * chunk_index
 * Represents a arbitrarily sized index within the chunk_list.
 * There is redundant data in the chunk_index for ease of use.
 *
 * Ex: Given:
 *  Index = 23;
 *  BlocksPerChunk = 10;
 * Gives a chunk_index with
 *  ChunkIndex = 2;
 *  IndexInChunk = 3;
 * As:
 *  23 = 10 * 2 + 3 = Index;
 */
struct chunk_index
{
  u32 ChunkIndex;     // Index of the chunk
  u32 IndexInChunk;   // Index of the slot within the chunk.
};

/*
 * CreateChunkIndex
 * Takes a arbitrarily sized index transforms it into a chunk_index.
 */
internal inline chunk_index
CreateChunkIndex(u32 Index, u32 BlocksPerChunk)
{
  chunk_index Result = {};
  Result.ChunkIndex = Index / BlocksPerChunk;
  Result.IndexInChunk = Index - Result.ChunkIndex * BlocksPerChunk;
  return Result;
}

/* 
 * NewBlockChunk
 *   Allocates a memory_block_chunk chunk to the given memory_arena
 *   Returns a pointer to the memory_block_chunk
 */
internal inline memory_block_chunk*
NewBlockChunk(memory_arena* Arena, u32 BlocksPerChunk, u32 BlockSize)
{
  memory_block_chunk* Result = PushStruct(Arena, memory_block_chunk);
  Result->Memory = (u8*) PushSize(Arena, BlockSize*BlocksPerChunk);
  Result->BlockCount = 0;
  Result->OccupationBitfield = PushArray(Arena, BlocksPerChunk, bitmask32);
  Result->Next = 0;
  return Result;
}

 /** 
  * AllocateAndInsertNewChunk
  *   Allocates a new chunk and inserts it into the list at the end
  *   Returns a pointer to the new chunk
  */
internal inline memory_block_chunk*
AllocateAndInsertNewChunk(memory_arena* Arena, chunk_list* List)
{
  memory_block_chunk* NewChunk = NewBlockChunk(Arena, List->BlockCountPerChunk, List->BlockSize);
  Assert(List->First)
  Assert(List->Last);
  NewChunk->IndexInList = List->Last->IndexInList + 1;
  List->Last->Next = NewChunk;
  List->Last = NewChunk;
  List->AvailableBlocks += List->BlockCountPerChunk;
  List->AvailableChunks++;
  return NewChunk;
}

internal inline b32 
IsBitSet(chunk_index const * BitFieldIndex, bitmask32* BitFlagArray)
{
  bitmask32* BitFlag = BitFlagArray + BitFieldIndex->ChunkIndex;
  bitmask32 BitToCheck = 1 << BitFieldIndex->IndexInChunk;
  b32 Result = (*BitFlag & BitToCheck);
  return Result;
}

internal inline void
SetBit(chunk_index const * BitFieldIndex, bitmask32* BitFlagArray)
{
  bitmask32* BitFlag = BitFlagArray + BitFieldIndex->ChunkIndex;
  bitmask32 BitToSet = 1 << BitFieldIndex->IndexInChunk;  
  *BitFlag |= BitToSet;
}

internal inline void
UnsetBit(chunk_index const * BitFieldIndex, bitmask32* BitFlagArray)
{
  bitmask32* BitFlag = BitFlagArray + BitFieldIndex->ChunkIndex;
  bitmask32 BitToUnset = 1 << BitFieldIndex->IndexInChunk;
  *BitFlag &= ~BitToUnset;
}


/*
 * Ranges of the different indeces. Example with 8 bit per flag in bit field array.
 * BitFieldArrayIndex |           1              |             2            |
 * Index              |  0  1  2  3  4  5  6  7  |  8  9  10 11 12 13 14 15 |
 * IndexPerBitField   |  0  1  2  3  4  5  6  7  |  0  1  2  3  4  5  6  7  |
 * BitFieldArray      |  x  x  x  x  x  x  x  x  |  x  x  x  x  x  x  x  x  |
 */
internal inline bit_scan_result
GetIndexOfFirstSetBitBaseIndex(u32 Index, u32 MaxIndex, bitmask32* BitFieldArray)
{
  b32 Found = false;
  u32 BitFieldArrayIndex = Index / 32;
  u32 IndexPerBitField = Index - 32 * BitFieldArrayIndex;
  while(Index < MaxIndex)
  {
    bitmask32 BitMask = BitFieldArray[BitFieldArrayIndex] >> IndexPerBitField;
    bit_scan_result BitScanResult = FindLeastSignificantSetBit(BitMask);
    if(BitScanResult.Found)
    {
      Index += BitScanResult.Index;
      Found = true;
      //Should not be possible to find a set bit outside the possible range of set bits
      Assert(IndexPerBitField < MaxIndex);
      break;
    }else{
      Index += 32 - IndexPerBitField;
      IndexPerBitField = 0;
      BitFieldArrayIndex++;
    }
  }

  bit_scan_result Result = {};
  Result.Index = Index;
  Result.Found = Found;
  return Result;
}

internal inline bit_scan_result
GetIndexOfFirstUnsetBitBaseIndex(u32 Index, u32 MaxIndex, bitmask32* BitFieldArray)
{
  b32 Found = false;
  u32 BitFieldArrayIndex = Index / 32;
  u32 IndexPerBitField = Index - 32 * BitFieldArrayIndex;
  while(Index < MaxIndex)
  {
    bitmask32 BitMask = ~(BitFieldArray[BitFieldArrayIndex] >> IndexPerBitField);
    bit_scan_result BitScanResult = FindLeastSignificantSetBit(BitMask);
    if(BitScanResult.Found)
    {
      Index += BitScanResult.Index;
      Found = true;
      //Should not be possible to find a set bit outside the possible range of set bits
      Assert(IndexPerBitField < MaxIndex);
      break;
    }else{
      Index += 32 - IndexPerBitField;
      IndexPerBitField = 0;
      BitFieldArrayIndex++;
    }
  }

  bit_scan_result Result = {};
  Result.Index = Index;
  Result.Found = Found;
  return Result;
}

/*
 *  UpdateFirstFreeChunkRelativeToChunk
 *    Starts with input chunk and finds and sets the first chunk which is not full to the list
 *    Sets List->FirstFree to null no free chunk is found
 */
internal inline void
UpdateFirstFreeChunkRelativeToChunk(chunk_list* List, memory_block_chunk* Chunk)
{
  while(Chunk)
  {
    if(Chunk->BlockCount < List->BlockCountPerChunk)
    {
      break;
    }
    Chunk = Chunk->Next;
  }
  List->FirstFree = Chunk;
}

internal bptr
GetBlockAndFlagAsOccupied(chunk_list* List, memory_block_chunk* Chunk, u32 IndexInChunk)
{
  Assert(IndexInChunk < List->BlockCountPerChunk);
  bptr Result = Chunk->Memory + List->BlockSize * IndexInChunk;

  chunk_index BitFieldIndex = CreateChunkIndex(IndexInChunk, 32);
  if(!IsBitSet(&BitFieldIndex, Chunk->OccupationBitfield))
  {
    // If the space was previously unoccupied we bump the occupation counters
    Chunk->BlockCount++;
    List->BlockCount++;
    SetBit(&BitFieldIndex, Chunk->OccupationBitfield);
    utils::ZeroSize(List->BlockSize, (void*) Result);
  }

  // If the Block is full we need to update new FirstFree
  if((List->FirstFree->IndexInList == Chunk->IndexInList) && (Chunk->BlockCount == List->BlockCountPerChunk))
  {
    UpdateFirstFreeChunkRelativeToChunk(List, Chunk->Next);
  }
  return Result;
}

internal void
FlagBlockAsUnocupied(chunk_list* List, memory_block_chunk* Chunk,  chunk_index* ChunkListIndex)
{
  chunk_index BitFieldIndex = CreateChunkIndex(ChunkListIndex->IndexInChunk, 32);
  if(IsBitSet(&BitFieldIndex, Chunk->OccupationBitfield))
  {
    // If the space was previously unoccupied we bump the occupation counters
    Chunk->BlockCount--;
    List->BlockCount--;
    Assert(Chunk->BlockCount < List->BlockCountPerChunk); // We should never underflow and hit UINT32_MAX in Chunk->BlockCount
    Assert(List->BlockCount < List->AvailableBlocks);     // We should never underflow and hit UINT32_MAX in List->BlockCount
    UnsetBit(&BitFieldIndex, Chunk->OccupationBitfield);
    // Note: We don't zero out memory when deleting a block.
    //       We zero out memory when pushing a new block.
  }

  // Update first free since we removed a slot
  if(Chunk->IndexInList < List->FirstFree->IndexInList)
  {
    List->FirstFree = Chunk;
  }else if(Chunk->IndexInList == List->FirstFree->IndexInList){
    UpdateFirstFreeChunkRelativeToChunk(List, List->FirstFree);  
  }  
}


internal memory_block_chunk*
GetChunk(chunk_list* List, u32 ChunkIndex)
{
  Assert(ChunkIndex < List->AvailableChunks);
  memory_block_chunk* Result = List->First;
  while(ChunkIndex-- > 0)
  {
    Result = Result->Next;
    Assert(Result);
  }
  return Result;
}

internal u32
GetChunkListIndexFromBlockPtr(chunk_list* List, bptr Block)
{
  memory_block_chunk* Chunk = List->First;
  u32 Index = 0;
  while(Chunk)
  {
    // If block lies within the memory range of the current chunk
    if( (Block >= Chunk->Memory) &&
        (Block < (Chunk->Memory + List->BlockSize*List->BlockCountPerChunk)))
    {
      break;
    }
    Index += List->BlockCountPerChunk;
    Chunk = Chunk->Next;
  }
  // If Chunk is 0 then Block is not within this list
  // This is a error state
  Assert(Chunk);

  u32 IndexInChunk = (u32) (Block - Chunk->Memory) / List->BlockSize;
  Index += IndexInChunk;
  return Index;
}

internal inline u32
GetRequiredNumberOfBitFlags(u32 BlockCountPerChunk)
{
  // Each bit field repressents 32 blocks.
  // We need to calculate how many 32 bit ints we need to have 1 bit per block in the chunk.
  u32 Result = BlockCountPerChunk / 32 + 1;
  return Result;
}

// Interface Functions

inline chunk_list NewChunkList(memory_arena* Arena, u32 BlockSize, u32 BlockCountPerChunk)
{
  chunk_list Result = {};
  // Const Values
  Result.BlockSize = BlockSize;
  Result.BlockCountPerChunk = BlockCountPerChunk;
  Result.AvailableBlocks = BlockCountPerChunk;

  Result.BitFieldCountPerChunk = GetRequiredNumberOfBitFlags(BlockCountPerChunk);

  Result.First = NewBlockChunk(Arena, Result.BlockCountPerChunk, Result.BlockSize);
  Result.FirstFree = Result.First;
  Result.Last = Result.First;
  Result.AvailableChunks = 1;

  return Result;
}



/* 
 * GetNewBlock
 * Gets the first free block from the list.
 * If the list is full, a new chunk is pushed to the end.
 * If ResultIndex is not null we return the listIndex
 */
bptr GetNewBlock(memory_arena* Arena, chunk_list* List, u32* ResultIndex)
{
  // List->FirstFree = 0 means we have to allocate a new chunk
  if(!List->FirstFree)
  {
    List->FirstFree = AllocateAndInsertNewChunk(Arena, List);
  }
  
  memory_block_chunk* FreeChunk = List->FirstFree;

  // A FreeChunk should Always have a unoccupied slot. If not, something has gone wrong.
  Assert(FreeChunk->BlockCount < List->BlockCountPerChunk);
  bit_scan_result BitScan = GetIndexOfFirstUnsetBitBaseIndex(0, List->BlockCountPerChunk, FreeChunk->OccupationBitfield);
  
  // Since the OccupationBitfield comes in chunks of 32 bits each chunk may have some left over bits
  // at the end if the number of Blocks per chunk is not a multiple of 32.
  // Therefore it can be good to assert that the bit we found is _not_ one of those left over bits.
  // Should never happen since FreeChunk should be guaranteed to have a free slot in it;
  Assert(BitScan.Found);
  Assert(BitScan.Index < List->BlockCountPerChunk);

  bptr Result = GetBlockAndFlagAsOccupied(List, FreeChunk, BitScan.Index);
  if(ResultIndex)
  {
    *ResultIndex = GetChunkListIndexFromBlockPtr(List, Result);
  }
  return Result;
}

// Returns the block at Index, flags the block as "Not free"
bptr GetBlock(chunk_list* List, u32 Index)
{
  if(Index >= List->AvailableBlocks)
  {
    return 0;
  }

  chunk_index ChunkListIndex = CreateChunkIndex(Index, List->BlockCountPerChunk);
  memory_block_chunk* Chunk = GetChunk(List, ChunkListIndex.ChunkIndex);
  bptr Result = GetBlockAndFlagAsOccupied(List, Chunk, ChunkListIndex.IndexInChunk);
  return Result;
}

// Returns the block at Index, Returns 0 if it's not set
bptr GetBlockIfItExists(chunk_list* List, u32 Index)
{
  bptr Result = 0;
  chunk_index ChunkListIndex = CreateChunkIndex(Index, List->BlockCountPerChunk);
  chunk_index BitFieldIndex = CreateChunkIndex(ChunkListIndex.IndexInChunk, 32);
  memory_block_chunk* Chunk = GetChunk(List, ChunkListIndex.ChunkIndex);
  if(IsBitSet(&BitFieldIndex, Chunk->OccupationBitfield))
  {
    Result = Chunk->Memory + List->BlockSize * ChunkListIndex.IndexInChunk;
  }
  return Result;
}

void FreeBlock(chunk_list* List, u32 Index)
{
  Assert(Index < List->AvailableBlocks);
  chunk_index ChunkListIndex = CreateChunkIndex(Index, List->BlockCountPerChunk);
  memory_block_chunk* Chunk = GetChunk(List, ChunkListIndex.ChunkIndex);
  FlagBlockAsUnocupied(List, Chunk, &ChunkListIndex);
}

void FreeBlock(chunk_list* List, bptr Block)
{
  u32 Index = GetChunkListIndexFromBlockPtr(List, Block);
  FreeBlock(List, Index);
}

void Clear(chunk_list* List)
{
  memory_block_chunk* Chunk = List->First;
  while(Chunk)
  {
    // Clear the bit field of the chunk
    for(u32 BitFieldChunkIndex = 0; 
            BitFieldChunkIndex < List->BitFieldCountPerChunk;
            BitFieldChunkIndex++)
    {
      u32* BitFieldChunk = Chunk->OccupationBitfield + BitFieldChunkIndex;
      *BitFieldChunk = 0;
    }
    Chunk->BlockCount = 0;
    Chunk = Chunk->Next;
  }
  List->BlockCount = 0;
  List->FirstFree = List->First;
}

inline u32 GetCapacity(chunk_list* List)
{
  u32 Result = List->AvailableBlocks;
  return Result;
}

inline u32 GetBlockCount(chunk_list* List)
{
  u32 Result = List->BlockCount;
  return Result;
}


chunk_list_iterator BeginIterator(chunk_list* List)
{
  chunk_list_iterator Result = {};
  memory_block_chunk* Chunk = List->First;
  // Get first non-empty chunk
  while(Chunk)
  {
    if(Chunk->BlockCount)
    {
      break;
    }
    Chunk = Chunk->Next;
  }

  if(Chunk)
  {
    Result.List = List;
    bit_scan_result BitScan = GetIndexOfFirstSetBitBaseIndex(0, List->BlockCountPerChunk, Chunk->OccupationBitfield);
    Assert(BitScan.Found);
    Result.IndexInChunk = BitScan.Index;
    Result.ActiveChunk = Chunk;
    // We should be guaranteed to have a block with some occupied slots
    Assert(Result.IndexInChunk < List->BlockCountPerChunk);
  }

  return Result;
}

/*
 * Next
 * Gives the next ocupied data slot in the list. 
 */
bptr Next(chunk_list_iterator* Iterator)
{
  if(!Iterator->ActiveChunk)
  {
    return 0;
  }
  chunk_list* List = Iterator->List;

  bptr Result = Iterator->ActiveChunk->Memory + List->BlockSize * Iterator->IndexInChunk;

  bit_scan_result BitScan = GetIndexOfFirstSetBitBaseIndex(Iterator->IndexInChunk+1, List->BlockCountPerChunk, Iterator->ActiveChunk->OccupationBitfield);
  Iterator->IndexInChunk = BitScan.Index;
 
  // If we didn't find a set block in the current chunk, find the next chunk with any entries:
  if(!BitScan.Found)
  {
    while(Iterator->ActiveChunk->Next && !Iterator->ActiveChunk->Next->BlockCount)
    {
      Iterator->ActiveChunk = Iterator->ActiveChunk->Next;
    }
    Iterator->ActiveChunk = Iterator->ActiveChunk->Next;
      
    if(Iterator->ActiveChunk)
    {
      BitScan = GetIndexOfFirstSetBitBaseIndex(0, List->BlockCountPerChunk, Iterator->ActiveChunk->OccupationBitfield);
      Assert(BitScan.Found);
      Iterator->IndexInChunk = BitScan.Index;
    }
  }

  return Result;
}

b32 Valid(chunk_list_iterator* Iterator)
{
  return Iterator->ActiveChunk != 0;
}