#include "systems/LightingSystem.h"

#include "world/GameWorld.h"

#include <cmath>

namespace SpaceSim
{
    namespace
    {
        float SrgbToLinear(float channel)
        {
            if (channel <= 0.04045f)
            {
                return channel / 12.92f;
            }

            return std::pow((channel + 0.055f) / 1.055f, 2.4f);
        }

        Vector3 ColorToLinearRgb(Color color)
        {
            return Vector3{
                SrgbToLinear(static_cast<float>(color.r) / 255.0f),
                SrgbToLinear(static_cast<float>(color.g) / 255.0f),
                SrgbToLinear(static_cast<float>(color.b) / 255.0f)
            };
        }
    }

    void LightingSystem::update(GameWorld& world)
    {
        SceneLighting& lighting = world.lighting;

        lighting.hasPrimaryStar = false;
        lighting.primaryStarIndex = -1;

        for (int i = 0;
             i < static_cast<int>(world.starSystem.objects.size());
             ++i)
        {
            const GlobalObject& object = world.starSystem.objects[i];

            if (object.type != GlobalObjectType::Sun)
            {
                continue;
            }

            lighting.hasPrimaryStar = true;
            lighting.primaryStarIndex = i;
            lighting.starGlobalPosition = object.position;
            lighting.starColor = ColorToLinearRgb(object.color);

            // The current star-system data has no physical luminosity yet, so
            // intensity intentionally remains normalized for this milestone.
            lighting.starIntensity = 1.0f;
            break;
        }
    }
}
