#pragma once

#include "memory.h"
#include "breadboard_entity_components.h"
#include "math/vector_math.h"
#include "assets.h"

enum class render_buffer_entry_type
{
  TEXT,
  NEW_LEVEL,
  QUAD_2D,
  QUAD_2D_COLOR,    // Uses texture array which is a stack of 512x512
  QUAD_2D_SPECIAL,  // Uses special texture is a single texture per draw call which is variable size
  COUNT
};

struct entry_type_2d_quad
{
  rect2f QuadRect;
  rect2f UVRect;
  r32 Rotation;
  v2 RotationCenterOffset;
  v4 Colour;
  bitmap_handle BitmapHandle;
};

struct push_buffer_header
{
  render_buffer_entry_type Type;
  push_buffer_header* Next;
};

struct render_group
{
  m4 ProjectionMatrix;
  m4 ViewMatrix;
  v3 CameraPosition;

  u32 ElementCount;
  memory_arena Arena;
  temporary_memory PushBufferMemory;

  push_buffer_header* First;
  push_buffer_header* Last;

  u32 BufferCounts[16];
};

void ResetRenderGroup(render_group* RenderGroup)
{
  EndTemporaryMemory(RenderGroup->PushBufferMemory);
  RenderGroup->PushBufferMemory = BeginTemporaryMemory(&RenderGroup->Arena);

  RenderGroup->ProjectionMatrix = M4Identity();
  RenderGroup->ViewMatrix = M4Identity();
  RenderGroup->CameraPosition = V3(0,0,0);
  RenderGroup->ElementCount = 0;
  RenderGroup->First = 0;
  RenderGroup->Last = 0;

  ZeroArray(ArrayCount(RenderGroup->BufferCounts), RenderGroup->BufferCounts);
}
