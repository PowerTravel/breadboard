#include "component_breadboard_components.h"


internal void InitiateElectricalComponent(component_electrical* ElectricalComponent, ElectricalComponentType Type)
{
  ElectricalComponent->Type = Type;
}

internal void ConnectPinToElectricalComponent(component_electrical* ElectricalComponent, component_connector_pin* Pin)
{
  component_connector_pin* TmpPin = ElectricalComponent->FirstPin;
  ElectricalComponent->FirstPin = Pin;
  Pin->NextPin = TmpPin;
  Pin->Component = ElectricalComponent;
}

internal world_coordinate GetPositionOfConnectorPin(r32 ConnectorRadius, r32 Angle)
{
  world_coordinate Result = {};

  Result.X = ConnectorRadius * Cos(Angle);
  Result.Y = ConnectorRadius * Sin(Angle); 

  return Result;
}

entity_id CreateElectricalComponent(entity_manager* EM, ElectricalComponentType EComponentType, world_coordinate WorldPos)
{
  entity_id Result = NewEntity(EM, COMPONENT_FLAG_ELECTRICAL);

  component_position* PositionComponent = GetPositionComponent(&Result);
  InitiatePositionComponent(PositionComponent, WorldPos, 0);

  const r32 ElectricalComponentRadius = 0.5f;
  component_hitbox* ElectricalComponentHitbox = GetHitboxComponent(&Result);
  InitiateCircleHitboxComponent(ElectricalComponentHitbox, PositionComponent->FirstChild, ElectricalComponentRadius);

  component_electrical* ElectricalComponent = GetElectricalComponent(&Result);
  InitiateElectricalComponent(ElectricalComponent, EComponentType);

  const r32 TriangleBase = 0.2;
  const r32 TriangleHeight = TriangleBase * Sin(Pi32/3.f);
  const r32 TriangleCenterPoint = 0.5;

  switch (EComponentType)
  {
    case ElectricalComponentType::Source:
    {
      entity_id ConnectorPin = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
      component_connector_pin* Pin = GetConnectorPinComponent(&ConnectorPin);
      Pin->Type = ElectricalPinType::Source;

      // Radial position of pin
      r32 RadialAngle = 0;
      world_coordinate RadialPosition = GetPositionOfConnectorPin(ElectricalComponentRadius, RadialAngle);
      position_node* RadialPositionNode = CreatePositionNode(RadialPosition, 0);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, RadialPositionNode);

      position_node* ConnectorPinPosition = CreatePositionNode({}, Pi32/6);
      InsertPositionNode(PositionComponent, RadialPositionNode, ConnectorPinPosition);

      component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPin);
      InitiateTriangleHitboxComponent(PinHitbox, ConnectorPinPosition, TriangleBase, TriangleHeight, TriangleCenterPoint);

      ConnectPinToElectricalComponent(ElectricalComponent, Pin);
    }break;
  }
#if 0
  else if(Pushed(Keyboard->Key_R))
  {
    entity_id ConnectorPinA = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* PinA = GetConnectorPinComponent(&ConnectorPinA);
    PinA->Type = ElectricalPinType::InputOutput;
    PinA->Component = Component;

    r32 Angle = Pi32;
    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPinA);
    hitbox_rectangle* Rectangle = &PinHitbox->Rectangle;
    CreateRectanglePinHitbox(PinA->Type, Rectangle, Angle);

    PinHitbox->Position.X = ElectricalComponentHitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = ElectricalComponentHitbox->Position.Y + ConnectorRadius * Sin(Angle);


    entity_id ConnectorPinB = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* PinB = GetConnectorPinComponent(&ConnectorPinB);
    PinB->Type = ElectricalPinType::InputOutput;
    PinB->Component = Component;

    Angle = 0;
    PinHitbox = GetHitboxComponent(&ConnectorPinB);
    Rectangle = &PinHitbox->Rectangle;
    CreateRectanglePinHitbox(PinB->Type, Rectangle, Angle);

    PinHitbox->Position.X = ElectricalComponentHitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = ElectricalComponentHitbox->Position.Y + ConnectorRadius * Sin(Angle);

    PinA->NextPin = PinB;
    Component->Type = ElectricalComponentType_Resistor;
    Component->FirstPin = PinA;
  }
  else if(Pushed(Keyboard->Key_L))
  {

    entity_id ConnectorPinAnode = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Anode = GetConnectorPinComponent(&ConnectorPinAnode);
    Anode->Type = ElectricalPinType::Positive;
    Anode->Component = Component;
    Component->FirstPin = Anode;

    r32 Angle = Pi32;
    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPinAnode);
    *PinHitbox = CreateTrianglePinHitbox(Pin->Type, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, ConnectorRadius, Angle, PinHitbox);

    entity_id ConnectorPinCathode = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Cathode = GetConnectorPinComponent(&ConnectorPinCathode);
    Cathode->Type = ElectricalPinType::Negative;
    Cathode->Component = Component;

    Angle = 0;
    PinHitbox = GetHitboxComponent(&ConnectorPinCathode);
    *PinHitbox = CreateTrianglePinHitbox(Pin->Type, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, ConnectorRadius, Angle, PinHitbox);

    PinHitbox->Position.X = ElectricalComponentHitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = ElectricalComponentHitbox->Position.Y + ConnectorRadius * Sin(Angle);

    Anode->NextPin = Cathode;
    Component->Type = ElectricalComponentType_Diode;
    Component->FirstPin = Anode;
  }
  if(Pushed(Keyboard->Key_G))
  {
    entity_id ConnectorPin = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
    component_connector_pin* Pin = GetConnectorPinComponent(&ConnectorPin);
    Pin->Type = ElectricalPinType::Sink;
    Pin->Component = Component;
    
    r32 Angle = Pi32;
    component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPin);
    hitbox_triangle* Triangle = &PinHitbox->Triangle;
    CreateTrianglePinHitbox(Pin->Type, Triangle, Angle);
    SetPositionOfConnectorPin(ElectricalComponentHitbox->Position, Angle, PinHitbox);

    PinHitbox->Position.X = Hitbox->Position.X + ConnectorRadius * Cos(Angle);
    PinHitbox->Position.Y = Hitbox->Position.Y + ConnectorRadius * Sin(Angle);

    Component->Type = ElectricalComponentType_Ground;
    Component->FirstPin = Pin;
  }
  #endif

  return Result;
}



void DeleteElectricalEntity(entity_manager* EM, entity_id ElectricalComponent)
{
  component_position* PositionComponent = GetPositionComponent(&ElectricalComponent);
  ClearPositionComponent(PositionComponent);

  component_electrical* Component = GetElectricalComponent(&ElectricalComponent);
  component_connector_pin* Pin = Component->FirstPin;
  while(Pin)
  {
    component_connector_pin* NextPin = Pin->NextPin;
    entity_id PinID = GetEntityIDFromComponent( (bptr) Pin);
    DeleteEntity(EM, &PinID);
    Pin = NextPin;
  }

  DeleteEntity(EM, &ElectricalComponent);
}