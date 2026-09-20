#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


// =============================================================
// SCENE
// =============================================================

uniform sampler2D sceneColorTexture;


// Actual camera-to-fragment distance.
//
// 0 means there is no solid geometry.
uniform sampler2D sceneLinearDepthTexture;


// =============================================================
// ATMOSPHERE LUTS
// =============================================================

uniform sampler2D skyViewTexture;

uniform sampler3D aerialScatteringTexture;

uniform sampler3D aerialTransmittanceTexture;


// =============================================================
// CAMERA / PLANET
// =============================================================

uniform mat4 inverseViewProjection;


uniform vec3 cameraPositionWorld;

uniform vec3 planetCenterWorld;


uniform vec3 sunDirection;


uniform float kmPerWorldUnit;

uniform float bottomRadiusKm;

uniform float topRadiusKm;


const float PI =
    3.14159265359;


// =============================================================
// CAMERA RAY
// =============================================================

vec3 reconstructWorldRay(
    vec2 uv)
{
    vec2 ndc =
        uv *
        2.0 -
        1.0;


    vec4 farPoint =
        inverseViewProjection *
        vec4(
            ndc,
            1.0,
            1.0);


    farPoint /=
        farPoint.w;


    return normalize(
        farPoint.xyz -
        cameraPositionWorld);
}


// =============================================================
// WORLD-SPACE SPHERE INTERSECTION
// =============================================================

bool raySphereIntervalWorld(
    vec3 origin,
    vec3 direction,
    float radius,
    out float tNear,
    out float tFar)
{
    float b =
        dot(
            origin,
            direction);


    float c =
        dot(
            origin,
            origin)
        -
        radius *
        radius;


    float discriminant =
        b *
        b -
        c;


    if (discriminant < 0.0)
    {
        tNear =
            -1.0;

        tFar =
            -1.0;

        return false;
    }


    float root =
        sqrt(
            max(
                discriminant,
                0.0));


    tNear =
        -b -
        root;


    tFar =
        -b +
        root;


    return
        tFar >
        0.0;
}


float nearestSphereIntersectionWorld(
    vec3 origin,
    vec3 direction,
    float radius)
{
    float tNear;

    float tFar;


    if (!raySphereIntervalWorld(
            origin,
            direction,
            radius,
            tNear,
            tFar))
    {
        return
            -1.0;
    }


    if (tNear > 0.000001)
    {
        return
            tNear;
    }


    if (tFar > 0.000001)
    {
        return
            tFar;
    }


    return
        -1.0;
}


// =============================================================
// SKY-VIEW MAPPING
// =============================================================

vec2 unitUvToSubUv(
    vec2 uv,
    vec2 textureSizePixels)
{
    return
        (
            clamp(
                uv,
                vec2(0.0),
                vec2(1.0))
            *
            (
                textureSizePixels -
                vec2(1.0)
            )
            +
            vec2(0.5)
        )
        /
        textureSizePixels;
}


vec2 skyViewLutUv(
    vec3 rayDirection,
    bool intersectsGround,
    vec3 cameraRelativeWorld)
{
    vec3 localUp =
        normalize(
            cameraRelativeWorld);


    float viewHeightKm =
        max(
            length(
                cameraRelativeWorld)
            *
            kmPerWorldUnit,
            bottomRadiusKm +
            0.001);


    float viewZenithCosAngle =
        clamp(
            dot(
                rayDirection,
                localUp),
            -1.0,
            1.0);


    // =========================================================
    // SUN-RELATIVE AZIMUTH
    // =========================================================

    vec3 normalizedSunDirection =
        normalize(
            sunDirection);


    vec3 sunTangent =
        normalizedSunDirection
        -
        localUp
        *
        dot(
            normalizedSunDirection,
            localUp);


    float sunTangentLength =
        length(
            sunTangent);


    if (sunTangentLength < 0.000001)
    {
        // Sun is almost exactly at zenith/nadir.
        //
        // Azimuth is essentially irrelevant in this case, so pick
        // any stable tangent direction.

        vec3 reference =
            abs(
                localUp.y)
            <
            0.99
                ?
                vec3(
                    0.0,
                    1.0,
                    0.0)
                :
                vec3(
                    1.0,
                    0.0,
                    0.0);


        sunTangent =
            normalize(
                cross(
                    reference,
                    localUp));
    }
    else
    {
        sunTangent /=
            sunTangentLength;
    }


    float viewZenithSinAngle =
        sqrt(
            max(
                1.0 -
                viewZenithCosAngle *
                viewZenithCosAngle,
                0.0));


    float lightViewCosAngle =
        1.0;


    if (viewZenithSinAngle > 0.000001)
    {
        vec3 viewTangent =
            (
                rayDirection
                -
                localUp *
                viewZenithCosAngle
            )
            /
            viewZenithSinAngle;


        lightViewCosAngle =
            clamp(
                dot(
                    viewTangent,
                    sunTangent),
                -1.0,
                1.0);
    }


    // =========================================================
    // HORIZON ANGLES
    // =========================================================

    float horizonDistance =
        sqrt(
            max(
                viewHeightKm *
                viewHeightKm
                -
                bottomRadiusKm *
                bottomRadiusKm,
                0.0));


    float cosBeta =
        clamp(
            horizonDistance
            /
            max(
                viewHeightKm,
                0.000001),
            0.0,
            1.0);


    float beta =
        acos(
            cosBeta);


    float zenithHorizonAngle =
        PI -
        beta;


    float viewZenithAngle =
        acos(
            viewZenithCosAngle);


    vec2 uv;


    // =========================================================
    // NONLINEAR V
    // =========================================================

    if (!intersectsGround)
    {
        float coord =
            viewZenithAngle
            /
            max(
                zenithHorizonAngle,
                0.000001);


        coord =
            clamp(
                coord,
                0.0,
                1.0);


        coord =
            1.0 -
            coord;


        coord =
            sqrt(
                max(
                    coord,
                    0.0));


        coord =
            1.0 -
            coord;


        uv.y =
            coord *
            0.5;
    }
    else
    {
        float coord =
            (
                viewZenithAngle
                -
                zenithHorizonAngle
            )
            /
            max(
                beta,
                0.000001);


        coord =
            clamp(
                coord,
                0.0,
                1.0);


        coord =
            sqrt(
                coord);


        uv.y =
            coord *
            0.5
            +
            0.5;
    }


    // =========================================================
    // SUN-RELATIVE U
    // =========================================================

    float azimuthCoord =
        -lightViewCosAngle *
        0.5
        +
        0.5;


    azimuthCoord =
        sqrt(
            clamp(
                azimuthCoord,
                0.0,
                1.0));


    uv.x =
        azimuthCoord;


    return
        unitUvToSubUv(
            uv,
            vec2(
                textureSize(
                    skyViewTexture,
                    0)));
}


// =============================================================
// AERIAL PERSPECTIVE MAPPING
// =============================================================

float maximumAtmospherePathKm()
{
    return
        2.0 *
        sqrt(
            max(
                topRadiusKm *
                topRadiusKm
                -
                bottomRadiusKm *
                bottomRadiusKm,
                0.0));
}


vec3 aerialTextureCoordinate(
    vec2 screenUv,
    float depthParameter)
{
    vec3 textureSizePixels =
        vec3(
            textureSize(
                aerialScatteringTexture,
                0));


    vec3 parameter =
        clamp(
            vec3(
                screenUv,
                depthParameter),
            vec3(0.0),
            vec3(1.0));


    return
        (
            parameter
            *
            (
                textureSizePixels -
                vec3(1.0)
            )
            +
            vec3(0.5)
        )
        /
        textureSizePixels;
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    vec3 sceneColor =
        texture(
            sceneColorTexture,
            vUV).rgb;


    float sceneDistance =
        texture(
            sceneLinearDepthTexture,
            vUV).r;


    vec3 rayDirection =
        reconstructWorldRay(
            vUV);


    vec3 cameraRelativeWorld =
        cameraPositionWorld -
        planetCenterWorld;


    float bottomRadiusWorld =
        bottomRadiusKm /
        kmPerWorldUnit;


    float topRadiusWorld =
        topRadiusKm /
        kmPerWorldUnit;


    // =========================================================
    // FULL-RES ATMOSPHERE INTERSECTION
    // =========================================================
    //
    // This decision happens here at framebuffer resolution.
    //
    // It is no longer baked into the low-resolution Sky-View LUT.

    float atmosphereNear;

    float atmosphereFar;


    if (!raySphereIntervalWorld(
            cameraRelativeWorld,
            rayDirection,
            topRadiusWorld,
            atmosphereNear,
            atmosphereFar))
    {
        outColor =
            vec4(
                sceneColor,
                1.0);

        return;
    }


    float rayStart =
        max(
            atmosphereNear,
            0.0);


    float rayEnd =
        atmosphereFar;


    float groundDistance =
        nearestSphereIntersectionWorld(
            cameraRelativeWorld,
            rayDirection,
            bottomRadiusWorld);


    bool intersectsGround =
        groundDistance > rayStart
        &&
        groundDistance < rayEnd;


    if (intersectsGround)
    {
        rayEnd =
            groundDistance;
    }


    if (rayEnd <= rayStart)
    {
        outColor =
            vec4(
                sceneColor,
                1.0);

        return;
    }


    // =========================================================
    // BACKGROUND / SKY
    // =========================================================

    if (sceneDistance <= 0.0)
    {
        vec2 skyUv =
            skyViewLutUv(
                rayDirection,
                intersectsGround,
                cameraRelativeWorld);


        vec3 skyRadiance =
            textureLod(
                skyViewTexture,
                skyUv,
                0.0).rgb;


        // Background scene color may eventually contain stars.
        //
        // Determine the FULL physical atmospheric path so those
        // stars can be attenuated correctly.
        float fullPathWorld =
            max(
                rayEnd -
                rayStart,
                0.0);


        float fullPathKm =
            fullPathWorld *
            kmPerWorldUnit;


        float fullDepthParameter =
            sqrt(
                clamp(
                    fullPathKm
                    /
                    max(
                        maximumAtmospherePathKm(),
                        0.000001),
                    0.0,
                    1.0));


        vec3 aerialUv =
            aerialTextureCoordinate(
                vUV,
                fullDepthParameter);


        vec3 fullTransmittance =
            textureLod(
                aerialTransmittanceTexture,
                aerialUv,
                0.0).rgb;


        outColor =
            vec4(
                sceneColor
                *
                fullTransmittance
                +
                skyRadiance,
                1.0);


        return;
    }


    // =========================================================
    // GEOMETRY / AERIAL PERSPECTIVE
    // =========================================================

    if (sceneDistance <= rayStart)
    {
        // Geometry exists before the atmosphere starts.
        //
        // Example later:
        // cockpit geometry while the ship is in space.

        outColor =
            vec4(
                sceneColor,
                1.0);


        return;
    }


    float atmosphericDistance =
        min(
            sceneDistance,
            rayEnd);


    float atmosphericPathWorld =
        max(
            atmosphericDistance
            -
            rayStart,
            0.0);


    float atmosphericPathKm =
        atmosphericPathWorld
        *
        kmPerWorldUnit;


    // Aerial Z now represents GLOBAL PHYSICAL PATH LENGTH.
    float depthParameter =
        sqrt(
            clamp(
                atmosphericPathKm
                /
                max(
                    maximumAtmospherePathKm(),
                    0.000001),
                0.0,
                1.0));


    vec3 aerialUv =
        aerialTextureCoordinate(
            vUV,
            depthParameter);


    vec3 scattering =
        textureLod(
            aerialScatteringTexture,
            aerialUv,
            0.0).rgb;


    vec3 transmittance =
        textureLod(
            aerialTransmittanceTexture,
            aerialUv,
            0.0).rgb;


    // =========================================================
    // FINAL COMPOSITE
    // =========================================================

    outColor =
        vec4(
            sceneColor
            *
            transmittance
            +
            scattering,
            1.0);
}