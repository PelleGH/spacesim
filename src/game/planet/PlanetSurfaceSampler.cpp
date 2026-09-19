#include "planet/PlanetSurfaceSampler.h"

#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace SpaceSim
{
    namespace
    {
        constexpr float MinimumSurfaceOffset = 0.8f;
        constexpr float MaximumTerrainHeight = 14.0f;

        float fract(float value)
        {
            return value - std::floor(value);
        }

        float smoothStep(float edge0, float edge1, float value)
        {
            const float t = Clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        float hash(Vector3 p)
        {
            // Integer lattice hashing matches GLSL exactly; floating point
            // fract-of-products can produce different continents on the GPU.
            unsigned int h = static_cast<unsigned int>(static_cast<int>(p.x)) * 374761393u;
            h ^= static_cast<unsigned int>(static_cast<int>(p.y)) * 668265263u;
            h ^= static_cast<unsigned int>(static_cast<int>(p.z)) * 2246822519u;
            h = (h ^ (h >> 13u)) * 1274126177u;
            h ^= h >> 16u;
            return static_cast<float>(h & 0x00ffffffu) / 16777215.0f;
        }

        float noise(Vector3 p)
        {
            const Vector3 i{
                std::floor(p.x),
                std::floor(p.y),
                std::floor(p.z)
            };

            Vector3 f{ fract(p.x), fract(p.y), fract(p.z) };
            f = Vector3{ f.x * f.x * (3.0f - 2.0f * f.x),
                         f.y * f.y * (3.0f - 2.0f * f.y),
                         f.z * f.z * (3.0f - 2.0f * f.z) };

            const auto corner = [&](float x, float y, float z)
            {
                return hash(Vector3Add(i, Vector3{ x, y, z }));
            };

            const float n000 = corner(0, 0, 0);
            const float n100 = corner(1, 0, 0);
            const float n010 = corner(0, 1, 0);
            const float n110 = corner(1, 1, 0);
            const float n001 = corner(0, 0, 1);
            const float n101 = corner(1, 0, 1);
            const float n011 = corner(0, 1, 1);
            const float n111 = corner(1, 1, 1);

            const float nx00 = Lerp(n000, n100, f.x);
            const float nx10 = Lerp(n010, n110, f.x);
            const float nx01 = Lerp(n001, n101, f.x);
            const float nx11 = Lerp(n011, n111, f.x);

            return Lerp(Lerp(nx00, nx10, f.y), Lerp(nx01, nx11, f.y), f.z);
        }

        float fbm(Vector3 p, int octaves = 6, float multiplier = 2.03f)
        {
            float total = 0.0f;
            float amp = 0.5f;

            for (int i = 0; i < octaves; ++i)
            {
                total += noise(p) * amp;
                p = Vector3Scale(p, multiplier);
                amp *= 0.5f;
            }

            return total;
        }

        float shaderSeedFromObjectSeed(int seed)
        {
            int safe = seed % 10000;
            if (safe < 0) safe += 10000;
            return static_cast<float>(safe) * 0.017f;
        }

        bool classHasOcean(PlanetClass planetClass)
        {
            return planetClass == PlanetClass::OceanWorld;
        }

        float terrainDetail(Vector3 direction, int seed)
        {
            unsigned int baseHash = static_cast<unsigned int>(seed);

            const auto hashCoordinate = [&](int x, int y, int z)
            {
                unsigned int h = baseHash;
                h ^= static_cast<unsigned int>(x) * 374761393u;
                h ^= static_cast<unsigned int>(y) * 668265263u;
                h ^= static_cast<unsigned int>(z) * 2246822519u;
                h = (h ^ (h >> 13u)) * 1274126177u;
                h ^= h >> 16u;

                return static_cast<float>(h & 0x00ffffffu) /
                    static_cast<float>(0x00ffffffu);
            };

            const auto valueNoise = [&](Vector3 position)
            {
                const int x = static_cast<int>(std::floor(position.x));
                const int y = static_cast<int>(std::floor(position.y));
                const int z = static_cast<int>(std::floor(position.z));

                const float fx = smoothStep(0.0f, 1.0f, fract(position.x));
                const float fy = smoothStep(0.0f, 1.0f, fract(position.y));
                const float fz = smoothStep(0.0f, 1.0f, fract(position.z));

                const float x00 = Lerp(hashCoordinate(x, y, z), hashCoordinate(x + 1, y, z), fx);
                const float x10 = Lerp(hashCoordinate(x, y + 1, z), hashCoordinate(x + 1, y + 1, z), fx);
                const float x01 = Lerp(hashCoordinate(x, y, z + 1), hashCoordinate(x + 1, y, z + 1), fx);
                const float x11 = Lerp(hashCoordinate(x, y + 1, z + 1), hashCoordinate(x + 1, y + 1, z + 1), fx);

                return Lerp(Lerp(x00, x10, fy), Lerp(x01, x11, fy), fz);
            };

            float result = 0.0f;
            float amplitude = 1.0f;
            float totalAmplitude = 0.0f;
            float frequency = 24.0f;

            for (int octave = 0; octave < 6; ++octave)
            {
                result += valueNoise(Vector3Scale(direction, frequency)) * amplitude;
                totalAmplitude += amplitude;
                frequency *= 2.0f;
                amplitude *= 0.5f;
            }

            return totalAmplitude > 0.0f ? result / totalAmplitude : 0.0f;
        }
    }

    int PlanetSurfaceSampler::seedFromId(const std::string& planetId)
    {
        unsigned int hashValue = 2166136261u;

        for (char character : planetId)
        {
            hashValue ^= static_cast<unsigned int>(character);
            hashValue *= 16777619u;
        }

        return static_cast<int>(hashValue & 0x7fffffffu);
    }

    PlanetSurfaceSample PlanetSurfaceSampler::sample(
        Vector3 radialDirection,
        int seed,
        PlanetClass planetClass)
    {
        radialDirection = Vector3Normalize(radialDirection);

        PlanetSurfaceSample sample{};

        const float shaderSeed = shaderSeedFromObjectSeed(seed);
        const float continents = fbm(
            Vector3Add(Vector3Scale(radialDirection, 2.15f), Vector3{ shaderSeed, shaderSeed, shaderSeed })) +
            (noise(Vector3Add(Vector3Scale(radialDirection, 38.0f),
                Vector3{shaderSeed * 0.61f, shaderSeed * 0.61f, shaderSeed * 0.61f})) - 0.5f) * 0.012f;

        sample.continentValue = continents;

        const float landMask = smoothStep(0.535f, 0.540f, continents);
        sample.isOcean = classHasOcean(planetClass) && landMask < 0.5f;

        if (sample.isOcean)
        {
            const float coastBand = smoothStep(0.485f, 0.545f, continents) * (1.0f - landMask);
            const float shallow = Clamp(coastBand, 0.0f, 1.0f);

            sample.terrainHeight = 0.0f;
            sample.color = Color{
                static_cast<unsigned char>(Lerp(5.0f, 20.0f, shallow)),
                static_cast<unsigned char>(Lerp(45.0f, 95.0f, shallow)),
                static_cast<unsigned char>(Lerp(80.0f, 120.0f, shallow)),
                255
            };
            return sample;
        }

        if (classHasOcean(planetClass))
        {
            // Broad uplands and connected ridges, rather than uniform small hills.
            const float uplift = fbm(Vector3Add(Vector3Scale(radialDirection, 5.0f),
                Vector3{shaderSeed * 1.73f, shaderSeed * 1.73f, shaderSeed * 1.73f}));
            const float ridgeNoise = noise(Vector3Add(Vector3Scale(radialDirection, 14.0f),
                Vector3{shaderSeed * 2.19f, shaderSeed * 2.19f, shaderSeed * 2.19f}));
            const float ridge = 1.0f - std::fabs(ridgeNoise * 2.0f - 1.0f);
            const float mountainBelt = smoothStep(0.38f, 0.62f, uplift);
            const float inland = smoothStep(0.540f, 0.595f, continents);
            sample.terrainHeight = inland * (1.5f + 8.0f * uplift +
                32.0f * mountainBelt * ridge * ridge * ridge);
            sample.color = ColorLerp(Color{45, 83, 39, 255}, Color{155, 144, 120, 255},
                smoothStep(8.0f, 28.0f, sample.terrainHeight));
            return sample;
        }

        const float detail = terrainDetail(radialDirection, seed);
        const float shapedDetail = std::pow(Clamp(detail, 0.0f, 1.0f), 1.65f);

        float reliefMask = 1.0f;
        if (classHasOcean(planetClass))
        {
            reliefMask = smoothStep(0.540f, 0.660f, continents);
        }

        sample.terrainHeight = classHasOcean(planetClass)
            ? (MinimumSurfaceOffset + shapedDetail * MaximumTerrainHeight) * reliefMask
            : MinimumSurfaceOffset + shapedDetail * MaximumTerrainHeight;

        const float normalizedHeight = Clamp(sample.terrainHeight / MaximumTerrainHeight, 0.0f, 1.0f);

        switch (planetClass)
        {
        case PlanetClass::Rocky:
            sample.color = ColorLerp(Color{ 98, 83, 69, 255 }, Color{ 184, 168, 143, 255 },
                smoothStep(0.1f, 0.7f, normalizedHeight));
            break;
        case PlanetClass::DesertWorld:
            sample.color = ColorLerp(Color{ 156, 112, 58, 255 }, Color{ 196, 160, 92, 255 }, normalizedHeight);
            break;
        case PlanetClass::IceWorld:
        case PlanetClass::BarrenMoon:
            sample.color = ColorLerp(Color{ 132, 145, 150, 255 }, Color{ 195, 205, 210, 255 }, normalizedHeight);
            break;
        case PlanetClass::CarbonWorld:
            sample.color = normalizedHeight < 0.55f ? Color{ 35, 31, 31, 255 } : Color{ 110, 55, 42, 255 };
            break;
        case PlanetClass::LavaWorld:
            sample.color = normalizedHeight < 0.65f ? Color{ 55, 38, 30, 255 } : Color{ 210, 70, 25, 255 };
            break;
        default:
            if (normalizedHeight < 0.25f)
            {
                sample.color = Color{ 65, 112, 70, 255 };
            }
            else if (normalizedHeight < 0.55f)
            {
                sample.color = Color{ 92, 132, 76, 255 };
            }
            else if (normalizedHeight < 0.78f)
            {
                sample.color = Color{ 112, 105, 80, 255 };
            }
            else
            {
                sample.color = Color{ 158, 154, 142, 255 };
            }
            break;
        }

        return sample;
    }
}
