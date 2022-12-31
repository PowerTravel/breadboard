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

  const r32 RectangleSide = 0.2;

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
      position_node* RadialPositionNode = CreatePositionNode(RadialPosition, RadialAngle);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, RadialPositionNode);

      position_node* ConnectorPinPosition = CreatePositionNode({}, Pi32/6);
      InsertPositionNode(PositionComponent, RadialPositionNode, ConnectorPinPosition);

      component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPin);
      InitiateTriangleHitboxComponent(PinHitbox, ConnectorPinPosition, TriangleBase, TriangleHeight, TriangleCenterPoint);

      ConnectPinToElectricalComponent(ElectricalComponent, Pin);
    }break;
    case ElectricalComponentType::Resistor:
    {
      entity_id ConnectorPinA = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
      component_connector_pin* PinA = GetConnectorPinComponent(&ConnectorPinA);
      PinA->Type = ElectricalPinType::InputOutput;

      r32 RadialAngleA = 0;
      world_coordinate RadialPositionA = GetPositionOfConnectorPin(ElectricalComponentRadius, RadialAngleA);
      position_node* RadialPositionNodeA = CreatePositionNode(RadialPositionA, RadialAngleA);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, RadialPositionNodeA);

      position_node* ConnectorPinPositionA = CreatePositionNode({}, 0);
      InsertPositionNode(PositionComponent, RadialPositionNodeA, ConnectorPinPositionA);

      component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPinA);
      InitiateRectangleHitboxComponent(PinHitbox, ConnectorPinPositionA, RectangleSide, RectangleSide);

      ConnectPinToElectricalComponent(ElectricalComponent, PinA);


      entity_id ConnectorPinB = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
      component_connector_pin* PinB = GetConnectorPinComponent(&ConnectorPinB);
      PinB->Type = ElectricalPinType::InputOutput;

      r32 RadialAngleB = Pi32;
      world_coordinate RadialPositionB = GetPositionOfConnectorPin(ElectricalComponentRadius, RadialAngleB);
      position_node* RadialPositionNodeB = CreatePositionNode(RadialPositionB, RadialAngleB);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, RadialPositionNodeB);

      position_node* ConnectorPinPositionB = CreatePositionNode({}, 0);
      InsertPositionNode(PositionComponent, RadialPositionNodeB, ConnectorPinPositionB);

      component_hitbox* PinHitboxB = GetHitboxComponent(&ConnectorPinB);
      InitiateRectangleHitboxComponent(PinHitboxB, ConnectorPinPositionB, RectangleSide, RectangleSide);

      ConnectPinToElectricalComponent(ElectricalComponent, PinB);
    }break;
    case ElectricalComponentType::Diode:
    {

      entity_id AnodePin = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
      component_connector_pin* Anode = GetConnectorPinComponent(&AnodePin);
      Anode->Type = ElectricalPinType::Input;

      r32 AnodeRadialAngle = 0;
      world_coordinate AnodeRadialPosition = GetPositionOfConnectorPin(ElectricalComponentRadius, AnodeRadialAngle);
      position_node* AnodeRadialPositionNode = CreatePositionNode(AnodeRadialPosition, AnodeRadialAngle);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, AnodeRadialPositionNode);

      position_node* AnodeConnectorPinPosition = CreatePositionNode({}, Pi32/6);
      InsertPositionNode(PositionComponent, AnodeRadialPositionNode, AnodeConnectorPinPosition);

      component_hitbox* PinHitbox = GetHitboxComponent(&AnodePin);
      InitiateTriangleHitboxComponent(PinHitbox, AnodeConnectorPinPosition, TriangleBase, TriangleHeight, TriangleCenterPoint);

      ConnectPinToElectricalComponent(ElectricalComponent, Anode);


      entity_id ConnectorPinCathode = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
      component_connector_pin* PinCathode = GetConnectorPinComponent(&ConnectorPinCathode);
      PinCathode->Type = ElectricalPinType::Output;

      r32 RadialAngleCathode = Pi32;
      world_coordinate RadialPositionCathode = GetPositionOfConnectorPin(ElectricalComponentRadius, RadialAngleCathode);
      position_node* RadialPositionNodeCathode = CreatePositionNode(RadialPositionCathode, RadialAngleCathode);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, RadialPositionNodeCathode);

      position_node* ConnectorPinPositionCathode = CreatePositionNode({}, -Pi32/6);
      InsertPositionNode(PositionComponent, RadialPositionNodeCathode, ConnectorPinPositionCathode);

      component_hitbox* PinHitboxCathode = GetHitboxComponent(&ConnectorPinCathode);
      InitiateTriangleHitboxComponent(PinHitboxCathode, ConnectorPinPositionCathode, TriangleBase, TriangleHeight, TriangleCenterPoint);

      ConnectPinToElectricalComponent(ElectricalComponent, PinCathode);

    }break;
    case ElectricalComponentType::Ground:
    {
      entity_id ConnectorPin = NewEntity(EM, COMPONENT_FLAG_CONNECTOR_PIN);
      component_connector_pin* Pin = GetConnectorPinComponent(&ConnectorPin);
      Pin->Type = ElectricalPinType::Sink;

      // Radial position of pin
      r32 RadialAngle = Pi32;
      world_coordinate RadialPosition = GetPositionOfConnectorPin(ElectricalComponentRadius, RadialAngle);
      position_node* RadialPositionNode = CreatePositionNode(RadialPosition, RadialAngle);
      InsertPositionNode(PositionComponent, PositionComponent->FirstChild, RadialPositionNode);

      position_node* ConnectorPinPosition = CreatePositionNode({}, -Pi32/6);
      InsertPositionNode(PositionComponent, RadialPositionNode, ConnectorPinPosition);

      component_hitbox* PinHitbox = GetHitboxComponent(&ConnectorPin);
      InitiateTriangleHitboxComponent(PinHitbox, ConnectorPinPosition, TriangleBase, TriangleHeight, TriangleCenterPoint);

      ConnectPinToElectricalComponent(ElectricalComponent, Pin);
    }break;
  }

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