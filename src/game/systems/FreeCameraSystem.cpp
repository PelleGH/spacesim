#include "game/systems/FreeCameraSystem.h"

#include "game/ecs/components/CameraComponent.h"
#include "game/ecs/components/LightingComponents.h"
#include "game/ecs/components/PlanetComponents.h"
#include "game/ecs/components/TransformComponent.h"
#include "input/SdlInput.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

#include <cmath>
#include <iostream>

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

        glm::vec3 rotateAroundAxis(const glm::vec3& direction, float radians, const glm::vec3& axis)
        {
            return glm::normalize(glm::angleAxis(radians, glm::normalize(axis)) * direction);
        }
    }

    void FreeCameraSystem::applyPreset(GameWorld& world, int preset)
    {
        if (world.activeCamera == entt::null || world.primaryPlanet == entt::null)
        {
            return;
        }

        auto& cameraTransform = world.registry.get<TransformComponent>(world.activeCamera);
        const auto& planetTransform = world.registry.get<TransformComponent>(world.primaryPlanet);
        const auto& planet = world.registry.get<PlanetComponent>(world.primaryPlanet);
        const auto& atmosphere = world.registry.get<AtmosphereComponent>(world.primaryPlanet);

        const float worldUnitsPerKm = planet.radiusWorld / atmosphere.parameters.bottomRadiusKm;
        const glm::vec3 center = glm::vec3(planetTransform.position);
        glm::vec3 position;
        glm::vec3 forward;
        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        if (preset == 0)
        {
            const float altitudeKm = 0.05f;
            position = center + glm::vec3(0.0f, planet.radiusWorld + altitudeKm * worldUnitsPerKm, 0.0f);
            forward = glm::normalize(glm::vec3(0.0f, -0.06f, -1.0f));
            std::cout << "Camera: SURFACE (~50 m)\n";
        }
        else if (preset == 1)
        {
            const float altitudeKm = 40.0f;
            position = center + glm::vec3(0.0f, planet.radiusWorld + altitudeKm * worldUnitsPerKm, 0.0f);
            forward = glm::normalize(glm::vec3(0.0f, -0.20f, -1.0f));
            std::cout << "Camera: UPPER ATMOSPHERE (~40 km)\n";
        }
        else
        {
            position = center + glm::vec3(0.0f, 35.0f, 140.0f);
            forward = glm::normalize(center - position);
            std::cout << "Camera: ORBIT\n";
        }

        cameraTransform.position = glm::dvec3(position);
        cameraTransform.rotation = orientationFromForwardUp(forward, up);
    }

    void FreeCameraSystem::update(GameWorld& world, const SdlInput& input, float dt)
    {
        (void)dt;

        if (world.activeCamera == entt::null ||
            !world.registry.all_of<TransformComponent, CameraComponent>(world.activeCamera))
        {
            return;
        }

        if (input.keyPressed(SDL_SCANCODE_1))
        {
            applyPreset(world, 0);
        }
        if (input.keyPressed(SDL_SCANCODE_2))
        {
            applyPreset(world, 1);
        }
        if (input.keyPressed(SDL_SCANCODE_3))
        {
            applyPreset(world, 2);
        }

        auto& transform = world.registry.get<TransformComponent>(world.activeCamera);
        const auto& camera = world.registry.get<CameraComponent>(world.activeCamera);

        if (input.keyPressed(SDL_SCANCODE_4) &&
            world.primaryStar != entt::null &&
            world.registry.all_of<PrimaryStarComponent>(world.primaryStar))
        {
            const auto& star = world.registry.get<PrimaryStarComponent>(world.primaryStar);
            const glm::vec3 up = transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
            transform.rotation = orientationFromForwardUp(glm::normalize(star.direction), up);
            std::cout << "Camera: looking directly at primary star\n";
        }

        if (!input.mouseDown(MouseButton::Right))
        {
            return;
        }

        glm::vec3 forward = glm::normalize(transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
        glm::vec3 up = glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));

        const glm::vec2 mouseDelta = input.mouseDelta();
        forward = rotateAroundAxis(forward, -mouseDelta.x * camera.lookSensitivity, up);

        const glm::vec3 right = glm::normalize(glm::cross(forward, up));
        const glm::vec3 pitchedForward = rotateAroundAxis(
            forward,
            -mouseDelta.y * camera.lookSensitivity,
            right);

        if (std::abs(glm::dot(pitchedForward, up)) < 0.995f)
        {
            forward = pitchedForward;
        }

        transform.rotation = orientationFromForwardUp(forward, up);
    }
}
