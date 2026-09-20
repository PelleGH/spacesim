#pragma once
#include "world/GameWorld.h"
#include "systems/ShipControlSystem.h"
#include "systems/CameraSystem.h"
#include "systems/FTLSystem.h"
#include "systems/PlayerGlobalPositionSystem.h"
#include "systems/PlanetProximitySystem.h"
#include "systems/OrbitalCruiseSystem.h"
#include "systems/AtmosphereSystem.h"
#include "systems/BubbleSystem.h"
#include "systems/LightingSystem.h"
namespace SpaceSim {
class GameSimulation {
public:
    GameSimulation();
    void update(float dt);
    GameWorld world;
    ShipControlSystem controls;
private:
    CameraSystem camera;
    FTLSystem ftl;
    PlayerGlobalPositionSystem globalPosition;
    PlanetProximitySystem proximity;
    OrbitalCruiseSystem cruise;
    AtmosphereSystem atmosphere;
    BubbleSystem bubble;
    LightingSystem lighting;
};
}
