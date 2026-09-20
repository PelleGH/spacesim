#include "renderer/RenderCamera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace SpaceSim
{
    glm::mat4 RenderCamera::viewMatrix() const
    {
        return glm::lookAt(
            position,
            position + forward,
            up);
    }

    glm::mat4 RenderCamera::projectionMatrix(
        float aspectRatio) const
    {
        return glm::perspectiveRH_ZO(
            glm::radians(verticalFovDegrees),
            aspectRatio,
            farPlane,
            nearPlane);
    }
}