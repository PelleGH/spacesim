#pragma once
#include "world/GameWorld.h"
#include "renderer/RenderCamera.h"
#include "renderer/RenderObject.h"
#include "renderer/planet/PlanetRenderObject.h"
#include "renderer/lighting/DirectionalLight.h"
#include "renderer/atmosphere/AtmosphereParameters.h"
#include <vector>
namespace SpaceSim {
// One uniform scale for all scene geometry. Simulation units remain unchanged.
inline constexpr double RenderUnitsPerGameUnit=.001;
// Prototype calibration, not a claim that the original game's units were physical.
inline constexpr double KilometresPerGameUnit=1.0;
struct GameRenderFrame {
    RenderCamera camera;
    DirectionalLight sun;
    std::vector<PlanetRenderObject> planets;
    std::vector<RenderObject> objects;
    int atmosphereBody=-1;
    glm::vec3 atmosphereCenter{};
    float atmosphereRadius=0;
};
GameRenderFrame makeGameRenderFrame(const GameWorld& world,const GpuMesh& sphere,const GpuMesh& box);
AtmosphereParameters gameAtmosphere(const GlobalObject& body);
}
