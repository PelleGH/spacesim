#include "rendering/PlanetSurfaceMapGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <raymath.h>

namespace SpaceSim
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;

        static float Clamp01(float value)
        {
            return std::clamp(value, 0.0f, 1.0f);
        }

        static float SmoothStep(float edge0, float edge1, float x)
        {
            float t = Clamp01((x - edge0) / (edge1 - edge0));
            return t * t * (3.0f - 2.0f * t);
        }

        static Color MakeColor(float r, float g, float b, float a = 1.0f)
        {
            return Color{
                static_cast<unsigned char>(Clamp01(r) * 255.0f),
                static_cast<unsigned char>(Clamp01(g) * 255.0f),
                static_cast<unsigned char>(Clamp01(b) * 255.0f),
                static_cast<unsigned char>(Clamp01(a) * 255.0f)
            };
        }
        static Color MakeNormalColor(Vector3 normal)
        {
            normal = Vector3Normalize(normal);

            return MakeColor(
                normal.x * 0.5f + 0.5f,
                normal.y * 0.5f + 0.5f,
                normal.z * 0.5f + 0.5f,
                1.0f
            );
        }
        static Color LerpColor(Color a, Color b, float t)
        {
            t = Clamp01(t);

            return Color{
                static_cast<unsigned char>(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
                static_cast<unsigned char>(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
                static_cast<unsigned char>(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
                static_cast<unsigned char>(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t)
            };
        }

        static std::uint32_t HashU32(std::uint32_t x)
        {
            x ^= x >> 16;
            x *= 0x7feb352du;
            x ^= x >> 15;
            x *= 0x846ca68bu;
            x ^= x >> 16;
            return x;
        }

        static float Hash31(int x, int y, int z, std::uint32_t seed)
        {
            std::uint32_t h = seed;
            h ^= HashU32(static_cast<std::uint32_t>(x) * 374761393u);
            h ^= HashU32(static_cast<std::uint32_t>(y) * 668265263u);
            h ^= HashU32(static_cast<std::uint32_t>(z) * 2147483647u);
            return static_cast<float>(HashU32(h) & 0x00ffffffu) / static_cast<float>(0x01000000u);
        }

        static float ValueNoise3D(Vector3 p, std::uint32_t seed)
        {
            int ix = static_cast<int>(std::floor(p.x));
            int iy = static_cast<int>(std::floor(p.y));
            int iz = static_cast<int>(std::floor(p.z));

            float fx = p.x - static_cast<float>(ix);
            float fy = p.y - static_cast<float>(iy);
            float fz = p.z - static_cast<float>(iz);

            fx = fx * fx * (3.0f - 2.0f * fx);
            fy = fy * fy * (3.0f - 2.0f * fy);
            fz = fz * fz * (3.0f - 2.0f * fz);

            float n000 = Hash31(ix,     iy,     iz,     seed);
            float n100 = Hash31(ix + 1, iy,     iz,     seed);
            float n010 = Hash31(ix,     iy + 1, iz,     seed);
            float n110 = Hash31(ix + 1, iy + 1, iz,     seed);
            float n001 = Hash31(ix,     iy,     iz + 1, seed);
            float n101 = Hash31(ix + 1, iy,     iz + 1, seed);
            float n011 = Hash31(ix,     iy + 1, iz + 1, seed);
            float n111 = Hash31(ix + 1, iy + 1, iz + 1, seed);

            float nx00 = n000 + (n100 - n000) * fx;
            float nx10 = n010 + (n110 - n010) * fx;
            float nx01 = n001 + (n101 - n001) * fx;
            float nx11 = n011 + (n111 - n011) * fx;

            float nxy0 = nx00 + (nx10 - nx00) * fy;
            float nxy1 = nx01 + (nx11 - nx01) * fy;

            return nxy0 + (nxy1 - nxy0) * fz;
        }

        static float Fbm(Vector3 p, std::uint32_t seed)
        {
            float total = 0.0f;
            float amplitude = 0.5f;

            for (int i = 0; i < 5; ++i)
            {
                total += ValueNoise3D(p, seed + static_cast<std::uint32_t>(i) * 1013u) * amplitude;

                p.x *= 2.03f;
                p.y *= 2.03f;
                p.z *= 2.03f;

                amplitude *= 0.5f;
            }

            return total;
        }
        static float RidgedFbm(Vector3 p, std::uint32_t seed)
        {
            float total = 0.0f;
            float amplitude = 0.5f;

            for (int i = 0; i < 5; ++i)
            {
                float n = ValueNoise3D(p, seed + static_cast<std::uint32_t>(i) * 1777u);
                n = 1.0f - std::abs(n * 2.0f - 1.0f);

                total += n * amplitude;

                p.x *= 2.08f;
                p.y *= 2.08f;
                p.z *= 2.08f;

                amplitude *= 0.5f;
            }

            return Clamp01(total);
        }

        static float CraterLayer(Vector3 dir, float frequency, std::uint32_t seed)
        {
            Vector3 p{
                dir.x * frequency,
                dir.y * frequency,
                dir.z * frequency
            };

            int baseX = static_cast<int>(std::floor(p.x));
            int baseY = static_cast<int>(std::floor(p.y));
            int baseZ = static_cast<int>(std::floor(p.z));

            float bestCrater = 0.0f;

            for (int oz = -1; oz <= 1; ++oz)
            {
                for (int oy = -1; oy <= 1; ++oy)
                {
                    for (int ox = -1; ox <= 1; ++ox)
                    {
                        int cx = baseX + ox;
                        int cy = baseY + oy;
                        int cz = baseZ + oz;

                        float rx = Hash31(cx, cy, cz, seed + 11u);
                        float ry = Hash31(cx, cy, cz, seed + 23u);
                        float rz = Hash31(cx, cy, cz, seed + 37u);
                        float rr = Hash31(cx, cy, cz, seed + 51u);

                        Vector3 center{
                            static_cast<float>(cx) + rx,
                            static_cast<float>(cy) + ry,
                            static_cast<float>(cz) + rz
                        };

                        float dx = p.x - center.x;
                        float dy = p.y - center.y;
                        float dz = p.z - center.z;

                        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                        float radius = 0.16f + rr * 0.18f;

                        float bowl = 1.0f - SmoothStep(radius * 0.25f, radius, dist);
                        float rim =
                            SmoothStep(radius * 0.70f, radius, dist) *
                            (1.0f - SmoothStep(radius, radius * 1.28f, dist));

                        float crater = bowl * 0.65f + rim * 0.45f;
                        bestCrater = std::max(bestCrater, crater);
                    }
                }
            }

            return Clamp01(bestCrater);
        }

        static float CraterField(Vector3 dir, std::uint32_t seed)
        {
            float large = CraterLayer(dir, 6.0f, seed + 3000u);
            float medium = CraterLayer(dir, 13.0f, seed + 4000u) * 0.65f;
            float small = CraterLayer(dir, 28.0f, seed + 5000u) * 0.35f;

            return Clamp01(std::max(large, std::max(medium, small)));
        }
        static Vector3 DirectionFromUv(float u, float v)
        {
            float longitude = (u - 0.5f) * 2.0f * Pi;
            float latitude = (0.5f - v) * Pi;

            float cosLat = std::cos(latitude);

            return Vector3{
                std::cos(longitude) * cosLat,
                std::sin(latitude),
                std::sin(longitude) * cosLat
            };
        }

        static std::uint32_t SeedFromObject(const GlobalObject& object)
        {
            std::uint32_t seed = 2166136261u;

            for (char c : object.id)
            {
                seed ^= static_cast<std::uint32_t>(c);
                seed *= 16777619u;
            }

            return seed;
        }
    }
    struct SurfacePreset
    {
        Color deepWater;
        Color shallowWater;

        Color landA;
        Color landB;
        Color landC;

        float seaLevelLow;
        float seaLevelHigh;

        float waterRoughness;
        float waterSpecular;

        float landRoughness;
        float landSpecular;

        bool hasLiquid = true;
    };

    static SurfacePreset GetSurfacePreset(PlanetClass planetClass)
    {
        switch (planetClass)
        {
        case PlanetClass::Ocean:
            return SurfacePreset{
                MakeColor(0.004f, 0.022f, 0.065f),
                MakeColor(0.020f, 0.070f, 0.120f),

                MakeColor(0.16f, 0.23f, 0.12f),
                MakeColor(0.32f, 0.27f, 0.16f),
                MakeColor(0.42f, 0.45f, 0.36f),

                0.50f,
                0.58f,

                0.28f,
                0.55f,

                0.92f,
                0.015f,

                true
            };

        case PlanetClass::Rocky:
            return SurfacePreset{
                MakeColor(0.015f, 0.020f, 0.025f),
                MakeColor(0.030f, 0.035f, 0.040f),

                MakeColor(0.30f, 0.28f, 0.24f),
                MakeColor(0.42f, 0.36f, 0.28f),
                MakeColor(0.55f, 0.52f, 0.46f),

                0.22f,
                0.28f,

                0.75f,
                0.04f,

                0.94f,
                0.018f,

                false
            };

        case PlanetClass::Ice:
            return SurfacePreset{
                MakeColor(0.010f, 0.040f, 0.080f),
                MakeColor(0.035f, 0.090f, 0.140f),

                MakeColor(0.58f, 0.68f, 0.72f),
                MakeColor(0.72f, 0.78f, 0.80f),
                MakeColor(0.88f, 0.92f, 0.95f),

                0.38f,
                0.48f,

                0.36f,
                0.22f,

                0.62f,
                0.08f,

                true
            };

        case PlanetClass::Desert:
            return SurfacePreset{
                MakeColor(0.025f, 0.018f, 0.010f),
                MakeColor(0.045f, 0.035f, 0.020f),

                // landA / landB / landC
                MakeColor(0.38f, 0.25f, 0.12f),
                MakeColor(0.58f, 0.40f, 0.18f),
                MakeColor(0.78f, 0.62f, 0.34f),

                0.10f,
                0.16f,

                0.55f,
                0.03f,

                0.98f,
                0.012f,

                false
            };

        case PlanetClass::Barren:
        default:
            return SurfacePreset{
                MakeColor(0.010f, 0.010f, 0.012f),
                MakeColor(0.020f, 0.020f, 0.024f),

                MakeColor(0.24f, 0.23f, 0.22f),
                MakeColor(0.36f, 0.34f, 0.31f),
                MakeColor(0.50f, 0.48f, 0.44f),

                0.04f,
                0.08f,

                0.80f,
                0.02f,

                0.90f,
                0.025f,
                false
            };
        }
    }
    PlanetSurfaceMaps GeneratePlanetSurfaceMaps(
        const GlobalObject& object,
        int width,
        int height
    )
    {
        PlanetSurfaceMaps maps{};
        maps.width = width;
        maps.height = height;

        Image albedoImage = GenImageColor(width, height, BLACK);
        Image materialImage = GenImageColor(width, height, BLACK);
        Image normalImage = GenImageColor(width, height, Color{ 128, 128, 255, 255 });

        std::vector<float> heightValues;
        heightValues.resize(static_cast<std::size_t>(width * height), 0.0f);

        const std::uint32_t seed = SeedFromObject(object);

        const SurfacePreset preset = GetSurfacePreset(object.planetClass);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                float u = static_cast<float>(x) / static_cast<float>(width - 1);
                float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);

                Vector3 dir = DirectionFromUv(u, v);

                float continents =
                    Fbm(Vector3{ dir.x * 2.45f + 4.2f, dir.y * 2.45f + 1.7f, dir.z * 2.45f + 8.9f }, seed);

                continents +=
                    Fbm(Vector3{ dir.x * 6.0f + 1.1f, dir.y * 6.0f + 9.3f, dir.z * 6.0f + 2.8f }, seed + 77u) * 0.22f;

                float landMask = 1.0f;
                float waterMask = 0.0f;

                if (preset.hasLiquid)
                {
                    landMask = SmoothStep(
                        preset.seaLevelLow,
                        preset.seaLevelHigh,
                        continents
                    );

                    waterMask = 1.0f - landMask;
                }

                float dryness =
                    Fbm(Vector3{ dir.x * 5.5f + 12.0f, dir.y * 5.5f, dir.z * 5.5f + 3.0f }, seed + 123u);

                float latitude = std::abs(dir.y);
                float ridges =
                    Fbm(Vector3{ dir.x * 18.0f + 2.0f, dir.y * 18.0f + 5.0f, dir.z * 18.0f + 9.0f }, seed + 456u);

                float fine =
                    Fbm(Vector3{ dir.x * 42.0f + 8.0f, dir.y * 42.0f + 1.0f, dir.z * 42.0f + 4.0f }, seed + 789u);

                float ridgeTerrain = RidgedFbm(
                    Vector3{
                        dir.x * 12.0f + 2.0f,
                        dir.y * 12.0f + 5.0f,
                        dir.z * 12.0f + 9.0f
                    },
                    seed + 1200u
                );

                float craterTerrain = 0.0f;
                float duneTerrain = 0.0f;

                float surfaceVariation = ridges * 0.55f + fine * 0.45f;

                if (object.planetClass == PlanetClass::Rocky ||
                    object.planetClass == PlanetClass::Barren)
                {
                    craterTerrain = CraterField(dir, seed);

                    float broadRock = Fbm(
                        Vector3{
                            dir.x * 5.5f + 3.0f,
                            dir.y * 5.5f + 5.0f,
                            dir.z * 5.5f + 11.0f
                        },
                        seed + 1700u
                    );

                    surfaceVariation =
                        broadRock * 0.34f +
                        ridgeTerrain * 0.36f +
                        craterTerrain * 0.22f +
                        fine * 0.08f;
                }

                if (object.planetClass == PlanetClass::Desert)
                {
                    float broadDunes = Fbm(
                        Vector3{
                            dir.x * 7.0f + 4.0f,
                            dir.y * 7.0f + 7.0f,
                            dir.z * 7.0f + 1.0f
                        },
                        seed + 1600u
                    );

                    float tightDunes = RidgedFbm(
                        Vector3{
                            dir.x * 22.0f + 12.0f,
                            dir.y * 4.0f,
                            dir.z * 22.0f + 6.0f
                        },
                        seed + 1650u
                    );

                    duneTerrain = broadDunes * 0.65f + tightDunes * 0.35f;

                    surfaceVariation =
                        duneTerrain * 0.70f +
                        fine * 0.15f +
                        ridges * 0.15f;
                }

                Color land = LerpColor(
                    preset.landA,
                    preset.landB,
                    SmoothStep(0.55f, 0.85f, dryness) * 0.55f
                );

                land = LerpColor(
                    land,
                    preset.landC,
                    SmoothStep(0.74f, 0.95f, latitude) * 0.25f
                );

                float coast = 1.0f - std::abs(landMask - 0.5f) * 2.0f;
                coast = Clamp01(coast);

                Color water = LerpColor(
                    preset.deepWater,
                    preset.shallowWater,
                    coast * 0.35f
                );

                Color albedo = preset.hasLiquid
                    ? LerpColor(water, land, landMask)
                    : land;
                if (!preset.hasLiquid)
                {
                    float darkPatch = SmoothStep(0.35f, 0.82f, surfaceVariation);
                    float brightPatch = SmoothStep(0.72f, 0.98f, fine);

                    float darkAmount = 0.20f;
                    float brightAmount = 0.10f;

                    if (object.planetClass == PlanetClass::Desert)
                    {
                        darkPatch = SmoothStep(0.38f, 0.90f, duneTerrain);
                        brightPatch = SmoothStep(0.55f, 0.92f, duneTerrain);
                        darkAmount = 0.10f;
                        brightAmount = 0.22f;
                    }

                    if (object.planetClass == PlanetClass::Rocky ||
                        object.planetClass == PlanetClass::Barren)
                    {
                        darkPatch = std::max(
                            SmoothStep(0.42f, 0.86f, ridgeTerrain),
                            SmoothStep(0.25f, 0.85f, craterTerrain)
                        );

                        brightPatch = SmoothStep(0.45f, 0.85f, craterTerrain);

                        darkAmount = 0.34f;
                        brightAmount = 0.12f;
                    }

                    Color darker = MakeColor(
                        static_cast<float>(albedo.r) / 255.0f * 0.62f,
                        static_cast<float>(albedo.g) / 255.0f * 0.62f,
                        static_cast<float>(albedo.b) / 255.0f * 0.62f
                    );

                    Color brighter = MakeColor(
                        std::min(static_cast<float>(albedo.r) / 255.0f * 1.22f, 1.0f),
                        std::min(static_cast<float>(albedo.g) / 255.0f * 1.22f, 1.0f),
                        std::min(static_cast<float>(albedo.b) / 255.0f * 1.22f, 1.0f)
                    );

                    albedo = LerpColor(albedo, darker, darkPatch * darkAmount);
                    albedo = LerpColor(albedo, brighter, brightPatch * brightAmount);
                }
                float roughness =
                    waterMask * preset.waterRoughness +
                    landMask * preset.landRoughness;

                float specularStrength =
                    waterMask * preset.waterSpecular +
                    landMask * preset.landSpecular;
                float heightPlaceholder = 0.0f;

                if (preset.hasLiquid)
                {
                    heightPlaceholder = landMask * SmoothStep(0.55f, 0.95f, continents);
                }
                else
                {
                    if (object.planetClass == PlanetClass::Desert)
                    {
                        float duneShape = SmoothStep(0.28f, 0.95f, duneTerrain);
                        float fineShape = SmoothStep(0.65f, 0.98f, fine);

                        heightPlaceholder = Clamp01(
                            duneShape * 0.82f +
                            fineShape * 0.18f
                        );
                    }
                    else if (object.planetClass == PlanetClass::Rocky ||
                            object.planetClass == PlanetClass::Barren)
                    {
                        float broadShape = SmoothStep(0.25f, 0.95f, continents);
                        float ridgeShape = SmoothStep(0.42f, 0.92f, ridgeTerrain);
                        float craterShape = SmoothStep(0.18f, 0.86f, craterTerrain);

                        heightPlaceholder = Clamp01(
                            broadShape * 0.30f +
                            ridgeShape * 0.40f +
                            craterShape * 0.30f
                        );
                    }
                    else
                    {
                        float broadShape = SmoothStep(0.25f, 0.95f, continents);
                        float ridgeShape = SmoothStep(0.50f, 0.95f, ridges);
                        float fineShape = SmoothStep(0.60f, 0.95f, fine);

                        heightPlaceholder = Clamp01(
                            broadShape * 0.55f +
                            ridgeShape * 0.30f +
                            fineShape * 0.15f
                        );
                    }
                }

                Color material = MakeColor(
                    waterMask,
                    roughness,
                    specularStrength,
                    heightPlaceholder
                );

                const int index = y * width + x;
                heightValues[static_cast<std::size_t>(index)] = heightPlaceholder;

                ImageDrawPixel(&albedoImage, x, y, albedo);
                ImageDrawPixel(&materialImage, x, y, material);
            }
        }
        for (int y = 0; y < height; ++y)
        {
            Color albedoEdge = GetImageColor(albedoImage, 0, y);
            Color materialEdge = GetImageColor(materialImage, 0, y);

            ImageDrawPixel(&albedoImage, width - 1, y, albedoEdge);
            ImageDrawPixel(&materialImage, width - 1, y, materialEdge);

            heightValues[static_cast<std::size_t>(y * width + (width - 1))] =
                heightValues[static_cast<std::size_t>(y * width)];
        }
        float normalStrength = 2.2f;

        switch (object.planetClass)
        {
        case PlanetClass::Ocean:
            normalStrength = 1.4f;
            break;

        case PlanetClass::Rocky:
            normalStrength = 1.85f;
            break;

        case PlanetClass::Ice:
            normalStrength = 1.4f;
            break;

        case PlanetClass::Desert:
            normalStrength = 0.75f;
            break;

        case PlanetClass::Barren:
            normalStrength = 1.7f;
            break;
        }

        auto sampleHeight = [&](int sx, int sy) -> float
        {
            if (sx < 0)
            {
                sx = width - 1;
            }

            if (sx >= width)
            {
                sx = 0;
            }

            sy = std::clamp(sy, 0, height - 1);

            return heightValues[static_cast<std::size_t>(sy * width + sx)];
        };

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                float hL = sampleHeight(x - 1, y);
                float hR = sampleHeight(x + 1, y);
                float hD = sampleHeight(x, y - 1);
                float hU = sampleHeight(x, y + 1);

                float dX = (hR - hL) * normalStrength;
                float dY = (hU - hD) * normalStrength;

                Vector3 normal{
                    -dX,
                    -dY,
                    1.0f
                };

                ImageDrawPixel(&normalImage, x, y, MakeNormalColor(normal));
            }
        }

        for (int y = 0; y < height; ++y)
        {
            Color normalEdge = GetImageColor(normalImage, 0, y);
            ImageDrawPixel(&normalImage, width - 1, y, normalEdge);
        }
        maps.albedo = LoadTextureFromImage(albedoImage);
        maps.material = LoadTextureFromImage(materialImage);
        maps.normal = LoadTextureFromImage(normalImage);

/*         GenTextureMipmaps(&maps.albedo);
        GenTextureMipmaps(&maps.material); */

        SetTextureFilter(maps.albedo, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(maps.material, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(maps.normal, TEXTURE_FILTER_BILINEAR);

        SetTextureWrap(maps.albedo, TEXTURE_WRAP_REPEAT);
        SetTextureWrap(maps.material, TEXTURE_WRAP_REPEAT);
        SetTextureWrap(maps.normal, TEXTURE_WRAP_REPEAT);

        UnloadImage(albedoImage);
        UnloadImage(materialImage);
        UnloadImage(normalImage);

        return maps;
    }

    void UnloadPlanetSurfaceMaps(PlanetSurfaceMaps& maps)
    {
        if (maps.albedo.id != 0)
        {
            UnloadTexture(maps.albedo);
            maps.albedo = Texture2D{};
        }

        if (maps.material.id != 0)
        {
            UnloadTexture(maps.material);
            maps.material = Texture2D{};
        }

        if (maps.normal.id != 0)
        {
            UnloadTexture(maps.normal);
            maps.normal = Texture2D{};
        }

        maps.width = 0;
        maps.height = 0;
    }
}