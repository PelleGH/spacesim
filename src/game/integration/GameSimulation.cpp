#include "integration/GameSimulation.h"
#include "world/StarSystemLoader.h"
#include "components/TransformComponent.h"
#include "components/ShipFlightComponent.h"
#include "components/PlayerControlledComponent.h"
#include "components/RenderableComponent.h"
#include <raymath.h>
#include <stdexcept>
namespace SpaceSim {
GameSimulation::GameSimulation() {
    world.starSystem=LoadStarSystemFromJson("data/systems/test_system.json");
    for(int i=0;i<int(world.starSystem.objects.size());++i) {
        const auto& body=world.starSystem.objects[i];
        if(body.isJumpTarget && world.selectedJumpTarget<0) world.selectedJumpTarget=i;
        if(body.id=="rocky_relay") {world.selectedJumpTarget=i;break;}
    }
    if(world.selectedJumpTarget<0) throw std::runtime_error("System has no starting relay");
    world.activePoi=world.selectedJumpTarget;
    world.activeBubbleOrigin=world.starSystem.objects[world.activePoi].position;
    auto ship=world.registry.create(); world.playerShip=ship;
    auto& transform=world.registry.emplace<TransformComponent>(ship);
    transform.position={0,0,-300};
    world.registry.emplace<ShipFlightComponent>(ship);
    world.registry.emplace<PlayerControlledComponent>(ship);
    auto& renderable=world.registry.emplace<RenderableComponent>(ship);renderable.type=RenderableType::Ship;renderable.color=SKYBLUE;
    camera.initialize(world);
    globalPosition.update(world);proximity.update(world);lighting.update(world);
    // Start looking toward the nearest planet so the new rendering is immediately visible.
    if(world.planetTransition.closestPlanetIndex>=0) {
        auto d=Normalize(world.starSystem.objects[world.planetTransition.closestPlanetIndex].position-world.globalPlayerPosition);
        transform.rotation=d.z < -.999999 ? QuaternionFromAxisAngle({0,1,0},PI) : QuaternionFromVector3ToVector3({0,0,1},{float(d.x),float(d.y),float(d.z)});
    }
    camera.update(world,0);
}
void GameSimulation::update(float dt) {
    ftl.update(world,dt);
    if(world.travelMode==TravelMode::NormalFlight) controls.update(world,dt);
    globalPosition.update(world);proximity.update(world);
    cruise.update(world,dt);
    globalPosition.update(world);proximity.update(world);atmosphere.update(world);
    bubble.update(world);lighting.update(world);camera.update(world,dt);
}
}
