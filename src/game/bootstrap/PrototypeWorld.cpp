#include "game/bootstrap/PrototypeWorld.h"

#include "game/ecs/components/CameraComponent.h"
#include "game/ecs/components/GlobalPositionComponent.h"
#include "game/ecs/components/HyperdriveComponents.h"
#include "game/ecs/components/LocalBubbleTransientComponent.h"
#include "game/ecs/components/LightingComponents.h"
#include "game/ecs/components/PlanetComponents.h"
#include "game/ecs/components/PlayerControlledComponent.h"
#include "game/ecs/components/RenderableComponent.h"
#include "game/ecs/components/ScaleReferenceComponent.h"
#include "game/ecs/components/ShipControlComponent.h"
#include "game/ecs/components/ShipMovementComponent.h"
#include "game/ecs/components/TransformComponent.h"
#include "game/world/SpaceScale.h"
#include "game/ecs/components/PreviousTransformComponent.h"
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

namespace SpaceSim
{
    namespace
    {
        glm::quat orientationFromForwardUp(const glm::vec3& forward, const glm::vec3& up)
        {
            const glm::vec3 f = glm::normalize(forward);
            const glm::vec3 right = glm::normalize(glm::cross(f, up));
            const glm::vec3 correctedUp = glm::normalize(glm::cross(right, f));

            glm::mat3 basis(1.0f);
            basis[0] = right;
            basis[1] = correctedUp;
            basis[2] = -f;
            return glm::normalize(glm::quat_cast(basis));
        }

        void createScaleReference(
            GameWorld& world,
            const glm::dvec3& positionMeters,
            const glm::vec3& sizeMeters,
            const glm::vec3& color,
            float emissiveStrength)
        {
            const entt::entity entity = world.registry.create();

            TransformComponent transform;
            transform.positionMeters = positionMeters;
            world.registry.emplace<TransformComponent>(entity, transform);

            ScaleReferenceComponent reference;
            reference.sizeMeters = sizeMeters;
            reference.baseColor = color;
            reference.emissiveStrength = emissiveStrength;
            world.registry.emplace<ScaleReferenceComponent>(entity, reference);
            world.registry.emplace<LocalBubbleTransientComponent>(entity);
        }
    }

    void createPrototypeWorld(GameWorld& world)
    {
        // -----------------------------------------------------------------
        // Global layer: a physically sized Earth-like planet at system origin.
        // -----------------------------------------------------------------
        world.primaryPlanet = world.registry.create();

        AtmosphereComponent atmosphere;
        atmosphere.parameters = makeEarthLikeAtmosphere();
        atmosphere.parameters.groundAlbedo = {0.10f, 0.14f, 0.18f};

        PlanetComponent planet;
        planet.radiusMeters =
            static_cast<double>(atmosphere.parameters.bottomRadiusKm) *
            SpaceScale::MetersPerKilometer;
        planet.hasOcean = true;

        GlobalPositionComponent planetPosition;
        planetPosition.positionMeters = glm::dvec3(0.0);

        world.registry.emplace<GlobalPositionComponent>(world.primaryPlanet, planetPosition);
        world.registry.emplace<PlanetComponent>(world.primaryPlanet, planet);
        world.registry.emplace<AtmosphereComponent>(world.primaryPlanet, atmosphere);

        JumpPointComponent earthJumpPoint;
        earthJumpPoint.displayName = "Earth";
        // First hyperdrive arrival stays just above the atmosphere. The current
        // renderer still uses the orbital/distant planet representation; once
        // the true surface transition exists this can be lowered substantially.
        earthJumpPoint.arrivalDistanceMeters = planet.radiusMeters + 1000.0;
        world.registry.emplace<JumpPointComponent>(world.primaryPlanet, earthJumpPoint);

        PlanetVisualComponent visual;
        visual.material.deepOceanColor = {0.006f, 0.022f, 0.055f};
        visual.material.shallowOceanColor = {0.020f, 0.100f, 0.145f};
        visual.material.oceanRoughness = 0.10f;
        visual.material.lowLandColor = {0.08f, 0.20f, 0.055f};
        visual.material.highLandColor = {0.38f, 0.30f, 0.17f};
        visual.material.landRoughness = 0.82f;
        visual.material.continentScale = 2.4f;
        visual.material.detailScale = 10.0f;
        visual.material.oceanLevel = 0.56f;
        visual.material.coastWidth = 0.018f;
        visual.material.seed = 13.37f;
        world.registry.emplace<PlanetVisualComponent>(world.primaryPlanet, visual);

        world.primaryStar = world.registry.create();
        PrimaryStarComponent star;
        star.direction = glm::normalize(star.direction);
        world.registry.emplace<PrimaryStarComponent>(world.primaryStar, star);

        // -----------------------------------------------------------------
        // Local gameplay bubble: one local metre is one physical metre.
        // The bubble currently sits six planetary radii from the planet center,
        // giving us a clearly visible but genuinely distant orbital backdrop.
        // -----------------------------------------------------------------
        world.localBubbleOriginMeters = glm::dvec3(
            0.0,
            0.0,
            planet.radiusMeters * 6.0);

        world.playerShip = world.registry.create();

        TransformComponent shipTransform;
        shipTransform.positionMeters = glm::dvec3(0.0);

        const glm::dvec3 shipGlobalMeters = world.localToGlobalMeters(shipTransform.positionMeters);
        const glm::dvec3 toPlanetMeters = planetPosition.positionMeters - shipGlobalMeters;
        shipTransform.rotation = orientationFromForwardUp(
            glm::normalize(glm::vec3(toPlanetMeters)),
            glm::vec3(0.0f, 1.0f, 0.0f));

        world.registry.emplace<TransformComponent>(world.playerShip, shipTransform);
        world.registry.emplace<PlayerControlledComponent>(world.playerShip);
        world.registry.emplace<ShipControlComponent>(world.playerShip);
        world.registry.emplace<ShipMovementComponent>(world.playerShip);
        world.registry.emplace<HyperdriveComponent>(world.playerShip);
        world.registry.emplace<HyperdriveControlComponent>(world.playerShip);
        world.registry.emplace<RenderableComponent>(world.playerShip);

        world.activeCamera = world.registry.create();
        TransformComponent cameraTransform;
        cameraTransform.positionMeters = shipTransform.positionMeters;
        cameraTransform.rotation = shipTransform.rotation;
        world.registry.emplace<TransformComponent>(world.activeCamera, cameraTransform);

        CameraComponent camera;
        camera.verticalFovDegrees = 62.0f;
        camera.nearClipMeters = 0.10f;
        camera.farClipMeters = 100000.0f;
        world.registry.emplace<CameraComponent>(world.activeCamera, camera);

        // -----------------------------------------------------------------
        // Temporary physical scale references. These are deliberately placed
        // off the centerline so the planet remains readable behind them.
        // -----------------------------------------------------------------
        createScaleReference(
            world,
            {-55.0, 0.0, -120.0},
            {10.0f, 10.0f, 10.0f},
            {0.86f, 0.66f, 0.15f},
            0.55f);

        createScaleReference(
            world,
            {85.0, 18.0, -340.0},
            {25.0f, 25.0f, 25.0f},
            {0.22f, 0.64f, 0.82f},
            0.40f);

        createScaleReference(
            world,
            {-180.0, -35.0, -900.0},
            {100.0f, 100.0f, 100.0f},
            {0.66f, 0.30f, 0.72f},
            0.30f);

        createScaleReference(
            world,
            {320.0, 70.0, -2200.0},
            {250.0f, 250.0f, 250.0f},
            {0.28f, 0.74f, 0.38f},
            0.20f);
    }
}
