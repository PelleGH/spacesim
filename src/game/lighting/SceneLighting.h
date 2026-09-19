#pragma once

#include "world/StarSystem.h"

#include <raylib.h>
#include <raymath.h>

namespace SpaceSim
{
    struct SceneLighting
    {
        bool hasPrimaryStar = false;
        int primaryStarIndex = -1;

        DVec3 starGlobalPosition{};

        // Linear RGB for the first lighting milestone. A later HDR pass can
        // promote this to physical radiance / color-temperature-derived values.
        Vector3 starColor{ 1.0f, 1.0f, 1.0f };
        float starIntensity = 1.0f;

        // Temporary indirect illumination until sky/atmospheric irradiance is
        // calculated by the renderer.
        Vector3 ambientColor{ 0.025f, 0.03f, 0.04f };
        float ambientIntensity = 1.0f;

        // Reserved for the future HDR/tone-mapping path.
        float exposure = 1.0f;
    };

    inline Vector3 GetLightDirectionAt(
        const SceneLighting& lighting,
        DVec3 globalPosition)
    {
        if (!lighting.hasPrimaryStar)
        {
            return Vector3{ 0.0f, 1.0f, 0.0f };
        }

        const DVec3 direction = Normalize(
            lighting.starGlobalPosition - globalPosition);

        return Vector3Normalize(Vector3{
            static_cast<float>(direction.x),
            static_cast<float>(direction.y),
            static_cast<float>(direction.z)
        });
    }
}
