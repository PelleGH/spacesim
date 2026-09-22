#pragma once

#include "renderer/planet/PlanetMaterial.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>

namespace SpaceSim
{
    // Canonical procedural surface sample for the modern renderer/game path.
    //
    // Sea level is radiusScale == 1.0. Positive signedElevationFraction is
    // terrain above sea level. Negative signedElevationFraction is underwater
    // terrain. Multiply the fraction by the physical planet radius to convert
    // it to metres.
    struct PlanetSurfaceSample
    {
        float heightField = 0.0f;
        float signedElevationFraction = 0.0f;
        float radiusScale = 1.0f;
        float waterDepthFraction = 0.0f;
        bool isLand = true;
    };

    namespace PlanetSurfaceDetail
    {
        inline float fract(float value)
        {
            return value - std::floor(value);
        }

        inline glm::vec3 fract(const glm::vec3& value)
        {
            return
            {
                fract(value.x),
                fract(value.y),
                fract(value.z)
            };
        }

        inline float hash31(glm::vec3 p)
        {
            p = fract(p * 0.1031f);

            p += glm::dot(
                p,
                glm::vec3(p.y, p.z, p.x) + glm::vec3(33.33f));

            return fract((p.x + p.y) * p.z);
        }

        inline float valueNoise(glm::vec3 p)
        {
            const glm::vec3 cell
            {
                std::floor(p.x),
                std::floor(p.y),
                std::floor(p.z)
            };

            const glm::vec3 f = fract(p);
            const glm::vec3 u =
                f * f * (glm::vec3(3.0f) - 2.0f * f);

            const float n000 = hash31(cell + glm::vec3(0.0f, 0.0f, 0.0f));
            const float n100 = hash31(cell + glm::vec3(1.0f, 0.0f, 0.0f));
            const float n010 = hash31(cell + glm::vec3(0.0f, 1.0f, 0.0f));
            const float n110 = hash31(cell + glm::vec3(1.0f, 1.0f, 0.0f));
            const float n001 = hash31(cell + glm::vec3(0.0f, 0.0f, 1.0f));
            const float n101 = hash31(cell + glm::vec3(1.0f, 0.0f, 1.0f));
            const float n011 = hash31(cell + glm::vec3(0.0f, 1.0f, 1.0f));
            const float n111 = hash31(cell + glm::vec3(1.0f, 1.0f, 1.0f));

            const float nx00 = glm::mix(n000, n100, u.x);
            const float nx10 = glm::mix(n010, n110, u.x);
            const float nx01 = glm::mix(n001, n101, u.x);
            const float nx11 = glm::mix(n011, n111, u.x);

            const float nxy0 = glm::mix(nx00, nx10, u.y);
            const float nxy1 = glm::mix(nx01, nx11, u.y);

            return glm::mix(nxy0, nxy1, u.z);
        }

        inline float fbm(glm::vec3 p)
        {
            float result = 0.0f;
            float amplitude = 0.5f;
            float totalAmplitude = 0.0f;

            for (int octave = 0; octave < 5; ++octave)
            {
                result += valueNoise(p) * amplitude;
                totalAmplitude += amplitude;

                p =
                    p * 2.03f +
                    glm::vec3(17.1f, 31.7f, 11.3f);

                amplitude *= 0.5f;
            }

            return
                result /
                std::max(totalAmplitude, 0.0001f);
        }
    }

    // Same broad field currently used by the planet/ocean shaders. Keeping
    // this function separate makes it useful for future biome/climate queries
    // without making those systems know how FBM is assembled.
    [[nodiscard]] inline float planetSurfaceHeightField(
        const glm::vec3& direction,
        const PlanetMaterial& material)
    {
        const glm::vec3 sphereDirection =
            glm::normalize(direction);

        const glm::vec3 seedOffset
        {
            material.seed * 1.371f,
            material.seed * 2.113f,
            material.seed * 3.731f
        };

        const float continents =
            PlanetSurfaceDetail::fbm(
                sphereDirection * material.continentScale + seedOffset);

        const float detail =
            PlanetSurfaceDetail::fbm(
                sphereDirection * material.detailScale + seedOffset * 2.7f);

        return continents * 0.82f + detail * 0.18f;
    }

    [[nodiscard]] inline PlanetSurfaceSample samplePlanetSurface(
        const glm::vec3& direction,
        const PlanetMaterial& material)
    {
        PlanetSurfaceSample sample;

        const glm::vec3 sphereDirection =
            glm::normalize(direction);

        const glm::vec3 seedOffset
        {
            material.seed * 1.371f,
            material.seed * 2.113f,
            material.seed * 3.731f
        };

        const float continents =
            PlanetSurfaceDetail::fbm(
                sphereDirection * material.continentScale + seedOffset);

        const float detail =
            PlanetSurfaceDetail::fbm(
                sphereDirection * material.detailScale + seedOffset * 2.7f);

        sample.heightField =
            continents * 0.82f + detail * 0.18f;

        // Shoreline truth is now simply heightField == oceanLevel.
        // The elevation approaches zero continuously from either side instead
        // of flattening every underwater terrain vertex to radius 1.
        const float landAmount =
            glm::clamp(
                (sample.heightField - material.oceanLevel) /
                    std::max(1.0f - material.oceanLevel, 0.0001f),
                0.0f,
                1.0f);

        // The current FBM values occupy a fairly narrow range around 0.5.
        // A 0.15-wide basin band gives the Earth-like preset continental
        // shelves followed by kilometre-scale deep ocean basins.
        constexpr float OceanBasinFieldWidth = 0.15f;

        const float oceanAmount =
            glm::clamp(
                (material.oceanLevel - sample.heightField) /
                    OceanBasinFieldWidth,
                0.0f,
                1.0f);

        const float broadShape =
            std::pow(landAmount, 1.35f);

        const float ridge =
            1.0f - std::abs(detail * 2.0f - 1.0f);

        const float mountainShape =
            ridge * ridge * ridge *
            broadShape * broadShape;

        const float landElevationFraction =
            std::max(material.terrainReliefScale, 0.0f) *
            (
                0.00030f * broadShape +
                0.00115f * mountainShape
            );

        // This is intentionally still a simple procedural basin model rather
        // than geology. At Earth radius the default reaches roughly 8 km at
        // its deepest while remaining shallow close to shore.
        const float basinShape =
            std::pow(oceanAmount, 1.55f) *
            glm::mix(0.78f, 1.0f, detail);

        const float seabedDepthFraction =
            0.00125f * basinShape;

        sample.signedElevationFraction =
            landElevationFraction - seabedDepthFraction;

        sample.radiusScale =
            1.0f + sample.signedElevationFraction;

        sample.isLand =
            sample.heightField >= material.oceanLevel;

        sample.waterDepthFraction =
            sample.isLand
                ? 0.0f
                : seabedDepthFraction;

        return sample;
    }

    [[nodiscard]] inline glm::vec3 planetSurfacePosition(
        const glm::vec3& direction,
        const PlanetMaterial& material)
    {
        const glm::vec3 sphereDirection =
            glm::normalize(direction);

        return
            sphereDirection *
            samplePlanetSurface(
                sphereDirection,
                material)
                .radiusScale;
    }
}
