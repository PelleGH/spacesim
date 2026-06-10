#include "systems/RenderSystem.h"

#include "components/TransformComponent.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
namespace SpaceSim
{
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
    // Smaller outer glow so bright stars do not become streaky/smeary.
    DrawStarBillboard(camera, position, color, size * 2.3f, 130);

    // Bright core.
    DrawStarBillboard(camera, position, WHITE, Clamp(size * 0.45f, 1.2f, 2.0f), 235);
}
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
        return Color{ 170, 200, base, 255 }; // cool blue-white
    }

    if (colorRoll > 0.88f)
    {
        return Color{ base, 210, 170, 255 }; // warm yellow-white
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

            // Only the brightest stars get an extra glow + core pass.
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
    
    static Vector3 ToRenderDirection(DVec3 relative)
    {
        DVec3 direction = Normalize(relative);

        return Vector3{
            static_cast<float>(direction.x),
            static_cast<float>(direction.y),
            static_cast<float>(direction.z)
        };
    }

    static void DrawSatellite(Vector3 satellitePos, Color panelColor)
    {
        DrawCube(satellitePos, 8.0f, 4.0f, 4.0f, LIGHTGRAY);
        DrawCubeWires(satellitePos, 8.0f, 4.0f, 4.0f, RAYWHITE);

        DrawCube(
            Vector3{ satellitePos.x - 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.5f,
            5.0f,
            panelColor
        );

        DrawCube(
            Vector3{ satellitePos.x + 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.5f,
            5.0f,
            panelColor
        );

        DrawSphere(
            Vector3{ satellitePos.x, satellitePos.y + 4.0f, satellitePos.z },
            1.5f,
            RED
        );
    }
    static bool IsCelestialBody(GlobalObjectType type)
    {
        return type == GlobalObjectType::Sun ||
            type == GlobalObjectType::Planet;
    }
    struct SkyBodyDrawCommand
    {
        Vector3 position{};
        float radius = 1.0f;
        Color color = WHITE;
        double globalDistance = 0.0;
    };

    static float ComputeApparentSkyRadius(
        const GlobalObject& object,
        double globalDistance,
        float skyDistance
    )
    {
        if (globalDistance < 1.0)
        {
            return 1.0f;
        }

        float radius = static_cast<float>(
            object.visualRadius / globalDistance * static_cast<double>(skyDistance)
        );

        return Clamp(radius, 3.0f, 500.0f);
    }

    static void DrawDistantStarSystem3D(const GameWorld& world)
    {

        constexpr float skyDistance = 900.0f;

        std::vector<SkyBodyDrawCommand> drawCommands;

        for (const auto& object : world.starSystem.objects)
        {
            int objectIndex = static_cast<int>(&object - world.starSystem.objects.data());

            if (!IsCelestialBody(object.type))
            {
                continue;
            }

            DVec3 relative = object.position - world.globalPlayerPosition;
            double globalDistance = Length(relative);

            if (globalDistance < 1.0)
            {
                continue;
            }

            Vector3 direction = ToRenderDirection(relative);
            Vector3 skyPos = Vector3Scale(direction, skyDistance);

            float radius = ComputeApparentSkyRadius(
                object,
                globalDistance,
                skyDistance
            );

            drawCommands.push_back(SkyBodyDrawCommand{
                skyPos,
                radius,
                object.color,
                globalDistance
            });
        }

        // Depth is disabled in the sky pass, so draw order controls occlusion.
        // Far objects first, near objects last.
        std::sort(
            drawCommands.begin(),
            drawCommands.end(),
            [](const SkyBodyDrawCommand& a, const SkyBodyDrawCommand& b)
            {
                return a.globalDistance > b.globalDistance;
            }
        );

        for (const SkyBodyDrawCommand& command : drawCommands)
        {
            DrawSphere(command.position, command.radius, command.color);
        }
    }
    static Vector3 GlobalToLocalPosition(const GameWorld& world, DVec3 globalPosition)
    {
        constexpr double localToGlobalScale = 0.001;

        DVec3 relative = globalPosition - world.activeBubbleOrigin;

        return Vector3{
            static_cast<float>(relative.x / localToGlobalScale),
            static_cast<float>(relative.y / localToGlobalScale),
            static_cast<float>(relative.z / localToGlobalScale)
        };
    }

    static bool IsLocalRenderableObject(GlobalObjectType type)
    {
        return type == GlobalObjectType::Satellite;
    }

    static void DrawObjectsInsideLocalBubble(const GameWorld& world)
    {
        constexpr double bubbleRadiusGlobal = 5000.0;

        for (const GlobalObject& object : world.starSystem.objects)
        {
            if (!IsLocalRenderableObject(object.type))
            {
                continue;
            }

            DVec3 relative = object.position - world.activeBubbleOrigin;

            if (Length(relative) > bubbleRadiusGlobal)
            {
                continue;
            }

            Vector3 localPosition = GlobalToLocalPosition(world, object.position);

            switch (object.type)
            {
            case GlobalObjectType::Satellite:
                DrawSatellite(localPosition, object.color);
                break;

            default:
                break;
            }
        }
    }
    void RenderSystem::renderSky(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        // Camera rotation only. Position is locked to origin so the sky does not
        // slide around when the player moves locally.
        Vector3 cameraForward = Vector3Normalize(Vector3Subtract(
            world.camera.target,
            world.camera.position
        ));

        Camera3D skyCamera{};
        skyCamera.position = Vector3{ 0.0f, 0.0f, 0.0f };
        skyCamera.target = cameraForward;
        skyCamera.up = world.camera.up;
        skyCamera.fovy = world.camera.fovy;
        skyCamera.projection = world.camera.projection;

        BeginMode3D(skyCamera);

        // The whole sky layer is a background painting.
        // Do not let stars or fake-distance planets use/write depth.
        // This prevents stars from punching through planets during FTL.
        rlDisableDepthTest();
        rlDisableDepthMask();

        DrawProceduralStarfield(skyCamera);
        rlDrawRenderBatchActive();
        rlSetTexture(0);

        DrawDistantStarSystem3D(world);
        rlDrawRenderBatchActive();

        // Restore normal depth behavior before leaving the sky pass.
        rlEnableDepthMask();
        rlEnableDepthTest();

        EndMode3D();
    }

    void RenderSystem::renderWorld(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        if (world.travelMode == TravelMode::FTLTravel)
        {
            return;
        }

        DrawObjectsInsideLocalBubble(world);

        auto view = world.registry.view<TransformComponent>();

        for (auto entity : view)
        {
            (void)entity;

            const auto& transform = view.get<TransformComponent>(entity);

            Matrix rotationMatrix = QuaternionToMatrix(transform.rotation);
            float16 rotationFloats = MatrixToFloatV(rotationMatrix);

            rlPushMatrix();

            rlTranslatef(transform.position.x, transform.position.y, transform.position.z);
            rlMultMatrixf(rotationFloats.v);
            rlScalef(transform.scale.x, transform.scale.y, transform.scale.z);

            DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, 2.0f, SKYBLUE);
            DrawCubeWires(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, 2.0f, WHITE);
            DrawCube(Vector3{ 0.0f, 0.0f, 1.25f }, 0.35f, 0.25f, 0.35f, RED);

            rlPopMatrix();
        }
    }
}