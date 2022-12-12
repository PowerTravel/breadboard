#pragma once

#include "math/affine_transformations.h"
#include "bitmap.h"

#include "Assets.h"
#include "breadboard_tile.h"
#include "data_containers.h"
#include "math/rect2f.h"

struct component_camera;
struct component_controller;
struct component_hitbox;
struct component_position;
struct component_electrical;
struct component_connector_pin;

struct keyboard_input;
struct mouse_input;
struct game_controller_input;
struct entity_manager;

struct component_controller
{
  keyboard_input* Keyboard;
  mouse_input* Mouse;
  game_controller_input* Controller;
};

enum component_type
{
  COMPONENT_FLAG_NONE               = 0,
  COMPONENT_FLAG_POSITION           = 1<<0,
  COMPONENT_FLAG_HITBOX             = 1<<1,
  COMPONENT_FLAG_ELECTRICAL         = 1<<2,
  COMPONENT_FLAG_CONNECTOR_PIN      = 1<<3,
  COMPONENT_FLAG_CAMERA             = 1<<4,
  COMPONENT_FLAG_CONTROLLER         = 1<<5,
  COMPONENT_FLAG_FINAL              = 1<<6,
};

// Initiates the backend entity manager with breadboard specific components
entity_manager* CreateEntityManager();

#define GetCameraComponent(EntityID) ((component_camera*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CAMERA))
#define GetControllerComponent(EntityID) ((component_controller*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CONTROLLER))
#define GetHitboxComponent(EntityID) ((component_hitbox*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_HITBOX))
#define GetPositionComponent(EntityID) ((component_position*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_POSITION))
#define GetElectricalComponent(EntityID) ((component_electrical*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_ELECTRICAL))
#define GetConnectorPinComponent(EntityID) ((component_connector_pin*) GetComponent(GlobalGameState->EntityManager, EntityID, COMPONENT_FLAG_CONNECTOR_PIN))