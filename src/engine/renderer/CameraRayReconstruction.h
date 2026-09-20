#pragma once

#include "renderer/RenderCamera.h"

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <cmath>


namespace SpaceSim
{
    // Builds a matrix specifically for fullscreen camera-ray reconstruction.
    //
    // It is intentionally NOT the inverse of projection * view.
    //
    // Existing fullscreen shaders currently do this:
    //
    //     world = matrix * vec4(ndc, 1, 1);
    //     ray   = normalize(world.xyz - cameraPosition);
    //
    // This matrix makes that operation produce the same analytic ray as:
    //
    //     normalize(
    //         forward
    //       + right * ndc.x * aspect * tan(fov / 2)
    //       + up    * ndc.y * tan(fov / 2));
    //
    // Avoiding inverse(viewProjection) is important for RendererLab because
    // its near/far range is extremely wide (0.001 -> 1000).
    inline glm::mat4 makeCameraRayReconstructionMatrix(
        const RenderCamera& camera,
        float aspectRatio)
    {
        const glm::vec3 forward =
            glm::normalize(
                camera.forward);


        const glm::vec3 right =
            glm::normalize(
                glm::cross(
                    forward,
                    camera.up));


        const glm::vec3 correctedUp =
            glm::normalize(
                glm::cross(
                    right,
                    forward));


        const float tanHalfFov =
            std::tan(
                glm::radians(
                    camera.verticalFovDegrees)
                *
                0.5f);


        glm::mat4 matrix(
            0.0f);


        // GLM matrices are column-major.
        matrix[0] =
            glm::vec4(
                right *
                aspectRatio *
                tanHalfFov,
                0.0f);


        matrix[1] =
            glm::vec4(
                correctedUp *
                tanHalfFov,
                0.0f);


        matrix[2] =
            glm::vec4(
                forward,
                0.0f);


        matrix[3] =
            glm::vec4(
                camera.position,
                1.0f);


        return
            matrix;
    }
}