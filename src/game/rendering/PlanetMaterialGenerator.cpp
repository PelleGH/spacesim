#include "rendering/PlanetMaterialGenerator.h"

#include "world/PlanetVisualGenerator.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace SpaceSim
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;

        static float Saturate(float v)
        {
            return Clamp(v, 0.0f, 1.0f);
        }

        static float Smooth01(float t)
        {
            t = Saturate(t);
            return t * t * (3.0f - 2.0f * t);
        }

        static float SmoothRange(float edge0, float edge1, float x)
        {
            if (std::abs(edge1 - edge0) <= 0.00001f)
            {
                return x >= edge1 ? 1.0f : 0.0f;
            }

            return Smooth01((x - edge0) / (edge1 - edge0));
        }

        static float Hash01(std::uint32_t value)
        {
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            value ^= value >> 16;

            return static_cast<float>(value) / static_cast<float>(0xffffffffu);
        }

        static std::uint32_t HashCell(int x, int y, int z, std::uint32_t seed)
        {
            std::uint32_t h = seed;
            h ^= static_cast<std::uint32_t>(x) * 374761393u;
            h ^= static_cast<std::uint32_t>(y) * 668265263u;
            h ^= static_cast<std::uint32_t>(z) * 2147483647u;
            h ^= h >> 13;
            h *= 1274126177u;
            h ^= h >> 16;
            return h;
        }

        static float ValueNoise3D(Vector3 p, std::uint32_t seed)
        {
            int x0 = static_cast<int>(std::floor(p.x));
            int y0 = static_cast<int>(std::floor(p.y));
            int z0 = static_cast<int>(std::floor(p.z));

            float fx = p.x - static_cast<float>(x0);
            float fy = p.y - static_cast<float>(y0);
            float fz = p.z - static_cast<float>(z0);

            fx = Smooth01(fx);
            fy = Smooth01(fy);
            fz = Smooth01(fz);

            auto sample = [&](int x, int y, int z)
            {
                return Hash01(HashCell(x, y, z, seed));
            };

            float c000 = sample(x0,     y0,     z0);
            float c100 = sample(x0 + 1, y0,     z0);
            float c010 = sample(x0,     y0 + 1, z0);
            float c110 = sample(x0 + 1, y0 + 1, z0);

            float c001 = sample(x0,     y0,     z0 + 1);
            float c101 = sample(x0 + 1, y0,     z0 + 1);
            float c011 = sample(x0,     y0 + 1, z0 + 1);
            float c111 = sample(x0 + 1, y0 + 1, z0 + 1);

            float x00 = Lerp(c000, c100, fx);
            float x10 = Lerp(c010, c110, fx);
            float x01 = Lerp(c001, c101, fx);
            float x11 = Lerp(c011, c111, fx);

            float y0v = Lerp(x00, x10, fy);
            float y1v = Lerp(x01, x11, fy);

            return Lerp(y0v, y1v, fz);
        }

        static float FractalNoise3D(Vector3 direction, float frequency, std::uint32_t seed, int octaves = 6)
        {
            float total = 0.0f;
            float amplitude = 0.5f;
            float amplitudeSum = 0.0f;

            Vector3 p = Vector3Scale(direction, frequency);

            for (int octave = 0; octave < octaves; ++octave)
            {
                total += ValueNoise3D(p, seed + static_cast<std::uint32_t>(octave * 1013)) * amplitude;
                amplitudeSum += amplitude;

                p = Vector3Scale(p, 2.03f);
                amplitude *= 0.5f;
            }

            if (amplitudeSum <= 0.00001f)
            {
                return 0.0f;
            }

            return total / amplitudeSum;
        }

        static float RidgedNoise3D(Vector3 direction, float frequency, std::uint32_t seed)
        {
            float n = FractalNoise3D(direction, frequency, seed);
            return Saturate(1.0f - std::abs(n * 2.0f - 1.0f));
        }

        static Color LerpColor(Color a, Color b, float t)
        {
            t = Saturate(t);

            return Color{
                static_cast<unsigned char>(Lerp(static_cast<float>(a.r), static_cast<float>(b.r), t)),
                static_cast<unsigned char>(Lerp(static_cast<float>(a.g), static_cast<float>(b.g), t)),
                static_cast<unsigned char>(Lerp(static_cast<float>(a.b), static_cast<float>(b.b), t)),
                static_cast<unsigned char>(Lerp(static_cast<float>(a.a), static_cast<float>(b.a), t))
            };
        }

        static Color MultiplyColor(Color color, float amount)
        {
            return Color{
                static_cast<unsigned char>(Clamp(static_cast<float>(color.r) * amount, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(static_cast<float>(color.g) * amount, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(static_cast<float>(color.b) * amount, 0.0f, 255.0f)),
                color.a
            };
        }

        static Color MakeMaterial(float waterMask, float secondaryMask, float roughness, float relief)
        {
            return Color{
                static_cast<unsigned char>(Clamp(waterMask * 255.0f, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(secondaryMask * 255.0f, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(roughness * 255.0f, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(relief * 255.0f, 0.0f, 255.0f))
            };
        }

        static Vector3 DirectionFromUv(float u, float v)
        {
            float longitude = (u - 0.5f) * 2.0f * Pi;
            float latitude = (0.5f - v) * Pi;

            float cosLat = std::cos(latitude);

            return Vector3Normalize(Vector3{
                std::cos(longitude) * cosLat,
                std::sin(latitude),
                std::sin(longitude) * cosLat
            });
        }

        struct PlanetMaterialSample
        {
            Color albedo{};
            Color material{};
            float height = 0.0f;     // normal-map height only; mesh height remains in PlanetTerrainGenerator
        };

        static PlanetMaterialSample SampleOceanWorldMaterial(const GlobalObject& object, Vector3 dir)
        {
            const std::uint32_t seed = object.planetData.seed;

            float continents = FractalNoise3D(dir, 2.15f, seed + 100u);
            float detail = FractalNoise3D(dir, 14.0f, seed + 1100u);
            float fine = FractalNoise3D(dir, 38.0f, seed + 1200u);
            float dryness = FractalNoise3D(dir, 5.0f, seed + 1300u);

            // Lower first value = more land. Higher = more ocean.
            float landMask = SmoothRange(0.545f, 0.595f, continents);
            float waterMask = 1.0f - landMask;

            // Narrow shelf keeps coasts from becoming a cyan paint outline.
            float coastBand = SmoothRange(0.505f, 0.550f, continents) * waterMask;

            // Darker and slightly richer ocean palette.
            // Add a bit of macro variation so the sea does not read as one flat cyan plate.
            Color deepOcean{ 1, 8, 22, 255 };
            Color midOcean{ 5, 22, 54, 255 };
            Color shallowOcean{ 16, 52, 88, 255 };

            float oceanVariation = FractalNoise3D(dir, 3.2f, seed + 1400u);
            float currentBands = RidgedNoise3D(dir, 8.5f, seed + 1450u);

            Color ocean = LerpColor(deepOcean, midOcean, oceanVariation * 0.55f);
            ocean = LerpColor(ocean, shallowOcean, coastBand * 0.42f);
            ocean = MultiplyColor(ocean, 0.84f + currentBands * 0.18f);

            float latitude = std::abs(dir.y);

            Color lushLand{ 34, 49, 31, 255 };
            Color temperateLand{ 62, 66, 45, 255 };
            Color dryLand{ 100, 82, 55, 255 };
            Color rockyLand{ 82, 80, 72, 255 };
            Color coldLand{ 122, 128, 122, 255 };

            float landDetail = detail * 0.7f + fine * 0.3f;
            float rockMask = SmoothRange(0.68f, 0.90f, detail);
            float dryMask = SmoothRange(0.58f, 0.86f, dryness);
            float coldMask = SmoothRange(0.74f, 0.97f, latitude);

            Color land = LerpColor(lushLand, temperateLand, SmoothRange(0.25f, 0.75f, landDetail));
            land = LerpColor(land, dryLand, dryMask * 0.45f);
            land = LerpColor(land, rockyLand, rockMask * 0.35f);
            land = LerpColor(land, coldLand, coldMask * 0.30f);

            Color albedo = LerpColor(ocean, land, landMask);
            albedo = MultiplyColor(albedo, 0.94f + fine * 0.08f);
            albedo.a = static_cast<unsigned char>(Clamp(landMask * 255.0f, 0.0f, 255.0f));

            float inlandFade = SmoothRange(0.60f, 0.70f, continents);
            float mountainMask = SmoothRange(0.68f, 0.95f, detail);
            float ridge = RidgedNoise3D(dir, 26.0f, seed + 1800u);

            float height = landMask * inlandFade * (
                fine * 0.25f +
                ridge * 0.45f +
                mountainMask * 0.95f
            );

            float landRoughness = Clamp(0.55f + rockMask * 0.30f + dryMask * 0.15f, 0.0f, 1.0f);
            float terrainRelief = Clamp(detail * 0.45f + fine * 0.25f + mountainMask * 0.45f, 0.0f, 1.0f);

            return PlanetMaterialSample{
                albedo,
                MakeMaterial(waterMask, coastBand, landRoughness, terrainRelief),
                height
            };
        }

        static PlanetMaterialSample SampleRockMaterial(const GlobalObject& object, Vector3 dir)
        {
            const PlanetVisual visual = GeneratePlanetVisual(object);
            const std::uint32_t seed = object.planetData.seed;

            float broad = FractalNoise3D(dir, 2.8f, seed + 2000u);
            float detail = FractalNoise3D(dir, 15.0f, seed + 2100u);
            float ridges = RidgedNoise3D(dir, 18.0f, seed + 2200u);
            float fine = FractalNoise3D(dir, 45.0f, seed + 2300u, 5);

            Color base = visual.baseColor;
            Color secondary = visual.secondaryColor;
            Color highlands{ 150, 140, 125, 255 };

            if (object.planetData.planetClass == PlanetClass::BarrenMoon)
            {
                highlands = Color{ 170, 170, 165, 255 };
            }
            else if (object.planetData.planetClass == PlanetClass::CarbonWorld)
            {
                highlands = Color{ 115, 85, 72, 255 };
            }

            Color albedo = LerpColor(base, secondary, broad * 0.70f);
            albedo = LerpColor(albedo, highlands, ridges * 0.22f);
            albedo = MultiplyColor(albedo, 0.82f + detail * 0.24f + fine * 0.06f);
            albedo.a = 255;

            float height = broad * 0.22f + ridges * 0.85f + fine * 0.12f;
            float roughness = Clamp(0.72f + ridges * 0.20f, 0.0f, 1.0f);
            float relief = Clamp(detail * 0.35f + ridges * 0.65f, 0.0f, 1.0f);

            return PlanetMaterialSample{
                albedo,
                MakeMaterial(0.0f, ridges, roughness, relief),
                height
            };
        }

        static PlanetMaterialSample SampleDesertMaterial(const GlobalObject& object, Vector3 dir)
        {
            const PlanetVisual visual = GeneratePlanetVisual(object);
            const std::uint32_t seed = object.planetData.seed;

            float dunes = FractalNoise3D(dir, 8.0f, seed + 3000u);
            float rock = RidgedNoise3D(dir, 17.0f, seed + 3100u);
            float broad = FractalNoise3D(dir, 2.2f, seed + 3200u);

            Color sand = visual.baseColor;
            Color darkSand = visual.secondaryColor;
            Color paleSand{ 214, 176, 110, 255 };

            Color albedo = LerpColor(darkSand, sand, broad * 0.65f + dunes * 0.25f);
            albedo = LerpColor(albedo, paleSand, dunes * 0.25f);
            albedo = MultiplyColor(albedo, 0.86f + rock * 0.14f);
            albedo.a = 255;

            float height = dunes * 0.22f + rock * 0.60f + broad * 0.18f;
            float relief = Clamp(rock * 0.55f + dunes * 0.25f, 0.0f, 1.0f);

            return PlanetMaterialSample{
                albedo,
                MakeMaterial(0.0f, dunes, 0.82f, relief),
                height
            };
        }

        static PlanetMaterialSample SampleIceMaterial(const GlobalObject& object, Vector3 dir)
        {
            const PlanetVisual visual = GeneratePlanetVisual(object);
            const std::uint32_t seed = object.planetData.seed;

            float plates = FractalNoise3D(dir, 4.0f, seed + 4000u);
            float cracks = RidgedNoise3D(dir, 24.0f, seed + 4100u);
            float frost = FractalNoise3D(dir, 34.0f, seed + 4200u, 5);

            Color blueIce = visual.secondaryColor;
            Color paleIce = visual.baseColor;
            Color whiteIce{ 230, 240, 245, 255 };

            Color albedo = LerpColor(blueIce, paleIce, plates * 0.55f);
            albedo = LerpColor(albedo, whiteIce, frost * 0.35f);
            albedo = MultiplyColor(albedo, 0.92f + frost * 0.08f);
            albedo.a = 255;

            float height = plates * 0.20f + cracks * 0.45f;
            float relief = Clamp(cracks * 0.55f + frost * 0.25f, 0.0f, 1.0f);

            return PlanetMaterialSample{
                albedo,
                MakeMaterial(0.0f, cracks, 0.38f, relief),
                height
            };
        }

        static PlanetMaterialSample SampleLavaMaterial(const GlobalObject& object, Vector3 dir)
        {
            const std::uint32_t seed = object.planetData.seed;

            float plates = FractalNoise3D(dir, 3.2f, seed + 5000u);
            float cracks = RidgedNoise3D(dir, 22.0f, seed + 5100u);
            float heat = FractalNoise3D(dir, 5.0f, seed + 5200u);

            float crackMask = SmoothRange(0.84f, 0.94f, cracks);
            float poolMask = SmoothRange(0.79f, 0.93f, heat) * 0.28f;
            float lavaMask = Saturate(crackMask + poolMask);

            Color crust = LerpColor(Color{ 18, 15, 13, 255 }, Color{ 72, 48, 34, 255 }, plates);
            Color warmCrust = LerpColor(crust, Color{ 110, 58, 26, 255 }, SmoothRange(0.50f, 0.82f, heat) * 0.35f);
            Color lava = LerpColor(Color{ 255, 68, 8, 255 }, Color{ 255, 205, 45, 255 }, heat);

            Color albedo = LerpColor(warmCrust, lava, lavaMask);
            albedo.a = 255;

            float height = plates * 0.24f + cracks * 0.35f;
            float relief = Clamp(plates * 0.35f + cracks * 0.55f, 0.0f, 1.0f);

            // G carries the hot/emissive hint. The current shader keeps this subtle;
            // a later lava shader can turn it into real night-side glow.
            return PlanetMaterialSample{
                albedo,
                MakeMaterial(0.0f, lavaMask, 0.78f, relief),
                height
            };
        }

        static PlanetMaterialSample SampleGasGiantMaterial(const GlobalObject& object, Vector3 dir)
        {
            const PlanetVisual visual = GeneratePlanetVisual(object);
            const std::uint32_t seed = object.planetData.seed;

            float latitude = dir.y * 0.5f + 0.5f;
            float broadTurbulence = FractalNoise3D(dir, 4.2f, seed + 6000u);
            float fineTurbulence = FractalNoise3D(dir, 18.0f, seed + 6100u, 5);

            float distortedLatitude = latitude + (broadTurbulence - 0.5f) * 0.09f + (fineTurbulence - 0.5f) * 0.025f;
            float bandWave = std::sin(distortedLatitude * 20.0f * Pi + broadTurbulence * 2.0f);
            float bandMask = Smooth01(bandWave * 0.5f + 0.5f);

            Color albedo = LerpColor(visual.baseColor, visual.secondaryColor, bandMask);

            if (object.planetData.planetClass == PlanetClass::IceGiant)
            {
                Color pale{ 170, 230, 235, 255 };
                albedo = LerpColor(albedo, pale, fineTurbulence * 0.18f);
            }
            else
            {
                Color cream{ 230, 190, 130, 255 };
                albedo = LerpColor(albedo, cream, fineTurbulence * 0.12f);
            }

            albedo = MultiplyColor(albedo, 0.90f + fineTurbulence * 0.10f);
            albedo.a = 255;

            return PlanetMaterialSample{
                albedo,
                MakeMaterial(0.0f, bandMask, 0.65f, 0.05f),
                0.0f
            };
        }

        static PlanetMaterialSample SamplePlanetMaterial(const GlobalObject& object, Vector3 dir)
        {
            if (!object.hasPlanetData)
            {
                Color color = object.color;
                color.a = 255;
                return PlanetMaterialSample{ color, MakeMaterial(0.0f, 0.0f, 0.75f, 0.0f), 0.0f };
            }

            switch (object.planetData.planetClass)
            {
            case PlanetClass::OceanWorld:
                return SampleOceanWorldMaterial(object, dir);

            case PlanetClass::DesertWorld:
                return SampleDesertMaterial(object, dir);

            case PlanetClass::IceWorld:
                return SampleIceMaterial(object, dir);

            case PlanetClass::LavaWorld:
                return SampleLavaMaterial(object, dir);

            case PlanetClass::GasGiant:
            case PlanetClass::IceGiant:
                return SampleGasGiantMaterial(object, dir);

            case PlanetClass::Rocky:
            case PlanetClass::BarrenMoon:
            case PlanetClass::CarbonWorld:
            default:
                return SampleRockMaterial(object, dir);
            }
        }

        static Vector3 EstimateMaterialNormal(const GlobalObject& object, Vector3 dir)
        {
            Vector3 up = std::abs(dir.y) < 0.95f
                ? Vector3{ 0.0f, 1.0f, 0.0f }
                : Vector3{ 1.0f, 0.0f, 0.0f };

            Vector3 tangent = Vector3Normalize(Vector3CrossProduct(up, dir));
            Vector3 bitangent = Vector3Normalize(Vector3CrossProduct(dir, tangent));

            float e = 0.0045f;

            float h0 = SamplePlanetMaterial(object, dir).height;
            float hx = SamplePlanetMaterial(object, Vector3Normalize(Vector3Add(dir, Vector3Scale(tangent, e)))).height;
            float hy = SamplePlanetMaterial(object, Vector3Normalize(Vector3Add(dir, Vector3Scale(bitangent, e)))).height;

            float strength = 2.2f;

            if (object.hasPlanetData &&
                (object.planetData.planetClass == PlanetClass::GasGiant ||
                 object.planetData.planetClass == PlanetClass::IceGiant))
            {
                strength = 0.0f;
            }

            Vector3 normal = Vector3Normalize(Vector3Subtract(
                dir,
                Vector3Add(
                    Vector3Scale(tangent, (hx - h0) * strength),
                    Vector3Scale(bitangent, (hy - h0) * strength)
                )
            ));

            return normal;
        }

        static Color EncodeNormal(Vector3 normal)
        {
            normal = Vector3Normalize(normal);

            return Color{
                static_cast<unsigned char>(Clamp((normal.x * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp((normal.y * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp((normal.z * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f)),
                255
            };
        }

        static Texture2D LoadTextureFromPixels(const std::vector<Color>& pixels, int width, int height)
        {
            Image image{};
            image.data = const_cast<Color*>(pixels.data());
            image.width = width;
            image.height = height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

            Texture2D texture = LoadTextureFromImage(image);
            GenTextureMipmaps(&texture);
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);

            return texture;
        }
    }

    PlanetMaterialMaps GeneratePlanetMaterialMaps(
        const GlobalObject& object,
        int width,
        int height
    )
    {
        std::vector<Color> albedoPixels;
        std::vector<Color> normalPixels;
        std::vector<Color> materialPixels;

        albedoPixels.resize(static_cast<std::size_t>(width * height));
        normalPixels.resize(static_cast<std::size_t>(width * height));
        materialPixels.resize(static_cast<std::size_t>(width * height));

        for (int y = 0; y < height; ++y)
        {
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);

            for (int x = 0; x < width; ++x)
            {
                float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);

                Vector3 dir = DirectionFromUv(u, v);

                PlanetMaterialSample sample = SamplePlanetMaterial(object, dir);
                Vector3 normal = EstimateMaterialNormal(object, dir);

                std::size_t index = static_cast<std::size_t>(y * width + x);

                albedoPixels[index] = sample.albedo;
                normalPixels[index] = EncodeNormal(normal);
                materialPixels[index] = sample.material;
            }
        }

        return PlanetMaterialMaps{
            LoadTextureFromPixels(albedoPixels, width, height),
            LoadTextureFromPixels(normalPixels, width, height),
            LoadTextureFromPixels(materialPixels, width, height)
        };
    }
}
