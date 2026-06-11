#include "world/PlanetTerrainGenerator.h"

#include <raymath.h>

#include <cmath>
#include <cstdint>

namespace SpaceSim
{
    namespace
    {
        static float Hash01(std::uint32_t value)
        {
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            value ^= value >> 16;

            return static_cast<float>(value) / static_cast<float>(0xffffffffu);
        }

        static float SmoothStep(float t)
        {
            t = Clamp(t, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        static float LerpFloat(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        static Color LerpColor(Color a, Color b, float t)
        {
            t = Clamp(t, 0.0f, 1.0f);

            return Color{
                static_cast<unsigned char>(LerpFloat(a.r, b.r, t)),
                static_cast<unsigned char>(LerpFloat(a.g, b.g, t)),
                static_cast<unsigned char>(LerpFloat(a.b, b.b, t)),
                static_cast<unsigned char>(LerpFloat(a.a, b.a, t))
            };
        }

        static Color MultiplyColor(Color color, float amount)
        {
            amount = Clamp(amount, 0.0f, 2.0f);

            return Color{
                static_cast<unsigned char>(Clamp(static_cast<float>(color.r) * amount, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(static_cast<float>(color.g) * amount, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(static_cast<float>(color.b) * amount, 0.0f, 255.0f)),
                color.a
            };
        }
        static float Saturate(float value)
        {
            return Clamp(value, 0.0f, 1.0f);
        }

        static Color AddColor(Color a, Color b)
        {
            return Color{
                static_cast<unsigned char>(Clamp(static_cast<float>(a.r) + static_cast<float>(b.r), 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(static_cast<float>(a.g) + static_cast<float>(b.g), 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(static_cast<float>(a.b) + static_cast<float>(b.b), 0.0f, 255.0f)),
                a.a
            };
        }

        static Color ApplyOrbitalLighting(
            Color color,
            PlanetClass planetClass,
            Vector3 direction,
            bool emissive = false
        )
        {
            // Temporary fake sun direction for orbital visuals.
            // Later this should come from the actual system star.
            Vector3 sunDirection = Vector3Normalize(Vector3{ -0.55f, 0.22f, -0.80f });

            float ndotl = Vector3DotProduct(Vector3Normalize(direction), sunDirection);

            // Soft day/night transition.
            float day = SmoothStep((ndotl + 0.18f) / 0.45f);

            float ambient = 0.16f;
            float sunlight = 0.92f;

            switch (planetClass)
            {
            case PlanetClass::GasGiant:
            case PlanetClass::IceGiant:
                ambient = 0.38f;
                sunlight = 0.72f;
                break;

            case PlanetClass::OceanWorld:
            case PlanetClass::IceWorld:
                ambient = 0.22f;
                sunlight = 0.90f;
                break;

            case PlanetClass::LavaWorld:
                ambient = 0.10f;
                sunlight = 0.80f;
                break;

            case PlanetClass::CarbonWorld:
                ambient = 0.18f;
                sunlight = 0.85f;
                break;

            default:
                break;
            }

            float lightAmount = ambient + day * sunlight;

            Color lit = MultiplyColor(color, lightAmount);

            // Lava should still glow on the dark side.
            if (emissive)
            {
                Color glow = MultiplyColor(color, 0.65f);
                lit = AddColor(lit, glow);
            }

            return lit;
        }
        static float ValueNoise3D(float x, float y, float z, std::uint32_t seed)
        {
            int ix = static_cast<int>(floorf(x));
            int iy = static_cast<int>(floorf(y));
            int iz = static_cast<int>(floorf(z));

            float fx = x - static_cast<float>(ix);
            float fy = y - static_cast<float>(iy);
            float fz = z - static_cast<float>(iz);

            float sx = SmoothStep(fx);
            float sy = SmoothStep(fy);
            float sz = SmoothStep(fz);

            auto sample = [seed](int px, int py, int pz)
            {
                std::uint32_t h = seed;
                h ^= static_cast<std::uint32_t>(px) * 374761393u;
                h ^= static_cast<std::uint32_t>(py) * 668265263u;
                h ^= static_cast<std::uint32_t>(pz) * 2147483647u;
                return Hash01(h);
            };

            float c000 = sample(ix, iy, iz);
            float c100 = sample(ix + 1, iy, iz);
            float c010 = sample(ix, iy + 1, iz);
            float c110 = sample(ix + 1, iy + 1, iz);

            float c001 = sample(ix, iy, iz + 1);
            float c101 = sample(ix + 1, iy, iz + 1);
            float c011 = sample(ix, iy + 1, iz + 1);
            float c111 = sample(ix + 1, iy + 1, iz + 1);

            float x00 = LerpFloat(c000, c100, sx);
            float x10 = LerpFloat(c010, c110, sx);
            float x01 = LerpFloat(c001, c101, sx);
            float x11 = LerpFloat(c011, c111, sx);

            float y0 = LerpFloat(x00, x10, sy);
            float y1 = LerpFloat(x01, x11, sy);

            return LerpFloat(y0, y1, sz);
        }

        static float FractalNoise3D(Vector3 direction, float frequency, std::uint32_t seed)
        {
            float total = 0.0f;
            float amplitude = 0.5f;
            float currentFrequency = frequency;

            for (int i = 0; i < 6; ++i)
            {
                total += ValueNoise3D(
                    direction.x * currentFrequency + 19.13f,
                    direction.y * currentFrequency + 31.71f,
                    direction.z * currentFrequency + 47.47f,
                    seed + static_cast<std::uint32_t>(i * 101)
                ) * amplitude;

                currentFrequency *= 2.0f;
                amplitude *= 0.5f;
            }

            return Clamp(total, 0.0f, 1.0f);
        }

        static float RidgedNoise3D(Vector3 direction, float frequency, std::uint32_t seed)
        {
            float n = FractalNoise3D(direction, frequency, seed);
            n = 1.0f - fabsf(n * 2.0f - 1.0f);
            return Clamp(n, 0.0f, 1.0f);
        }

        static float CalculateOceanHeight(
            const PlanetData& data,
            Vector3 direction
        )
        {
            float continents = FractalNoise3D(direction, 2.2f, data.seed + 100u);

            float landMask = SmoothStep((continents - 0.56f) / 0.09f);

            float broad = FractalNoise3D(direction, 4.5f, data.seed + 180u);
            float mountains = RidgedNoise3D(direction, 9.0f, data.seed + 200u);

            float landHeight = 0.0008f + broad * 0.0012f + mountains * 0.0015f;

            return landMask * landHeight;
        }

        static float CalculateRockyHeight(
            const PlanetData& data,
            Vector3 direction
        )
        {
            float broad = FractalNoise3D(direction, 2.5f, data.seed);
            float mountains = RidgedNoise3D(direction, 9.0f, data.seed + 500u);
            float detail = FractalNoise3D(direction, 24.0f, data.seed + 600u);

            return broad * 0.012f + mountains * 0.025f + detail * 0.004f;
        }

        static float CalculateBarrenMoonHeight(
            const PlanetData& data,
            Vector3 direction
        )
        {
            float broad = FractalNoise3D(direction, 3.0f, data.seed);
            float craterNoise = RidgedNoise3D(direction, 14.0f, data.seed + 700u);
            float detail = FractalNoise3D(direction, 32.0f, data.seed + 800u);

            return broad * 0.010f + craterNoise * 0.024f + detail * 0.006f;
        }

        static float CalculateLavaHeight(
            const PlanetData& data,
            Vector3 direction
        )
        {
            float crust = FractalNoise3D(direction, 4.0f, data.seed);
            float cracks = RidgedNoise3D(direction, 18.0f, data.seed + 900u);

            return crust * 0.012f + cracks * 0.014f;
        }

        static float CalculateGasHeight()
        {
            return 0.0f;
        }

        static float CalculateHeight(
            const PlanetData& data,
            Vector3 direction
        )
        {
            switch (data.planetClass)
            {
            case PlanetClass::OceanWorld:
                return CalculateOceanHeight(data, direction);

            case PlanetClass::BarrenMoon:
                return CalculateBarrenMoonHeight(data, direction);

            case PlanetClass::LavaWorld:
                return CalculateLavaHeight(data, direction);

            case PlanetClass::GasGiant:
            case PlanetClass::IceGiant:
                return CalculateGasHeight();

            case PlanetClass::Rocky:
            case PlanetClass::IceWorld:
            case PlanetClass::DesertWorld:
            case PlanetClass::CarbonWorld:
            default:
                return CalculateRockyHeight(data, direction);
            }
        }

        static Color CalculateSurfaceColor(
            const GlobalObject& object,
            Vector3 direction,
            float height
        )
        {
            PlanetVisual visual = GeneratePlanetVisual(object);

            if (!object.hasPlanetData)
            {
                return object.color;
            }

            const PlanetData& data = object.planetData;

            float broad = FractalNoise3D(direction, 3.0f, data.seed + 1000u);
            float detail = FractalNoise3D(direction, 18.0f, data.seed + 1100u);

            switch (data.planetClass)
            {
            case PlanetClass::OceanWorld:
            {
                Color surface{};

                if (height < 0.004f)
                {
                    surface = MultiplyColor(visual.baseColor, 0.82f + detail * 0.12f);
                }
                else
                {
                    Color land = LerpColor(visual.secondaryColor, Color{ 120, 95, 60, 255 }, broad * 0.35f);
                    surface = MultiplyColor(land, 0.85f + detail * 0.18f);
                }

                return ApplyOrbitalLighting(surface, data.planetClass, direction);
            }
            case PlanetClass::IceWorld:
            {
                Color ice = LerpColor(visual.baseColor, Color{ 230, 240, 245, 255 }, broad * 0.35f);
                Color surface = MultiplyColor(ice, 0.90f + detail * 0.12f);

                return ApplyOrbitalLighting(surface, data.planetClass, direction);
            }

            case PlanetClass::DesertWorld:
            {
                Color sand = LerpColor(visual.baseColor, visual.secondaryColor, broad * 0.55f);
                Color surface = MultiplyColor(sand, 0.88f + detail * 0.14f);

                return ApplyOrbitalLighting(surface, data.planetClass, direction);
            }

            case PlanetClass::CarbonWorld:
            {
                Color carbon = LerpColor(visual.baseColor, visual.secondaryColor, broad * 0.35f);
                Color surface = MultiplyColor(carbon, 0.95f + detail * 0.35f);

                return ApplyOrbitalLighting(surface, data.planetClass, direction);
            }

            case PlanetClass::LavaWorld:
            {
                float plates = FractalNoise3D(direction, 2.4f, data.seed + 1200u);
                float crackNoise = RidgedNoise3D(direction, 18.0f, data.seed + 1300u);
                float heatNoise = FractalNoise3D(direction, 5.0f, data.seed + 1400u);

                Color darkCrust = LerpColor(
                    Color{ 18, 15, 13, 255 },
                    Color{ 62, 42, 30, 255 },
                    plates
                );

                Color warmCrust = LerpColor(
                    darkCrust,
                    Color{ 110, 58, 26, 255 },
                    SmoothStep((heatNoise - 0.48f) / 0.30f) * 0.35f
                );

                Color hotLava = LerpColor(
                    Color{ 255, 70, 8, 255 },
                    Color{ 255, 210, 45, 255 },
                    heatNoise
                );

                // Higher threshold = fewer glowing cracks.
                // Narrower divisor = thinner transition.
                float crackMask = SmoothStep((crackNoise - 0.86f) / 0.08f);

                // Very rare larger molten pools.
                float poolMask = SmoothStep((heatNoise - 0.78f) / 0.12f) * 0.22f;

                float lavaMask = Clamp(crackMask + poolMask, 0.0f, 1.0f);

                Color surface = LerpColor(warmCrust, hotLava, lavaMask);

                bool emissive = lavaMask > 0.12f;
                return ApplyOrbitalLighting(surface, data.planetClass, direction, emissive);
            }
            case PlanetClass::GasGiant:
            case PlanetClass::IceGiant:
            {
                float latitude = direction.y * 0.5f + 0.5f;

                // Distort the bands so they stop looking like perfect painted rings.
                float turbulence = FractalNoise3D(direction, 5.0f, data.seed + 2100u);
                float fineTurbulence = FractalNoise3D(direction, 16.0f, data.seed + 2200u);

                float distortedLatitude = latitude + (turbulence - 0.5f) * 0.08f + (fineTurbulence - 0.5f) * 0.025f;
                float bands = sinf(distortedLatitude * 18.0f * PI + broad * 1.2f);

                float t = SmoothStep(0.5f + 0.5f * bands);

                Color bandColor = LerpColor(visual.baseColor, visual.secondaryColor, t);
                Color surface = MultiplyColor(bandColor, 0.92f + detail * 0.08f);

                return ApplyOrbitalLighting(surface, data.planetClass, direction);
            }

            case PlanetClass::BarrenMoon:
            case PlanetClass::Rocky:
            default:
            {
                Color rock = LerpColor(visual.baseColor, visual.secondaryColor, broad * 0.65f);
                Color surface = MultiplyColor(rock, 0.78f + detail * 0.25f);

                return ApplyOrbitalLighting(surface, data.planetClass, direction);
            }
            }
        }
    }

    PlanetTerrainSample SamplePlanetTerrain(
        const GlobalObject& object,
        Vector3 unitDirection
    )
    {
        unitDirection = Vector3Normalize(unitDirection);

        PlanetTerrainSample sample{};

        if (!object.hasPlanetData)
        {
            sample.height = 0.0f;
            sample.color = object.color;
            return sample;
        }

        sample.height = CalculateHeight(object.planetData, unitDirection);
        sample.color = CalculateSurfaceColor(object, unitDirection, sample.height);

        return sample;
    }

    float SamplePlanetHeight(
        const GlobalObject& object,
        Vector3 unitDirection
    )
    {
        return SamplePlanetTerrain(object, unitDirection).height;
    }

    Color SamplePlanetSurfaceColor(
        const GlobalObject& object,
        Vector3 unitDirection
    )
    {
        return SamplePlanetTerrain(object, unitDirection).color;
    }

    Vector3 EstimatePlanetNormal(
        const GlobalObject& object,
        Vector3 unitDirection
    )
    {
        unitDirection = Vector3Normalize(unitDirection);

        Vector3 tangent = Vector3CrossProduct(unitDirection, Vector3{ 0.0f, 1.0f, 0.0f });

        if (Vector3Length(tangent) < 0.001f)
        {
            tangent = Vector3CrossProduct(unitDirection, Vector3{ 1.0f, 0.0f, 0.0f });
        }

        tangent = Vector3Normalize(tangent);
        Vector3 bitangent = Vector3Normalize(Vector3CrossProduct(unitDirection, tangent));

        constexpr float epsilon = 0.0025f;

        Vector3 dirA = Vector3Normalize(Vector3Add(unitDirection, Vector3Scale(tangent, epsilon)));
        Vector3 dirB = Vector3Normalize(Vector3Subtract(unitDirection, Vector3Scale(tangent, epsilon)));
        Vector3 dirC = Vector3Normalize(Vector3Add(unitDirection, Vector3Scale(bitangent, epsilon)));
        Vector3 dirD = Vector3Normalize(Vector3Subtract(unitDirection, Vector3Scale(bitangent, epsilon)));

        float hA = SamplePlanetHeight(object, dirA);
        float hB = SamplePlanetHeight(object, dirB);
        float hC = SamplePlanetHeight(object, dirC);
        float hD = SamplePlanetHeight(object, dirD);

        Vector3 pointA = Vector3Scale(dirA, 1.0f + hA);
        Vector3 pointB = Vector3Scale(dirB, 1.0f + hB);
        Vector3 pointC = Vector3Scale(dirC, 1.0f + hC);
        Vector3 pointD = Vector3Scale(dirD, 1.0f + hD);

        Vector3 dTangent = Vector3Subtract(pointA, pointB);
        Vector3 dBitangent = Vector3Subtract(pointC, pointD);

        return Vector3Normalize(Vector3CrossProduct(dBitangent, dTangent));
    }
}