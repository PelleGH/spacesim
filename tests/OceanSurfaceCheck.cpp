#include "world/StarSystemLoader.h"
#include "world/PlanetTerrainGenerator.h"
#include "planet/PlanetSurfaceSampler.h"
#include "rendering/DistantBodyRenderer.h"
#include "rendering/PlanetSurfacePatch.h"
#include <raymath.h>
#include <rlgl.h>
#include <cmath>
#include <cstdio>
#include <string>
#include "PlanetTileChecks.h"
#include "rendering/AdaptivePlanetRenderer.h"
#include <thread>
#include <chrono>

int main(int argc, char** argv)
{
    SpaceSim::GameWorld world;
    world.starSystem = SpaceSim::LoadStarSystemFromJson("data/systems/test_system.json");
    int index = -1;
    for (int i = 0; i < static_cast<int>(world.starSystem.objects.size()); ++i)
        if (world.starSystem.objects[i].hasPlanetData &&
            world.starSystem.objects[i].planetData.planetClass == SpaceSim::PlanetClass::OceanWorld)
            index = i;
    if (index < 0) return 1;
    const auto& planet = world.starSystem.objects[index];
    const int seed = planet.planetData.seed == 0 ? SpaceSim::PlanetSurfaceSampler::seedFromId(planet.id)
        : static_cast<int>(planet.planetData.seed);
    SpaceSim::DVec3 starPosition{};
    for (int i = 0; i < static_cast<int>(world.starSystem.objects.size()); ++i)
    {
        const auto& body = world.starSystem.objects[i];
        if (body.type != SpaceSim::GlobalObjectType::Sun) continue;
        starPosition = body.position;
        world.lighting.hasPrimaryStar = true;
        world.lighting.primaryStarIndex = i;
        world.lighting.starGlobalPosition = body.position;
        world.lighting.starColor = Vector3{
            static_cast<float>(body.color.r) / 255.0f,
            static_cast<float>(body.color.g) / 255.0f,
            static_cast<float>(body.color.b) / 255.0f
        };
        break;
    }
    const auto toSun = SpaceSim::Normalize(starPosition - planet.position);
    const Vector3 sunDirection{static_cast<float>(toSun.x), static_cast<float>(toSun.y), static_cast<float>(toSun.z)};
    int oceans = 0, land = 0;
    float highestTerrain = 0.0f;
    int mountains = 0, lowlands = 0;
    Vector3 coastDirection{0,0,-1};
    float coastError = 1.0f;
    for (int i = 0; i < 4096; ++i)
    {
        const float y = 1.0f - 2.0f * (i + 0.5f) / 4096.0f;
        const float r = std::sqrt(1.0f - y*y), angle = i * 2.39996323f;
        const Vector3 direction = Vector3Normalize(Vector3{ r*std::cos(angle), y, r*std::sin(angle) });
        const auto surface = SpaceSim::PlanetSurfaceSampler::sample(direction, seed, SpaceSim::PlanetClass::OceanWorld);
        const auto orbital = SpaceSim::SamplePlanetTerrain(planet, direction);
        if (orbital.height < 0.0f || orbital.height > 0.002f) return 6;
        highestTerrain = std::max(highestTerrain, surface.terrainHeight);
        if (surface.terrainHeight > 20.0f) ++mountains;
        if (!surface.isOcean && surface.terrainHeight < 5.0f) ++lowlands;
        if (std::fabs(direction.y) < 0.6f &&
            Vector3DotProduct(direction, sunDirection) > 0.6f &&
            std::fabs(surface.continentValue - 0.555f) < coastError)
        {
            coastError = std::fabs(surface.continentValue - 0.555f);
            coastDirection = direction;
        }
        if (std::fabs(orbital.height * SpaceSim::PlanetSurfaceSampler::TerrainUnitsPerRadius - surface.terrainHeight) > 0.001f)
        {
            std::printf("Height mismatch at %d: globe=%f patch=%f\n", i,
                orbital.height * SpaceSim::PlanetSurfaceSampler::TerrainUnitsPerRadius, surface.terrainHeight);
            return 2;
        }
        if (surface.isOcean) { ++oceans; if (orbital.height != 0.0f) return 3; }
        else ++land;
    }
    std::printf("4096 shared-height checks passed; water=%d land=%d\n", oceans, land);
    if (oceans == 0 || land == 0) return 4;
    std::printf("Relief: peak=%.2f, mountain samples=%d, lowland samples=%d\n", highestTerrain, mountains, lowlands);
    if (mountains < 10 || lowlands < 10 || highestTerrain >= 60.0f) return 5;
    // Approaching the same world refines local spacing without inflating relief.
    const float highPatch = SpaceSim::PlanetSurfacePatch::halfSizeForAltitude(650, 14000, 2000);
    const float lowPatch = SpaceSim::PlanetSurfacePatch::halfSizeForAltitude(650, 14000, 1000);
    if (!(highPatch > lowPatch && lowPatch > 0.0f)) return 7;
    const float minimumPatch = SpaceSim::PlanetSurfacePatch::halfSizeForAltitude(650, 14000, 0);
    if (!(minimumPatch > 0.0f && minimumPatch < lowPatch)) return 8;
    std::printf("Peak radius fraction=%.6f; patch half-width %.2f -> %.2f -> %.2f\n",
        highestTerrain / SpaceSim::PlanetSurfaceSampler::TerrainUnitsPerRadius,
        highPatch, lowPatch, minimumPatch);
    if (!CheckPlanetTiles(seed)) return 13;
    if (argc > 1 && std::string(argv[1]) == "--cpu") return 0;

    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_MSAA_4X_HINT);
    InitWindow(1000, 700, "Ocean surface render check");
    rlSetClipPlanes(0.05, 5000.0);
    const Vector3 center = Vector3Scale(coastDirection, -700.0f);
    const Vector3 cameraUp = Vector3Normalize(Vector3CrossProduct(
        Vector3CrossProduct(coastDirection, Vector3{0,1,0}), coastDirection));
    world.camera = Camera3D{ {0,0,0}, center, cameraUp, 60, CAMERA_PERSPECTIVE };
    world.planetTransition.closestPlanetIndex = index;
    world.planetTransition.mode = SpaceSim::PlanetRenderMode::Surface;
    world.planetTransition.altitude = 1000.0;
    {
    SpaceSim::DistantBodyRenderer renderer;
    SpaceSim::PlanetSurfacePatch patch;
    patch.update(world, {true, center, 650.0f});
    const auto initialRevision = patch.geometryRevision();
    if (initialRevision == 0) return 9;
    patch.update(world, {true, center, 650.0f});
    if (patch.geometryRevision() != initialRevision) return 10;
    // A floating-origin translation must preserve the planet-local geometry.
    const Vector3 offset{100, 20, -30};
    world.camera.position = offset;
    patch.update(world, {true, Vector3Add(center, offset), 650.0f});
    if (patch.geometryRevision() != initialRevision) return 11;
    world.camera.position = {0,0,0};
    world.planetTransition.altitude = 500.0;
    patch.update(world, {true, center, 650.0f});
    if (patch.geometryRevision() == initialRevision) return 12;
    world.planetTransition.altitude = 1000.0;
    std::printf("Terrain cache reuse, origin translation and approach refinement checks passed\n");
    for (int shot = 0; shot < 3; ++shot)
    {
        const float radius = shot == 0 ? 250.0f : 650.0f;
        BeginDrawing();
        ClearBackground(Color{ 9,15,25,255 });
        BeginMode3D(world.camera);
        if (shot == 2)
        {
            patch.update(world, {true, center, radius});
            patch.draw();
        }
        renderer.renderLocalPlanet(planet, world.lighting, center, radius, 1.0f, world.camera.position);
        EndMode3D();
        EndDrawing();
        Image frame = LoadImageFromScreen();
        ExportImage(frame, shot == 0 ? "build/ocean-orbit.png" :
            shot == 1 ? "build/ocean-close-base.png" : "build/ocean-close-patch.png");
        UnloadImage(frame);
    }
    SpaceSim::AdaptivePlanetRenderer adaptive;
    for (int shot=0;shot<3;++shot) {
        const float radius=shot==0?250.0f:shot==1?650.0f:697.0f;
        if (shot==2) world.camera.target=Vector3Add(Vector3Scale(Vector3Normalize(center),12),Vector3Scale(cameraUp,100));
        bool ready=false;
        const auto start=std::chrono::steady_clock::now();
        for(int frame=0;frame<1200;++frame) {
            ready=adaptive.update(planet,center,radius,world.camera,700);
            const auto stats=adaptive.statistics();
            if(stats.resident>512 || stats.pending>2 || stats.uploaded>2) return 14;
            if(ready && std::fabs(stats.coveredFaces-6.0)>1e-9) return 18;
            if(ready && stats.pending==0) break;
            if(std::chrono::steady_clock::now()-start>std::chrono::seconds(20)) return 15;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if(!ready) return 16;
        const auto stats=adaptive.statistics();
        if(stats.visible<6 || (shot>0 && stats.deepest<2)) return 17;
        std::printf("Adaptive shot %d: resident=%d visible=%d level=%d pending=%d\n",shot,stats.resident,stats.visible,stats.deepest,stats.pending);
        BeginDrawing(); ClearBackground(Color{9,15,25,255}); BeginMode3D(world.camera);
        adaptive.draw(planet,world.lighting,center,radius,world.camera.position);
        EndMode3D(); EndDrawing();
        Image frame=LoadImageFromScreen();
        ExportImage(frame,shot==0?"build/adaptive-orbit.png":shot==1?"build/adaptive-close.png":"build/adaptive-horizon.png");
        UnloadImage(frame);
    }
    // Sweep the camera around the planet while requests are still in flight.
    // Every frame must retain complete coverage and respect the cache limits.
    double maxUpdateMs=0;
    for(int step=0;step<480;++step) {
        const float angle=(step/40)*.52f;
        const Vector3 movingCenter{700*std::sin(angle),0,700*std::cos(angle)};
        const auto started=std::chrono::steady_clock::now();
        const bool ready=adaptive.update(planet,movingCenter,697,world.camera,700);
        maxUpdateMs=std::max(maxUpdateMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count());
        const auto stats=adaptive.statistics();
        if(!ready || std::fabs(stats.coveredFaces-6.0)>1e-9 || stats.resident>512 || stats.pending>2 || stats.uploaded>2) return 19;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::printf("480 movement/cache-pressure frames passed; maximum update %.3f ms (not whole-game frame time)\n",maxUpdateMs);
    auto otherPlanet=planet;
    otherPlanet.id+="-cache-isolation-check";
    otherPlanet.planetData.seed+=19;
    if(adaptive.update(otherPlanet,center,650,world.camera,700)) return 20;
    if(adaptive.statistics().resident!=0) return 21;
    std::puts("Planet switch discarded old geometry and restored loading fallback");
    std::puts("Adaptive streaming and upload-budget checks passed");
    } // Release terrain GPU resources before closing the graphics context.
    CloseWindow();
    return 0;
}
