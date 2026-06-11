#include "SpaceBackgroundRenderer.h"

#include <raylib.h>
#include <raymath.h>

#include <cmath>
#include <vector>

namespace SpaceSim
{
    namespace
    {
        struct BackgroundStar
        {
            Vector3 direction{};
            float brightness = 1.0f;
            float size = 1.0f;
            float shimmerSpeed = 1.0f;
            float shimmerPhase = 0.0f;
            Color baseColor = WHITE;
            bool isBright = false;
        };

        static Texture2D GetStarGlowTexture()
        {
            static bool initialized = false;
            static Texture2D texture{};

            if (!initialized)
            {
                constexpr int size = 32;

                Image image = GenImageColor(size, size, BLANK);

                const float center = static_cast<float>(size - 1) * 0.5f;

                for (int y = 0; y < size; ++y)
                {
                    for (int x = 0; x < size; ++x)
                    {
                        const float dx = (static_cast<float>(x) - center) / center;
                        const float dy = (static_cast<float>(y) - center) / center;
                        const float distance = sqrtf(dx * dx + dy * dy);

                        float alpha = 1.0f - distance;
                        alpha = Clamp(alpha, 0.0f, 1.0f);
                        alpha = alpha * alpha * alpha;

                        ImageDrawPixel(
                            &image,
                            x,
                            y,
                            Color{
                                255,
                                255,
                                255,
                                static_cast<unsigned char>(alpha * 255.0f)
                            }
                        );
                    }
                }

                texture = LoadTextureFromImage(image);
                GenTextureMipmaps(&texture);
                SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
                UnloadImage(image);

                initialized = true;
            }

            return texture;
        }

        static void DrawStarBillboard(
            const Camera3D& camera,
            Vector3 position,
            Color color,
            float billboardSize,
            unsigned char alpha
        )
        {
            Texture2D glowTexture = GetStarGlowTexture();

            Color drawColor = color;
            drawColor.a = alpha;

            DrawBillboardPro(
                camera,
                glowTexture,
                Rectangle{
                    0.0f,
                    0.0f,
                    static_cast<float>(glowTexture.width),
                    static_cast<float>(glowTexture.height)
                },
                position,
                camera.up,
                Vector2{ billboardSize, billboardSize },
                Vector2{ billboardSize * 0.5f, billboardSize * 0.5f },
                0.0f,
                drawColor
            );
        }

        static void DrawSmallStarBillboard(
            const Camera3D& camera,
            Vector3 position,
            Color color,
            float brightness
        )
        {
            const float billboardSize = Clamp(
                0.95f + brightness * 1.1f,
                0.95f,
                1.7f
            );

            const unsigned char alpha = static_cast<unsigned char>(
                Clamp(105.0f + brightness * 110.0f, 105.0f, 210.0f)
            );

            DrawStarBillboard(camera, position, color, billboardSize, alpha);
        }

        static void DrawMediumStarBillboard(
            const Camera3D& camera,
            Vector3 position,
            Color color,
            float brightness
        )
        {
            const float billboardSize = Clamp(
                2.2f + brightness * 2.2f,
                2.2f,
                4.2f
            );

            const unsigned char alpha = static_cast<unsigned char>(
                Clamp(150.0f + brightness * 105.0f, 150.0f, 245.0f)
            );

            DrawStarBillboard(camera, position, color, billboardSize, alpha);
        }

        static void DrawBrightStarBillboard(
            const Camera3D& camera,
            Vector3 position,
            Color color,
            float size
        )
        {
            DrawStarBillboard(camera, position, color, size * 2.3f, 130);
            DrawStarBillboard(camera, position, WHITE, Clamp(size * 0.45f, 1.2f, 2.0f), 235);
        }

        static float Hash01(unsigned int value)
        {
            value ^= value >> 16;
            value *= 0x7feb352d;
            value ^= value >> 15;
            value *= 0x846ca68b;
            value ^= value >> 16;

            return static_cast<float>(value) / static_cast<float>(0xffffffffu);
        }

        static Vector3 GenerateStarDirection(int index)
        {
            const float u = Hash01(index * 17u + 1u);
            const float v = Hash01(index * 29u + 7u);

            const float theta = 2.0f * PI * u;
            const float z = 2.0f * v - 1.0f;
            const float r = sqrtf(1.0f - z * z);

            return Vector3{
                r * cosf(theta),
                z,
                r * sinf(theta)
            };
        }

        static Color GenerateStarColor(int index, float brightness)
        {
            const float colorRoll = Hash01(index * 53u + 3u);

            unsigned char base = static_cast<unsigned char>(180.0f + brightness * 75.0f);

            if (colorRoll < 0.12f)
            {
                return Color{ 170, 200, base, 255 };
            }

            if (colorRoll > 0.88f)
            {
                return Color{ base, 210, 170, 255 };
            }

            return Color{ base, base, base, 255 };
        }

        static std::vector<BackgroundStar> GenerateBackgroundStars()
        {
            constexpr int starCount = 2000;

            std::vector<BackgroundStar> stars;
            stars.reserve(starCount);

            for (int i = 0; i < starCount; ++i)
            {
                BackgroundStar star;

                star.direction = GenerateStarDirection(i);

                float brightness = Hash01(i * 101u + 11u);
                brightness = powf(brightness, 1.35f);

                star.brightness = brightness;
                star.size = 0.35f + brightness * 1.15f;
                star.shimmerSpeed = 0.25f + Hash01(i * 131u + 17u) * 0.8f;
                star.shimmerPhase = Hash01(i * 151u + 19u) * 6.28318f;
                star.baseColor = GenerateStarColor(i, brightness);
                star.isBright = brightness > 0.93f;

                if (star.isBright)
                {
                    star.size = 1.5f + brightness * 2.5f;
                }

                stars.push_back(star);
            }

            return stars;
        }

        static void DrawProceduralStarfield(const Camera3D& camera)
        {
            static std::vector<BackgroundStar> stars = GenerateBackgroundStars();

            constexpr float skyDistance = 820.0f;

            const float time = static_cast<float>(GetTime());

            for (const BackgroundStar& star : stars)
            {
                float shimmer =
                    0.92f + 0.08f * sinf(time * star.shimmerSpeed + star.shimmerPhase);

                Color color = star.baseColor;
                color.r = static_cast<unsigned char>(static_cast<float>(color.r) * shimmer);
                color.g = static_cast<unsigned char>(static_cast<float>(color.g) * shimmer);
                color.b = static_cast<unsigned char>(static_cast<float>(color.b) * shimmer);

                Vector3 position = Vector3Scale(star.direction, skyDistance);

                if (star.brightness < 0.68f)
                {
                    DrawSmallStarBillboard(camera, position, color, star.brightness);
                }
                else if (star.brightness < 0.93f)
                {
                    DrawMediumStarBillboard(camera, position, color, star.brightness);
                }
                else
                {
                    DrawBrightStarBillboard(camera, position, color, star.size);
                }
            }
        }
    }

    void SpaceBackgroundRenderer::render(const Camera3D& camera)
    {
        DrawProceduralStarfield(camera);
    }
}