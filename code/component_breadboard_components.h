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

struct circuit_edge_spline
{
  world_coordinate Point;
  v2 InputCurvature;
  v2 OutputCurvature;
  circuit_edge_spline* Next;
};

struct circuit_node_header;

struct circuit_edge
{
  circuit_node_header* A;  // The Index this edge is located in Node A's circuit edge list is its Connection ID
  circuit_node_header* B;  // The Index this edge is located in Node B's circuit edge list is its Connection ID
  circuit_edge_spline* Points; // Where does the wire pass through in the world
};

struct circuit_node_header
{
  ElectricalComponentType Type; // The type of electric component
  u32 BodySize;                 // Size of body in bytes
  u32 TotalEdgeCount;           // The order of the edges dictates its function, defined by the type.
  circuit_edge* Edges;          // Edges connecting to other nodes
};

struct electrical_circuit_memory
{
  chunk_list Nodes;
  chunk_list Edges;
  chunk_list Splines;
};

struct electrical_circuit
{
  u32 SourceNodeCount;
  circuit_node_header* SourceNodes;
  linked_memory Nodes;
  chunk_list Edges;
  chunk_list Splines;
};

electrical_circuit NewElectricalCircuit(memory_arena* Arena)
{
  u32 NodeChunkSize = 128;
  u32 EdgeChunkSize = 2*NodeChunkSize;
  u32 SplineChunkSize = 4*NodeChunkSize;
  electrical_circuit Result = {};
  //u32 MaxLinkedMemorySize = 
//  InitiateLinkedMemory(Arena, &Result.Nodes, sizeof(circuit_node_header), NodeChunkSize);
  Result.Edges   = NewChunkList(Arena, sizeof(circuit_edge), EdgeChunkSize);
  Result.Splines = NewChunkList(Arena, sizeof(circuit_edge_spline), SplineChunkSize);
  return Result;
}

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