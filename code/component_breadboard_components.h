#pragma once
#include "entity_components_backend.h"
#include "types.h"
#include "coordinate_systems.h"
#include "math/rect2f.h"

enum class ElectricalComponentType
{
  None,
  Source,
  Ground,
  Diode,
  Led_Red,
  Led_Green,
  Led_Blue,
  Resistor,
  Wire,
  COUNT
};


const c8* ComponentTypeToString(u32 Type)
{
  switch(Type)
  { 
    case ElectricalComponentType::Source: return "Source";
    case ElectricalComponentType::Ground: return "Ground";
    case ElectricalComponentType::Led_Red: return "Led_Red";
    case ElectricalComponentType::Led_Green: return "Led_Green";
    case ElectricalComponentType::Led_Blue: return "Led_Blue";
    case ElectricalComponentType::Resistor: return "Resistor";
    case ElectricalComponentType::Wire: return "Wire";
  }
  return "";
};

enum class ElectricalPinType
{
  Positive,
  Negative,
  Output,
  Input,
  InputOutput,
  A,
  B,
  Source,
  Sink,
  Count
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

struct component_electrical;

struct component_connector_pin
{
  ElectricalPinType Type;
  component_connector_pin* NextPin;
  component_electrical* Component;
};

struct component_electrical
{
  ElectricalComponentType Type;
  component_connector_pin* FirstPin;
};

entity_id CreateElectricalComponent(entity_manager* EM, ElectricalComponentType EComponentType, world_coordinate WorldPos);
void DeleteElectricalEntity(entity_manager* EM, entity_id ElectricalComponent);