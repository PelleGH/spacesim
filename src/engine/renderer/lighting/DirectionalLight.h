#pragma once

#include <glm/vec3.hpp>


namespace SpaceSim
{
    struct DirectionalLight
    {
        // Direction FROM the surface / camera TOWARD the light.
        //
        // For the primary star this is updated from the actual
        // star position once the renderer is connected to the game.
        glm::vec3 direction
        {
            0.0f,
            1.0f,
            0.0f
        };


        // Incident linear HDR light used by PBR and atmosphere
        // calculations.
        //
        // This is deliberately separate from the visible stellar
        // disk radiance below.
        glm::vec3 radiance
        {
            1.0f
        };


        // =========================================================
        // VISIBLE DISTANT SOURCE
        // =========================================================
        //
        // A DirectionalLight in SpaceSim represents a very distant
        // stellar source, so it can optionally have a visible disk.
        //
        // Keeping these as numeric properties instead of a
        // "Sun / RedDwarf / BlueStar" enum means procedurally
        // generated stars can simply supply their actual values.

        bool sourceVisible =
            true;


        // Linear HDR radiance of the visible stellar surface.
        //
        // Later the procedural star generator can derive this from
        // stellar temperature/luminosity rather than selecting from
        // hard-coded star classes.
        glm::vec3 sourceDiskRadiance
        {
            60.0f,
            56.0f,
            48.0f
        };


        // Apparent angular RADIUS in radians.
        //
        // 0.00465 rad ~= 0.266 degrees.
        //
        // That is approximately the apparent angular radius of the
        // Sun from Earth.
        //
        // Later:
        //
        // angularRadius ~= asin(starRadius / distanceToStar)
        float sourceAngularRadiusRadians =
            0.00465f;


        // 0 = uniformly bright disk
        // 1 = very strong center-to-edge darkening
        //
        // Different generated stars can use different values later.
        float sourceLimbDarkening =
            0.55f;
    };
}