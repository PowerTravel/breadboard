#pragma once
#include "types.h"
#include "math/rect2f.h"

enum ElectricalComponentType
{
  ElectricalComponentType_None,
  ElectricalComponentType_Source,
  ElectricalComponentType_Ground,
  ElectricalComponentType_Led_Red,
  ElectricalComponentType_Led_Green,
  ElectricalComponentType_Led_Blue,
  ElectricalComponentType_Resistor,
  ElectricalComponentType_Wire,
  ElectricalComponentType_COUNT
};


const c8* ComponentTypeToString(u32 Type)
{
  switch(Type)
  { 
    case ElectricalComponentType_Source: return "Source";
    case ElectricalComponentType_Ground: return "Ground";
    case ElectricalComponentType_Led_Red: return "Led_Red";
    case ElectricalComponentType_Led_Green: return "Led_Green";
    case ElectricalComponentType_Led_Blue: return "Led_Blue";
    case ElectricalComponentType_Resistor: return "Resistor";
    case ElectricalComponentType_Wire: return "Wire";
  }
  return "";
};

enum ElectricalPinType
{
  ElectricalPinType_Positive,
  ElectricalPinType_Negative,
  ElectricalPinType_Output,
  ElectricalPinType_Input,
  ElectricalPinType_A,
  ElectricalPinType_B,
  ElectricalPinType_Count
};

enum LEDColor
{
  LED_COLOR_RED,
  LED_COLOR_GREEN,
  LED_COLOR_BLUE
};

struct electric_dynamic_state
{
  u32 Volt;
  u32 Current;
  u32 Temperature;
};

struct electric_static_state
{
  u32 Resistance;
};

struct io_pin
{
  struct electrical_component* Component;
};

struct electrical_component
{
  u32 Type;
  r32 Rotation;
  u32 PinCount;
  io_pin Pins[6];
  electric_dynamic_state DynamicState;
  electric_static_state StaticState;
};

enum ElectricalComponentSprite
{
  ElectricalComponentSprite_Resistor_Fixed,
  ElectricalComponentSprite_Ground_Earth,
  ElectricalComponentSprite_Battery_SingleCell,
  ElectricalComponentSprite_Diode_Led,
  ElectricalComponentSprite_Conductor_Joint,
  ElectricalComponentSprite_MaxSize,
};

u32 ElectricalComponentToSpriteType(electrical_component* Component)
{
  u32 TileSpriteSheetIndex = 0;
  switch(Component->Type)
  {
    case ElectricalComponentType_Source:    TileSpriteSheetIndex = ElectricalComponentSprite_Battery_SingleCell; break;
    case ElectricalComponentType_Ground:    TileSpriteSheetIndex = ElectricalComponentSprite_Ground_Earth; break;
    case ElectricalComponentType_Led_Red:   TileSpriteSheetIndex = ElectricalComponentSprite_Diode_Led; break;
    case ElectricalComponentType_Led_Green: TileSpriteSheetIndex = ElectricalComponentSprite_Diode_Led; break;
    case ElectricalComponentType_Led_Blue:  TileSpriteSheetIndex = ElectricalComponentSprite_Diode_Led; break;
    case ElectricalComponentType_Resistor:  TileSpriteSheetIndex = ElectricalComponentSprite_Resistor_Fixed; break;
  }
  return TileSpriteSheetIndex;
}

void ConnectPin(electrical_component* ComponentA, ElectricalPinType PinA, electrical_component* ComponentB, ElectricalPinType PinB)
{
  Assert(ArrayCount(ComponentA->Pins) == ElectricalPinType_Count);
  Assert(ArrayCount(ComponentB->Pins) == ElectricalPinType_Count);
  Assert(PinA <= ElectricalPinType_Count);
  Assert(PinB <= ElectricalPinType_Count);
  Assert(!ComponentA->Pins[PinA].Component);
  Assert(!ComponentB->Pins[PinB].Component);
  ComponentA->Pins[PinA].Component = ComponentB;
  ComponentB->Pins[PinB].Component = ComponentA;
}

void DisconnectPin(electrical_component* ComponentA, ElectricalPinType PinA, electrical_component* ComponentB, ElectricalPinType PinB)
{
  Assert(ArrayCount(ComponentA->Pins) == ElectricalPinType_Count);
  Assert(ArrayCount(ComponentB->Pins) == ElectricalPinType_Count);
  Assert(PinA <= ElectricalPinType_Count);
  Assert(PinB <= ElectricalPinType_Count);
  Assert(ComponentA->Pins[PinA].Component == ComponentB);
  Assert(ComponentB->Pins[PinB].Component == ComponentA);
  ComponentA->Pins[PinA].Component = 0;
  ComponentB->Pins[PinB].Component = 0;
}

electrical_component* GetComponentConnectedAtPin(electrical_component* Component, ElectricalPinType PinType)
{
  Assert(ArrayCount(Component->Pins) == ElectricalPinType_Count);
  Assert(PinType <= ElectricalPinType_Count);
  electrical_component* Result = Component->Pins[PinType].Component;
  Assert(Result);
  return Result;
}


internal inline r32 Invert(r32 Val, r32 Length)
{
  r32 Result =  Length - Val;
  Assert(Val >= 0);
  return Result;
}

internal void ConvertYUp(bitmap_points* Points,  r32 SpriteSheetHeight_Pixels)
{
  if(Points->YDirectionUp)
  {
    return;
  }

  Points->TopLeft.Y = Invert(Points->TopLeft.Y, SpriteSheetHeight_Pixels);
  Points->BotRight.Y = Invert(Points->BotRight.Y, SpriteSheetHeight_Pixels);
  Points->Center.Y = Invert(Points->Center.Y, SpriteSheetHeight_Pixels);
  for(u32 i = 0; i < ArrayCount(Points->Points); ++i)
  {
    Points->Points[i].Y = Invert(Points->Points[i].Y, SpriteSheetHeight_Pixels);  
  }
  Points->YDirectionUp = true;
}

rect2f GetTextureRect(v2 TopLeft, v2 BotRight,  r32 SpriteSheetWidth_Pixels, r32 SpriteSheetHeight_Pixels, b32 YisUp)
{
  // From here on out we are origin bot left
  rect2f RectInPixels = {};
  RectInPixels.X = TopLeft.X;
  r32 X1_Pixels = BotRight.X;
  RectInPixels.Y = YisUp ? BotRight.Y : Invert(BotRight.Y, SpriteSheetHeight_Pixels);
  r32 Y1_Pixels = YisUp ? TopLeft.Y : Invert(TopLeft.Y, SpriteSheetHeight_Pixels);

  RectInPixels.W = X1_Pixels-RectInPixels.X;
  RectInPixels.H = Y1_Pixels-RectInPixels.Y;
  Assert(RectInPixels.W > 0);
  Assert(RectInPixels.H > 0);
  return RectInPixels;
}

rect2f GetUVRect(rect2f RectInPixels, r32 SpriteSheetWidth_Pixels, r32 SpriteSheetHeight_Pixels)
{
  // We want the bitmap_coordinate to fall in the middle of the pixel so that it's clamped to the correct edge
  RectInPixels.X += 0.5f;
  RectInPixels.Y += 0.5f;
  RectInPixels.W -= 1.f;
  RectInPixels.H -= 1.f;

  rect2f UVRect = {};
  UVRect.X = RectInPixels.X / SpriteSheetWidth_Pixels;
  UVRect.Y = RectInPixels.Y / SpriteSheetHeight_Pixels;
  UVRect.W = RectInPixels.W / SpriteSheetWidth_Pixels;   // Scale W
  UVRect.H = RectInPixels.H / SpriteSheetHeight_Pixels; // Scale H
  return UVRect;
}

inline v2 GetRelativeCenter(v2 TopLeft, v2 BotRight)
{
  v2 Result = (TopLeft + BotRight) * 0.5f;
  Result.X -= TopLeft.X;
  Result.Y -= TopLeft.Y;
  return Result;
}

inline bitmap_points GetElectricalComponentSpriteBitmapPoints(u32 Index)
{
  Index = Index % ElectricalComponentSprite_MaxSize;
  bitmap_points Points = {};
  Points.YDirectionUp = false;
  switch(Index)
  {
    case ElectricalComponentSprite_Resistor_Fixed:
    {
      Points.TopLeft = V2(78, 60);
      Points.BotRight = V2(205, 89);
      Points.Center = GetRelativeCenter(Points.TopLeft, Points.BotRight);
      Points.Points[0] = V2(80,75);
      Points.Points[1] = V2(204,75);
    } break;
    case ElectricalComponentSprite_Ground_Earth:
    {
      Points.TopLeft = V2(1616, 615);
      Points.BotRight = V2(1651, 659);
      Points.Center = GetRelativeCenter(Points.TopLeft, Points.BotRight);
      Points.Points[0] = V2(1634, 618);
    } break;
    case ElectricalComponentSprite_Diode_Led:
    {
      Points.TopLeft = V2(68, 755);
      Points.BotRight = V2(196, 823);
      Points.Center = GetRelativeCenter(V2(70,787), V2(181,822));
      Points.Points[0] = V2(71, 805);
      Points.Points[1] = V2(180, 805);
    } break;
    case ElectricalComponentSprite_Battery_SingleCell:
    {
      Points.TopLeft = V2(1510, 388);
      Points.BotRight = V2(1588, 446);
      Points.Center = GetRelativeCenter(Points.TopLeft, Points.BotRight);
      Points.Points[0] = V2(1512, 416);
      Points.Points[1] = V2(1586, 416);
    } break;
    case ElectricalComponentSprite_Conductor_Joint:
    {
      Points.TopLeft = V2(1658, 34);
      Points.BotRight = V2(1678, 56);
      Points.Center = GetRelativeCenter(Points.TopLeft, Points.BotRight);
    } break;
  }
  return Points;
}

v2 GetCenterOffset(electrical_component* Component)
{
  r32 PixelsPerUnitLegth = 128;
  u32 SpriteTileType = ElectricalComponentToSpriteType(Component);
  bitmap_points TilePoint = GetElectricalComponentSpriteBitmapPoints(SpriteTileType);
  v2 HitBoxCenter = GetRelativeCenter(TilePoint.TopLeft, TilePoint.BotRight);
  v2 CenterOffset = (HitBoxCenter - TilePoint.Center)/PixelsPerUnitLegth;
  return CenterOffset;
}
