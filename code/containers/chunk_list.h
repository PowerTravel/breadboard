#pragma once

#include "memory.h"
#include "types.h"

/*
 * memory_block_chunk:
 *   A chunk of memory blocks of fixed size
 *   Block:                  1       2       3       4       ...
 *   Memory:             | x x x | - - - | x x x | - - - | - ...
 *   OccupationBitfield:     1       0       1       0       ...
 *   Stores arbitrary data of fixed size
 */

struct memory_block_chunk
{
  // Beginning of memory 
  u8* Memory;

  u32 IndexInList;
  // BlockCount specifies how blocks in the chunk are occupied
  u32 BlockCount; 

  // Each set bit in OccupationBitfield represents an occupied Block and locates its position in Memory.
  // The size in bytes of each Block is constant for each chunk and thus held by the list.
  // The number of u32s in the array is also constant for each chunk and thus held by the list.
  // Think of OccupationBitfield as its own chunk_list where the Slot size is 1 bit and the chunk size is 32 slots;
  bitmask32* OccupationBitfield;

  memory_block_chunk* Next;
};


/* chunk_list
 * A list where each entry is an array ('chunk') of fixed size slots
 * chunk_list grows in sizes of chunks.
 *  - Each chunk is a fixed number of slots.
 *  - Each slot is of fixed size big enough to hold the type the list contains.
 *
 * Design Goal:
 * chunk_list is supposed to give a combinations of the flexibility of list but
 * access speed of vectors with ability for insertions and deletions.
 *  - Fast access within each chunk.
 *  - Can grow in memory but not shrink, done in steps of chunks.
 *      You can always add another chunk to the end but they cannot be removed.
 *      However a freed chunk can be reused.
 *  - Can remove data and new inserts will keep the list packed.
 *  - Things are inserted where they fit, no control of order which means no sorting
 *    without copying memory.
 *
 *  Use case: Gives memory at some address. Good when you want to have dynamic memory.
 *            Avoid if you need a large amount of memory for a short while. The large
 *            ammount cannot be freed witohut releasing the entire chunk_list.
 *
 *
 * Ex: chunk_list with 
 *  - BlockSize = 4 bytes;
 *  - BlockCountPerChunk = 2;
 * 
 * Chunk:    |        0        |    |        1        |    |
 * BitField: |    1   |   0    |    |    0   |   1    |    |
 * Block:    |    0   |   1    | -> |    0   |   1    | -> | ...
 * Bytes:    | 0 1 2 3 4 5 6 7 |    | 0 1 2 3 4 5 6 7 |    |
 * Occupied: | x x x x - - - - |    | - - - - x x x x |    | 
 *
 */
struct chunk_list
{
  // Const Members
  u32 BlockSize;                      // Size in bytes of each memory block for this chunk list. Decided by user.
  u32 BlockCountPerChunk;             // Number of blocks in each chunk. Same for every chunk in the list. Decided by user.

  u32 BitFieldCountPerChunk;          // Number of u32 needed to give each block one bit. Calculated from BlockCountPerChunk.

  // Variable Members
  u32 AvailableBlocks;                // Number of allocated blocks in the whole list.
  u32 AvailableChunks;                // Number of allocated chunks in the list.
  u32 BlockCount;                     // Number of occupied blocks in the whole list.
  memory_block_chunk* First;
  memory_block_chunk* Last;
  memory_block_chunk* FirstFree;
};

chunk_list NewChunkList(memory_arena* Arena, u32 BlockSize, u32 BlocksPerChunk);

// Returns first free block, allocates more chunks if necessary.
bptr GetNewBlock(memory_arena* Arena, chunk_list* List, u32* ResultIndex = 0);

// Returns the block at Index, flags the block as "Not free". Does not allocate but marks as "set"
bptr GetBlock(chunk_list* List, u32 Index);
bptr GetBlockIfItExists(chunk_list* List, u32 Index);

// Frees blocks starting with Block / BlockIndex;
void FreeBlock(chunk_list* List, u32 Index);
void FreeBlock(chunk_list* List, bptr Block);

// Frees the whole list but keeps its capacity
void Clear(chunk_list* List);

// Gets the total available blocks in the list
inline u32 GetCapacity(chunk_list* List);
// Gets the number of set blocks of the list;
inline u32 GetBlockCount(chunk_list* List);

struct chunk_list_iterator
{
  chunk_list* List;
  memory_block_chunk* ActiveChunk;
  u32 IndexInChunk;
};

// Iterator which will loop through set blocks
chunk_list_iterator BeginIterator(chunk_list* List);

// Gets next item in list and increments iterator.
bptr Next(chunk_list_iterator* Iterator);

b32 Valid(chunk_list_iterator* Iterator);

// Implemment when needed.
// Returns first free array block, allocates more if necessary
// bptr GetNewBlockArray(memory_arena* Arena, chunk_list* List)
// Frees range of blocks
// bptr FreeBlockArray(chunk_list* List, u32 Count, bptr FirstBlock);

