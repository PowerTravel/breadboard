#pragma once
#include "types.h"
#include "coordinate_systems.h"
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


inline bitmap_points GetElectricalComponentSpriteBitmapPoints(u32 Index)
{
  Index = Index % ElectricalComponentSprite_MaxSize;
  bitmap_points Points = {};
  u32 WidthPx = 1818;
  u32 HeightPx = 2498;
  switch(Index)
  {
    case ElectricalComponentSprite_Resistor_Fixed:
    {
      Points.TopLeft = BitmapCoordinatePxLh(78, 60, WidthPx, HeightPx);
      Points.BotRight = BitmapCoordinatePxLh(205, 89, WidthPx, HeightPx);
      Points.Center = GetAbsoluteCenter(Points.TopLeft, Points.BotRight);
      Points.Points[0] = BitmapCoordinatePxLh(80,75, WidthPx, HeightPx);
      Points.Points[1] = BitmapCoordinatePxLh(204,75, WidthPx, HeightPx);
    } break;
    case ElectricalComponentSprite_Ground_Earth:
    {
      Points.TopLeft = BitmapCoordinatePxLh(1616, 615, WidthPx, HeightPx);
      Points.BotRight = BitmapCoordinatePxLh(1651, 659, WidthPx, HeightPx);
      Points.Center = GetAbsoluteCenter(Points.TopLeft, Points.BotRight);
      Points.Points[0] = BitmapCoordinatePxLh(1634, 618, WidthPx, HeightPx);
    } break;
    case ElectricalComponentSprite_Diode_Led:
    {
      Points.TopLeft = BitmapCoordinatePxLh(68, 755, WidthPx, HeightPx);
      Points.BotRight = BitmapCoordinatePxLh(196, 823, WidthPx, HeightPx);
      Points.Center = GetAbsoluteCenter(BitmapCoordinatePxLh(70,787, WidthPx, HeightPx), BitmapCoordinatePxLh(181,822, WidthPx, HeightPx));
      Points.Points[0] = BitmapCoordinatePxLh(71, 805, WidthPx, HeightPx);
      Points.Points[1] = BitmapCoordinatePxLh(180, 805, WidthPx, HeightPx);
    } break;
    case ElectricalComponentSprite_Battery_SingleCell:
    {
      Points.TopLeft = BitmapCoordinatePxLh(1510, 388, WidthPx, HeightPx);
      Points.BotRight = BitmapCoordinatePxLh(1588, 446, WidthPx, HeightPx);
      Points.Center = GetAbsoluteCenter(Points.TopLeft, Points.BotRight);
      Points.Points[0] = BitmapCoordinatePxLh(1512, 416, WidthPx, HeightPx);
      Points.Points[1] = BitmapCoordinatePxLh(1586, 416, WidthPx, HeightPx);
    } break;
    case ElectricalComponentSprite_Conductor_Joint:
    {
      Points.TopLeft = BitmapCoordinatePxLh(1658, 34, WidthPx, HeightPx);
      Points.BotRight = BitmapCoordinatePxLh(1678, 56, WidthPx, HeightPx);
      Points.Center = GetAbsoluteCenter(Points.TopLeft, Points.BotRight);
    } break;
  }
  return Points;
}
