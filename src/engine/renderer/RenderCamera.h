#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct RenderCamera
    {
        glm::vec3 position
        {
            0.0f,
            0.0f,
            5.0f
        };

        glm::vec3 forward
        {
            0.0f,
            0.0f,
            -1.0f
        };

        glm::vec3 up
        {
            0.0f,
            1.0f,
            0.0f
        };

        float verticalFovDegrees = 60.0f;

        float nearPlane = 0.1f;
        float farPlane = 10000.0f;

        glm::mat4 viewMatrix() const;
        glm::mat4 projectionMatrix(
            float aspectRatio) const;
    };
}