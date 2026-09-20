#include "integration/GameRenderBridge.h"
#include "components/TransformComponent.h"
#include "world/SpaceCoordinates.h"
#include "components/RenderableComponent.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
namespace SpaceSim {
namespace {
glm::vec3 vec(Vector3 v){return {v.x,v.y,v.z};}
glm::vec3 relative(DVec3 body,DVec3 eye){auto d=body-eye;return {float(d.x*RenderUnitsPerGameUnit),float(d.y*RenderUnitsPerGameUnit),float(d.z*RenderUnitsPerGameUnit)};}
glm::vec3 linear(Color c){auto f=[](float v){v/=255;return v<=.04045f?v/12.92f:std::pow((v+.055f)/1.055f,2.4f);};return {f(c.r),f(c.g),f(c.b)};}
}
AtmosphereParameters gameAtmosphere(const GlobalObject& body) {
    auto p=makeEarthLikeAtmosphere();
    p.bottomRadiusKm=float(body.visualRadius*KilometresPerGameUnit);
    // Match the existing game's atmosphere extent for this integration milestone.
    const float thickness=4000.0f*float(KilometresPerGameUnit);
    const float scale=thickness/100.0f;
    p.topRadiusKm=p.bottomRadiusKm+thickness;
    p.rayleighScaleHeightKm*=scale;p.mieScaleHeightKm*=scale;
    p.ozoneCenterHeightKm*=scale;p.ozoneHalfWidthKm*=scale;
    p.rayleighScatteringPerKm/=scale;p.mieScatteringPerKm/=scale;
    p.mieExtinctionPerKm/=scale;p.ozoneAbsorptionPerKm/=scale;
    if(body.planetData.atmosphere==AtmosphereType::Thin) {p.rayleighScatteringPerKm*=.25f;p.mieScatteringPerKm*=.25f;p.mieExtinctionPerKm*=.25f;}
    return p;
}
GameRenderFrame makeGameRenderFrame(const GameWorld& world,const GpuMesh& sphere,const GpuMesh& box) {
    GameRenderFrame frame;
    const auto& camera=world.camera;
    const auto& player=world.registry.get<TransformComponent>(world.playerShip);
    const DVec3 eye=world.globalPlayerPosition+DVec3{camera.position.x-player.position.x,camera.position.y-player.position.y,camera.position.z-player.position.z}*LocalToGlobalScale;
    frame.camera.position={0,0,0};
    frame.camera.forward=glm::normalize(vec(camera.target)-vec(camera.position));frame.camera.up=vec(camera.up);
    frame.camera.verticalFovDegrees=camera.fovy;frame.camera.nearPlane=.0000001f;frame.camera.farPlane=2000;
    frame.sun.sourceVisible=world.lighting.hasPrimaryStar;
    if(frame.sun.sourceVisible) {
        const auto& star=world.starSystem.objects[world.lighting.primaryStarIndex];
        auto d=Normalize(star.position-eye);frame.sun.direction={float(d.x),float(d.y),float(d.z)};
        frame.sun.radiance=vec(world.lighting.starColor)*20.0f;
        frame.sun.sourceDiskRadiance=vec(world.lighting.starColor)*60.0f;
        frame.sun.sourceAngularRadiusRadians=float(std::asin(std::clamp(star.visualRadius/std::max(Length(star.position-eye),star.visualRadius),0.0,1.0)));
    }
    for(int i=0;i<int(world.starSystem.objects.size());++i) {
        const auto& body=world.starSystem.objects[i];
        if(body.type!=GlobalObjectType::Planet) continue;
        PlanetRenderObject planet;planet.mesh=&sphere;
        const auto center=relative(body.position,eye);
        const float radius=float(body.visualRadius*RenderUnitsPerGameUnit);
        planet.modelMatrix=glm::translate(glm::mat4(1),center)*glm::scale(glm::mat4(1),glm::vec3(radius));
        planet.radiusKm=float(body.visualRadius*KilometresPerGameUnit);
        planet.hasOcean=body.hasPlanetData && body.planetData.planetClass==PlanetClass::OceanWorld;
        planet.material.seed=float(body.planetData.seed%10000)*.017f;
        if(!planet.hasOcean) {
            planet.material.oceanLevel=-2;planet.material.lowLandColor=linear(body.color);
            planet.material.highLandColor=planet.material.lowLandColor*.55f;
        }
        frame.planets.push_back(planet);
        if(i==world.planetTransition.closestPlanetIndex && body.hasPlanetData && body.planetData.atmosphere!=AtmosphereType::None &&
            world.planetTransition.mode!=PlanetRenderMode::Distant) {
            frame.atmosphereBody=i;frame.atmosphereCenter=center;frame.atmosphereRadius=radius;
        }
    }
    auto view=world.registry.view<const TransformComponent,const RenderableComponent>();
    for(auto entity:view) {
        if(world.travelMode==TravelMode::FTLTravel) continue;
        const auto& t=view.get<const TransformComponent>(entity);const auto& r=view.get<const RenderableComponent>(entity);
        RenderObject object;object.mesh=&box;
        auto pos=relative(LocalToGlobalPosition(world,t.position),eye);
        glm::quat rotation(t.rotation.w,t.rotation.x,t.rotation.y,t.rotation.z);
        const float size=entity==world.playerShip?2.0f:8.0f;
        object.modelMatrix=glm::translate(glm::mat4(1),pos)*glm::mat4_cast(rotation)*glm::scale(glm::mat4(1),vec(t.scale)*float(RenderUnitsPerGameUnit*LocalToGlobalScale)*size);
        object.material.baseColor=linear(r.color);object.material.roughness=.55f;
        frame.objects.push_back(object);
    }
    return frame;
}
}
