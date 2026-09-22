#include "game/rendering/RenderSystem.h"

#include "game/ecs/components/CameraComponent.h"
#include "game/ecs/components/GlobalPositionComponent.h"
#include "game/ecs/components/LightingComponents.h"
#include "game/ecs/components/PlanetComponents.h"
#include "game/ecs/components/RenderableComponent.h"
#include "game/ecs/components/ScaleReferenceComponent.h"
#include "game/ecs/components/TransformComponent.h"
#include "game/rendering/GameRenderResources.h"
#include "game/world/SpaceScale.h"

#include "renderer/RenderCamera.h"
#include "renderer/SceneRenderer.h"
#include "renderer/atmosphere/AtmosphereInstance.h"
#include "renderer/lighting/DirectionalLight.h"
#include "renderer/lighting/EnvironmentLight.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>


namespace SpaceSim
{
    namespace
    {
        glm::vec3 metersToRender(
            const glm::dvec3& meters)
        {
            return
                glm::vec3(
                    meters)
                *
                SpaceScale::RenderUnitsPerLocalMeter;
        }


        glm::vec3 metersToRender(
            const glm::vec3& meters)
        {
            return
                meters
                *
                SpaceScale::RenderUnitsPerLocalMeter;
        }


        glm::mat4 cameraRelativeWorldMatrix(
            const TransformComponent& transform,
            const glm::dvec3& cameraPositionMeters)
        {
            const glm::dvec3 relativeMeters =
                transform.positionMeters
                -
                cameraPositionMeters;


            return
                glm::translate(
                    glm::mat4(
                        1.0f),
                    metersToRender(
                        relativeMeters))
                *
                glm::mat4_cast(
                    transform.rotation)
                *
                glm::scale(
                    glm::mat4(
                        1.0f),
                    transform.scale);
        }


        PbrMaterial materialFrom(
            const RenderableComponent& renderable)
        {
            PbrMaterial material;

            material.baseColor =
                renderable.baseColor;

            material.metallic =
                renderable.metallic;

            material.roughness =
                renderable.roughness;


            return
                material;
        }


        void pushBoxRenderUnits(
            std::vector<RenderObject>& objects,
            const GpuMesh& box,
            const glm::mat4& root,
            const glm::vec3& localPositionRenderUnits,
            const glm::vec3& localSizeRenderUnits,
            const PbrMaterial& material)
        {
            RenderObject object;

            object.mesh =
                &box;


            object.modelMatrix =
                root
                *
                glm::translate(
                    glm::mat4(
                        1.0f),
                    localPositionRenderUnits)
                *
                glm::scale(
                    glm::mat4(
                        1.0f),
                    localSizeRenderUnits);


            object.material =
                material;


            objects.push_back(
                object);
        }


        void pushBoxMeters(
            std::vector<RenderObject>& objects,
            const GpuMesh& box,
            const glm::mat4& root,
            const glm::vec3& localPositionMeters,
            const glm::vec3& localSizeMeters,
            const PbrMaterial& material)
        {
            pushBoxRenderUnits(
                objects,
                box,
                root,
                metersToRender(
                    localPositionMeters),
                metersToRender(
                    localSizeMeters),
                material);
        }
    }


    void RenderSystem::render(
        GameWorld& world,
        SceneRenderer& renderer,
        const GameRenderResources& resources,
        int width,
        int height)
    {
        if (
            world.activeCamera
                ==
                entt::null
            ||
            !world.registry.valid(
                world.activeCamera)
            ||
            !world.registry.all_of<
                TransformComponent,
                CameraComponent>(
                    world.activeCamera))
        {
            throw std::runtime_error(
                "RenderSystem requires an active camera entity.");
        }


        const auto& cameraTransform =
            world.registry.get<
                TransformComponent>(
                    world.activeCamera);


        const auto& cameraComponent =
            world.registry.get<
                CameraComponent>(
                    world.activeCamera);


        // =========================================================
        // CAMERA
        // =========================================================
        //
        // Render space is camera-relative.
        //
        // The game continues to work in physical metres and
        // double-precision global coordinates.
        //
        // The camera itself is always the float-space origin.
        //

        RenderCamera camera;


        camera.position =
            glm::vec3(
                0.0f);


        camera.forward =
            glm::normalize(
                cameraTransform.rotation
                *
                glm::vec3(
                    0.0f,
                    0.0f,
                    -1.0f));


        camera.up =
            glm::normalize(
                cameraTransform.rotation
                *
                glm::vec3(
                    0.0f,
                    1.0f,
                    0.0f));


        camera.verticalFovDegrees =
            cameraComponent.verticalFovDegrees;


        camera.nearPlane =
            std::max(
                0.00001f,
                cameraComponent.nearClipMeters
                *
                SpaceScale::RenderUnitsPerLocalMeter);


        camera.farPlane =
            std::max(
                SpaceScale::MinimumRenderFarPlane,
                cameraComponent.farClipMeters
                *
                SpaceScale::RenderUnitsPerLocalMeter);


        const glm::dvec3 cameraGlobalMeters =
            world.localToGlobalMeters(
                cameraTransform.positionMeters);


        // =========================================================
        // LIGHTING
        // =========================================================

        DirectionalLight sun;


        if (
            world.primaryStar
                !=
                entt::null
            &&
            world.registry.valid(
                world.primaryStar)
            &&
            world.registry.all_of<
                PrimaryStarComponent>(
                    world.primaryStar))
        {
            const auto& star =
                world.registry.get<
                    PrimaryStarComponent>(
                        world.primaryStar);


            sun.direction =
                glm::normalize(
                    star.direction);


            sun.radiance =
                star.radiance;
        }


        EnvironmentLight environment;

        environment.diffuseMultiplier =
            glm::vec3(
                0.0f);

        environment.specularMultiplier =
            glm::vec3(
                0.0f);


        m_planets.clear();
        m_objects.clear();


        AtmosphereInstance atmosphereInstance;

        const AtmosphereInstance* activeAtmosphere =
            nullptr;


        // =========================================================
        // GLOBAL PLANETS -> RENDER REPRESENTATIONS
        // =========================================================
        //
        // Far away:
        //
        //     compact angular-size representation.
        //
        // Near body:
        //
        //     true camera-relative center and physical radius.
        //
        // Near surface:
        //
        //     additionally provide a small, stable local surface
        //     frame generated directly from double-precision
        //     coordinates.
        //
        // This final surface frame is what the local ocean renderer
        // consumes.
        //

        const auto planetView =
            world.registry.view<
                GlobalPositionComponent,
                PlanetComponent,
                PlanetVisualComponent>();


        for (
            const entt::entity entity :
            planetView)
        {
            const auto& globalPosition =
                planetView.get<
                    GlobalPositionComponent>(
                        entity);


            const auto& planet =
                planetView.get<
                    PlanetComponent>(
                        entity);


            const auto& visual =
                planetView.get<
                    PlanetVisualComponent>(
                        entity);


            const glm::dvec3 relativeMeters =
                globalPosition.positionMeters
                -
                cameraGlobalMeters;


            const double physicalDistanceMeters =
                glm::length(
                    relativeMeters);


            if (
                physicalDistanceMeters
                    <=
                    1.0
                ||
                planet.properties.physical.radiusMeters
                    <=
                    0.0)
            {
                continue;
            }


            const glm::vec3 direction =
                glm::normalize(
                    glm::vec3(
                        relativeMeters));


            const double altitudeMeters =
                physicalDistanceMeters
                -
                planet.properties.physical.radiusMeters;


            const bool useNearBody =
                altitudeMeters
                <=
                SpaceScale::
                    NearBodyTransitionAltitudeMeters;


            glm::vec3 center(
                0.0f);


            float renderRadius =
                0.0f;


            // =====================================================
            // NEAR-BODY REPRESENTATION
            // =====================================================

            if (useNearBody)
            {
                center =
                    metersToRender(
                        relativeMeters);


                renderRadius =
                    static_cast<float>(
                        planet.properties.physical.radiusMeters
                        *
                        static_cast<double>(
                            SpaceScale::
                                RenderUnitsPerLocalMeter));


                const double atmosphereThicknessMeters =
                    world.registry.all_of<
                        AtmosphereComponent>(
                            entity)
                        ?
                        std::max(
                            0.0,
                            static_cast<double>(
                                world.registry.get<
                                    AtmosphereComponent>(
                                        entity)
                                    .parameters
                                    .topRadiusKm
                                -
                                world.registry.get<
                                    AtmosphereComponent>(
                                        entity)
                                    .parameters
                                    .bottomRadiusKm)
                            *
                            SpaceScale::
                                MetersPerKilometer)
                        :
                        0.0;


                const double requiredFarMeters =
                    physicalDistanceMeters
                    +
                    planet.properties.physical.radiusMeters
                    +
                    atmosphereThicknessMeters
                    +
                    10000.0;


                camera.farPlane =
                    std::max(
                        camera.farPlane,
                        static_cast<float>(
                            requiredFarMeters
                            *
                            static_cast<double>(
                                SpaceScale::
                                    RenderUnitsPerLocalMeter)));
            }

            // =====================================================
            // DISTANT-BODY REPRESENTATION
            // =====================================================

            else
            {
                const double radiusDistanceRatio =
                    planet.properties.physical.radiusMeters
                    /
                    physicalDistanceMeters;


                renderRadius =
                    SpaceScale::
                        DistantBodyCenterRenderUnits
                    *
                    static_cast<float>(
                        radiusDistanceRatio);


                center =
                    direction
                    *
                    SpaceScale::
                        DistantBodyCenterRenderUnits;
            }


            // =====================================================
            // PLANET RENDER OBJECT
            // =====================================================

            PlanetRenderObject renderPlanet;


            renderPlanet.mesh =
                &resources.planetSphere();


            renderPlanet.modelMatrix =
                glm::translate(
                    glm::mat4(
                        1.0f),
                    center)
                *
                glm::scale(
                    glm::mat4(
                        1.0f),
                    glm::vec3(
                        renderRadius));


            renderPlanet.material =
                visual.material;


            renderPlanet.hasOcean =
                planet.properties.surface.hasLiquidOcean;


            renderPlanet.radiusKm =
                static_cast<float>(
                    planet.properties.physical.radiusMeters
                    /
                    SpaceScale::
                        MetersPerKilometer);


            renderPlanet
                .oceanGravityMetersPerSecondSquared =
                    planet
                        .properties
                        .physical
                        .surfaceGravityMetersPerSecondSquared;


            // =====================================================
            // STABLE NEAR-SURFACE FRAME
            // =====================================================
            //
            // The old local ocean code reconstructed its anchor
            // using:
            //
            //     planetCenter - camera + planetRadius
            //
            // after everything had already become floats.
            //
            // At Earth scale that means trying to recover ~8 km
            // from values around ~6,360 km.
            //
            // That is exactly the sort of cancellation which can
            // turn tiny precision changes into visible wave
            // twitching.
            //
            // Instead, calculate the surface point directly here,
            // while everything is still double precision.
            //
            // For now the local surface frame activates below
            // 24 km.
            //

            constexpr double surfaceFrameAltitudeMeters =
                24000.0;


            if (
                altitudeMeters
                    >=
                    0.0
                &&
                altitudeMeters
                    <=
                    surfaceFrameAltitudeMeters)
            {
                const glm::dvec3 cameraFromPlanetMeters =
                    cameraGlobalMeters
                    -
                    globalPosition.positionMeters;


                const double cameraFromPlanetLengthMeters =
                    glm::length(
                        cameraFromPlanetMeters);


                if (
                    cameraFromPlanetLengthMeters
                        >
                        1.0)
                {
                    const glm::dvec3 surfaceUp =
                        cameraFromPlanetMeters
                        /
                        cameraFromPlanetLengthMeters;


                    // The sea-level point directly beneath the
                    // camera is simply altitude metres toward the
                    // planet.
                    //
                    // This remains a small number even though the
                    // planet itself is enormous.
                    const glm::dvec3 anchorRelativeMeters =
                        -surfaceUp
                        *
                        altitudeMeters;


                    renderPlanet.surfaceFrameValid =
                        true;


                    renderPlanet.surfaceAltitudeKm =
                        static_cast<float>(
                            altitudeMeters
                            /
                            SpaceScale::
                                MetersPerKilometer);


                    renderPlanet.surfaceAnchorNormalWorld =
                        glm::vec3(
                            surfaceUp);


                    renderPlanet.surfaceAnchorRelativeWorld =
                        metersToRender(
                            anchorRelativeMeters);
                }
            }


            m_planets.push_back(
                renderPlanet);


            // =====================================================
            // ATMOSPHERE
            // =====================================================

            if (
                activeAtmosphere
                    ==
                    nullptr
                &&
                world.registry.all_of<
                    AtmosphereComponent>(
                        entity))
            {
                const auto& atmosphere =
                    world.registry.get<
                        AtmosphereComponent>(
                            entity);


                if (atmosphere.enabled)
                {
                    atmosphereInstance.parameters =
                        &atmosphere.parameters;


                    atmosphereInstance.luts =
                        &resources.atmosphereLuts();


                    atmosphereInstance.planetCenterWorld =
                        center;


                    atmosphereInstance.planetRadiusWorld =
                        renderRadius;


                    activeAtmosphere =
                        &atmosphereInstance;
                }
            }
        }


        // =========================================================
        // LOCAL GAMEPLAY OBJECTS
        // =========================================================

        const auto renderableView =
            world.registry.view<
                TransformComponent,
                RenderableComponent>();


        for (
            const entt::entity entity :
            renderableView)
        {
            const auto& transform =
                renderableView.get<
                    TransformComponent>(
                        entity);


            const auto& renderable =
                renderableView.get<
                    RenderableComponent>(
                        entity);


            if (
                renderable.mesh
                    !=
                    RenderMeshKind::
                        PlaceholderShip)
            {
                continue;
            }


            const glm::mat4 root =
                cameraRelativeWorldMatrix(
                    transform,
                    cameraTransform.positionMeters);


            const PbrMaterial hull =
                materialFrom(
                    renderable);


            PbrMaterial dark =
                hull;


            dark.baseColor *=
                0.42f;


            dark.roughness =
                0.42f;


            PbrMaterial engine;


            engine.baseColor =
                glm::vec3(
                    0.04f,
                    0.08f,
                    0.11f);


            engine.metallic =
                0.25f;


            engine.roughness =
                0.25f;


            engine.emissiveColor =
                glm::vec3(
                    0.18f,
                    0.62f,
                    1.0f);


            engine.emissiveStrength =
                12.0f;


            const GpuMesh& box =
                resources.placeholderBox();


            // ~28 metre temporary ship.
            //
            // All dimensions here are physical metres.
            //
            // Forward is local -Z and the engine is on +Z.

            pushBoxMeters(
                m_objects,
                box,
                root,
                {
                    0.0f,
                    0.0f,
                    0.0f
                },
                {
                    5.5f,
                    3.2f,
                    20.0f
                },
                hull);


            pushBoxMeters(
                m_objects,
                box,
                root,
                {
                    0.0f,
                    0.25f,
                    -12.5f
                },
                {
                    4.0f,
                    2.6f,
                    7.0f
                },
                hull);


            pushBoxMeters(
                m_objects,
                box,
                root,
                {
                    0.0f,
                    -0.20f,
                    1.0f
                },
                {
                    17.0f,
                    0.7f,
                    7.5f
                },
                dark);


            pushBoxMeters(
                m_objects,
                box,
                root,
                {
                    0.0f,
                    0.0f,
                    11.0f
                },
                {
                    4.2f,
                    2.5f,
                    2.0f
                },
                engine);
        }


        // =========================================================
        // TEMPORARY SCALE REFERENCES
        // =========================================================

        const auto referenceView =
            world.registry.view<
                TransformComponent,
                ScaleReferenceComponent>();


        for (
            const entt::entity entity :
            referenceView)
        {
            const auto& transform =
                referenceView.get<
                    TransformComponent>(
                        entity);


            const auto& reference =
                referenceView.get<
                    ScaleReferenceComponent>(
                        entity);


            const glm::mat4 root =
                cameraRelativeWorldMatrix(
                    transform,
                    cameraTransform.positionMeters);


            PbrMaterial material;


            material.baseColor =
                reference.baseColor;


            material.metallic =
                0.12f;


            material.roughness =
                0.52f;


            material.emissiveColor =
                reference.baseColor;


            material.emissiveStrength =
                reference.emissiveStrength;


            pushBoxMeters(
                m_objects,
                resources.placeholderBox(),
                root,
                glm::vec3(
                    0.0f),
                reference.sizeMeters,
                material);
        }


        // =========================================================
        // DRAW
        // =========================================================

        renderer.render(
            width,
            height,
            camera,
            sun,
            environment,
            resources.environmentMap(),
            resources.environmentIbl(),
            m_planets,
            m_objects,
            activeAtmosphere);
    }
}