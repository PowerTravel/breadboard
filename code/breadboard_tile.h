#pragma once

#include "types.h"
#include "math/vector_math.h"
#include "data_containers.h"
#include "math/aabb.h"

struct electrical_component;
struct tile_map_position{

  // Position within a tile
  // Measured from lower left
  r32 RelTileX;
  r32 RelTileY;
  r32 RelTileZ;

  // Note: The high bits are the tile page index
  //       The low bits are the tile index relative to the page
  u32 AbsTileX;
  u32 AbsTileY;
  u32 AbsTileZ;
};

// Tile Chunk Position is the Chunk Index from tile_map_position
// separated into its parts of high bits and low bits
struct tile_index{
  // High bits of PageIndex
  s32 PageX;
  s32 PageY;
  s32 PageZ;

  // Low bits of page index, Each page only has x/y coords
  u32 TileX;
  u32 TileY;
};

enum tile_type
{
  TILE_TYPE_NONE
};

struct tile_contents
{
  tile_type Type;
  electrical_component* Component;
};

struct tile_page{
  s32 PageX;
  s32 PageY;
  s32 PageZ;

  tile_contents* Page;

  tile_page* NextInHash;
};

struct tile_map{
  r32 TileHeight;
  r32 TileWidth;
  r32 TileDepth;

  s32 PageShift;
  s32 PageMask;
  s32 PageDim;

  // NOTE(Jakob): At the moment this needs to be a power of 2
  tile_page MapHash[4096];

  tile_map_position MousePosition;
};


void InitializeTileMap( tile_map* TileMap );
void GetIntersectingTiles(tile_map* TileMap, list<tile_map_position>* OutputList, aabb3f* AABB );
inline tile_contents GetTileContents(tile_map* TileMap, tile_page* TilePage, s32 RelTileIndexX, s32 RelTileIndexY );
inline tile_contents GetTileContents(tile_map* TileMap, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ);
inline tile_contents GetTileContents(tile_map* TileMap, tile_map_position CanPos);
inline void SetTileContentsAbs(memory_arena* Arena, tile_map* TileMap, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ, tile_contents TileContents);
inline void SetTileContents(memory_arena* Arena, tile_map* TileMap, tile_map_position* CanPos, tile_contents TileContents);
aabb3f GetTileAABB(tile_map* TileMap, tile_map_position CanPos );
b32 IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos);
inline tile_map_position CanonicalizePosition( tile_map* TileMap, v3 Pos );