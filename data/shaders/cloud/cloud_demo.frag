#version 450 core

layout(location = 0)
in vec2 vUV;

layout(location = 0)
out vec4 outColor;


// =============================================================
// SCENE
// =============================================================

uniform sampler2D sceneColorTexture;
uniform sampler2D sceneLinearDepthTexture;


// =============================================================
// CLOUD NOISE
// =============================================================

uniform sampler3D baseShapeNoise;


// =============================================================
// CAMERA
// =============================================================

uniform mat4 inverseViewProjection;

uniform vec3 cameraPositionWorld;


// =============================================================
// LIGHTING
// =============================================================

uniform vec3 sunDirection;
uniform vec3 sunRadiance;

uniform float cloudExtinctionPerKm;

uniform vec3 cloudScatteringAlbedo;

uniform float shadowExtinctionMultiplier;

uniform float forwardScatteringG;
uniform float backwardScatteringG;
uniform float forwardScatteringWeight;

uniform float cloudLightingIntensity;


// =============================================================
// EXISTING DEBUG CONTROLS
// =============================================================
//
// F5 -> 0 -> no cloud
// F6 -> 1 -> stratus control
// F7 -> 2 -> cumulus
// F8 -> 3 -> towering / storm

uniform int cloudMorphologyDebugOverride;


// Number keys:
//
// 1 = front
// 2 = 45 degrees
// 3 = side
// 4 = elevated oblique
// 5 = top-ish

uniform int cloudDebugView;


// =============================================================
// QUALITY
// =============================================================

const int CLOUD_SAMPLE_COUNT =
    96;


const int CLOUD_LIGHT_SAMPLE_COUNT =
    8;


const float PI =
    3.14159265358979323846;


// Current renderer convention:
//
// 100 physical metres = 1 render unit.
//
// Therefore:
//
// 1 render unit = 0.1 km.
//
// This demo is intentionally local-camera-space and does NOT use
// the planet's kmPerWorldUnit because the planet may currently be
// using the compact distant-body representation.

const float LOCAL_KM_PER_WORLD_UNIT =
    0.1;


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


    return
        normalize(
            farPoint.xyz -
            cameraPositionWorld);
}


// =============================================================
// RAY / SPHERE INTERSECTION
// =============================================================

bool raySphereIntervalKm(
    vec3 originRelativeToSphereKm,
    vec3 direction,
    float radiusKm,
    out float tNearKm,
    out float tFarKm)
{
    float b =
        dot(
            originRelativeToSphereKm,
            direction);


    float c =
        dot(
            originRelativeToSphereKm,
            originRelativeToSphereKm)
        -
        radiusKm *
        radiusKm;


    float discriminant =
        b *
        b -
        c;


    if (discriminant < 0.0)
    {
        tNearKm =
            -1.0;


        tFarKm =
            -1.0;


        return
            false;
    }


    float root =
        sqrt(
            max(
                discriminant,
                0.0));


    tNearKm =
        -b -
        root;


    tFarKm =
        -b +
        root;


    return
        tFarKm >
        0.0;
}


// =============================================================
// CAMERA-RELATIVE DEMO BASIS
// =============================================================
//
// The demo cloud sits in front of the camera.
//
// We derive an approximate camera basis directly from reconstructed
// rays so we don't need another C++ camera uniform.

void buildDemoBasis(
    out vec3 forward,
    out vec3 right,
    out vec3 up)
{
    forward =
        reconstructWorldRay(
            vec2(
                0.5,
                0.5));


    vec3 upperRay =
        reconstructWorldRay(
            vec2(
                0.5,
                0.70));


    vec3 approximateUp =
        upperRay -
        forward *
        dot(
            upperRay,
            forward);


    if (length(
            approximateUp) <
        0.0001)
    {
        approximateUp =
            vec3(
                0.0,
                1.0,
                0.0);
    }


    up =
        normalize(
            approximateUp);


    right =
        normalize(
            cross(
                forward,
                up));


    up =
        normalize(
            cross(
                right,
                forward));
}


// =============================================================
// DEBUG VIEW ROTATIONS
// =============================================================
//
// Instead of making you fly around the demo cloud, number keys
// rotate the cloud relative to the camera.
//
// That makes morphology iteration very fast.

vec3 rotateAroundY(
    vec3 p,
    float angle)
{
    float c =
        cos(
            angle);


    float s =
        sin(
            angle);


    return
        vec3(
            c * p.x +
                s * p.z,

            p.y,

            -s * p.x +
                c * p.z);
}


vec3 rotateAroundX(
    vec3 p,
    float angle)
{
    float c =
        cos(
            angle);


    float s =
        sin(
            angle);


    return
        vec3(
            p.x,

            c * p.y -
                s * p.z,

            s * p.y +
                c * p.z);
}


vec3 applyDemoViewRotation(
    vec3 p)
{
    if (cloudDebugView == 2)
    {
        return
            rotateAroundY(
                p,
                0.70);
    }


    if (cloudDebugView == 3)
    {
        return
            rotateAroundY(
                p,
                1.57079632679);
    }


    if (cloudDebugView == 4)
    {
        p =
            rotateAroundY(
                p,
                0.55);


        p =
            rotateAroundX(
                p,
                -0.42);


        return
            p;
    }


    if (cloudDebugView == 5)
    {
        return
            rotateAroundX(
                p,
                -1.05);
    }


    return
        p;
}


// =============================================================
// ELLIPSOID ENVELOPE
// =============================================================
//
// These aren't rendered ellipsoids.
//
// They are only a very cheap way of defining the LARGE-SCALE
// volume in which cloud density is permitted.
//
// 3D noise then destroys/carves these shapes.

float ellipsoidEnvelope(
    vec3 p,
    vec3 center,
    vec3 radii,
    float edgeSoftness)
{
    vec3 local =
        (
            p -
            center
        )
        /
        max(
            radii,
            vec3(
                0.001));


    float distanceFromCenter =
        length(
            local);


    return
        1.0 -
        smoothstep(
            1.0 -
                edgeSoftness,
            1.0,
            distanceFromCenter);
}


// =============================================================
// LOW STRATUS ENVELOPE
// =============================================================
//
// This is intentionally flat.
//
// It exists mostly as our control sample:
//
//      _____________________
//   __________________________
// -------------------------------- cloud base

float stratusEnvelope(
    vec3 p)
{
    float envelope =
        0.0;


    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.0,
                    -0.3,
                    0.0),
                vec3(
                    10.5,
                    1.35,
                    7.0),
                0.22));


    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    -5.0,
                    0.0,
                    1.2),
                vec3(
                    7.0,
                    1.1,
                    5.5),
                0.25));


    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    5.0,
                    0.15,
                    -1.0),
                vec3(
                    7.5,
                    1.2,
                    5.5),
                0.25));


    return
        envelope;
}


// =============================================================
// CUMULUS ENVELOPE
// =============================================================
//
// This is deliberately built from overlapping vertically developed
// lobes:
//
//                     ███
//                  ███████
//          ███   █████████
//       ███████████████████
//     ██████████████████████
// --------------------------------
//
// This is the structural thing our planet clouds have been missing.

float cumulusEnvelope(
    vec3 p)
{
    float envelope =
        0.0;


    // Broad lower cloud body.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.0,
                    -2.0,
                    0.0),
                vec3(
                    7.0,
                    2.4,
                    5.3),
                0.26));


    // Left lower lobe.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    -3.2,
                    -0.3,
                    0.4),
                vec3(
                    3.8,
                    3.3,
                    3.7),
                0.28));


    // Main central rising lobe.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.7,
                    0.5,
                    -0.3),
                vec3(
                    4.5,
                    4.0,
                    4.0),
                0.26));


    // Right-hand puff.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    3.8,
                    -0.1,
                    0.8),
                vec3(
                    3.3,
                    2.8,
                    3.0),
                0.30));


    // Upper development.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.8,
                    3.4,
                    -0.2),
                vec3(
                    2.9,
                    3.3,
                    2.8),
                0.27));


    // Small irregular upper-left lobe.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    -1.7,
                    2.7,
                    0.5),
                vec3(
                    2.5,
                    2.6,
                    2.5),
                0.30));


    return
        envelope;
}


// =============================================================
// TOWERING / STORM ENVELOPE
// =============================================================
//
// Big lower cloud body:
//
//          ███████████████
//
// Strong vertical core:
//
//                ███
//             ███████
//           █████████
//         ███████████
//       ███████████████
//
// And a beginning of an anvil near the top.
//
// This is intentionally somewhat exaggerated because this is the
// "hero cloud" prototype.

float stormEnvelope(
    vec3 p)
{
    float envelope =
        0.0;


    // Large dense storm base.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.0,
                    -3.2,
                    0.0),
                vec3(
                    10.5,
                    2.8,
                    7.5),
                0.24));


    // Lower updraft body.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    -0.5,
                    0.0,
                    0.0),
                vec3(
                    7.2,
                    4.6,
                    5.8),
                0.25));


    // Middle tower.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.4,
                    4.0,
                    -0.3),
                vec3(
                    5.5,
                    5.1,
                    4.7),
                0.25));


    // Upper tower.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    0.9,
                    8.0,
                    0.1),
                vec3(
                    4.0,
                    4.5,
                    3.6),
                0.27));


    // Top turret.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    1.0,
                    11.0,
                    -0.2),
                vec3(
                    3.0,
                    3.4,
                    2.8),
                0.30));


    // Beginning of anvil spreading outward.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    1.0,
                    12.4,
                    0.0),
                vec3(
                    8.5,
                    1.45,
                    5.2),
                0.28));


    // Asymmetrical anvil extension.

    envelope =
        max(
            envelope,
            ellipsoidEnvelope(
                p,
                vec3(
                    5.0,
                    12.0,
                    0.3),
                vec3(
                    6.0,
                    1.2,
                    4.2),
                0.32));


    return
        envelope;
}


// =============================================================
// NOISE COORDINATE ROTATION
// =============================================================

mat3 noiseRotation()
{
    return
        mat3(
             0.00,  0.80,  0.60,
            -0.80,  0.36, -0.48,
            -0.60, -0.48,  0.64);
}


// =============================================================
// COARSE 3D SHAPE NOISE
// =============================================================
//
// R is our combined Perlin / Worley cloud-body channel.

float sampleCoarseNoise(
    vec3 p,
    float periodKm)
{
    vec3 uvA =
        p /
        max(
            periodKm,
            0.001);


    uvA +=
        vec3(
            0.173,
            0.417,
            0.731);


    float noiseA =
        textureLod(
            baseShapeNoise,
            uvA,
            0.0).r;


    float secondPeriodKm =
        periodKm *
        1.731;


    vec3 uvB =
        (
            noiseRotation() *
            p
        )
        /
        secondPeriodKm;


    uvB +=
        vec3(
            0.619,
            0.227,
            0.491);


    float noiseB =
        textureLod(
            baseShapeNoise,
            uvB,
            0.0).r;


    return
        mix(
            noiseA,
            noiseB,
            0.35);
}


// =============================================================
// SMALLER-SCALE EROSION NOISE
// =============================================================
//
// B is the Worley/cellular channel generated by the existing
// cloud noise volume.
//
// We mix a little R back in so erosion doesn't become obviously
// cellular.

float sampleErosionNoise(
    vec3 p,
    float periodKm)
{
    vec3 uvA =
        p /
        max(
            periodKm,
            0.001);


    uvA +=
        vec3(
            0.347,
            0.913,
            0.121);


    vec4 sampleA =
        textureLod(
            baseShapeNoise,
            uvA,
            0.0);


    vec3 uvB =
        (
            noiseRotation() *
            p
        )
        /
        (
            periodKm *
            1.413
        );


    uvB +=
        vec3(
            0.781,
            0.359,
            0.563);


    vec4 sampleB =
        textureLod(
            baseShapeNoise,
            uvB,
            0.0);


    float cellular =
        mix(
            sampleA.b,
            sampleB.b,
            0.42);


    float combined =
        mix(
            sampleA.r,
            sampleB.r,
            0.42);


    return
        mix(
            cellular,
            combined,
            0.28);
}


// =============================================================
// CLOUD ENVELOPE FOR PRESET
// =============================================================

float demoEnvelope(
    vec3 localPositionKm,
    int preset)
{
    if (preset == 1)
    {
        return
            stratusEnvelope(
                localPositionKm);
    }


    if (preset == 2)
    {
        return
            cumulusEnvelope(
                localPositionKm);
    }


    if (preset == 3)
    {
        return
            stormEnvelope(
                localPositionKm);
    }


    return
        0.0;
}


// =============================================================
// FULL LOCAL DEMO DENSITY
// =============================================================

float demoDensityLocal(
    vec3 localPositionKm,
    int preset)
{
    localPositionKm =
        applyDemoViewRotation(
            localPositionKm);


    float envelope =
        demoEnvelope(
            localPositionKm,
            preset);


    if (envelope <=
        0.0001)
    {
        return
            0.0;
    }


    float coarsePeriodKm =
        8.0;


    float erosionPeriodKm =
        2.4;


    float erosionStrength =
        0.42;


    float densityMultiplier =
        1.0;


    if (preset == 1)
    {
        coarsePeriodKm =
            12.0;


        erosionPeriodKm =
            3.8;


        erosionStrength =
            0.24;


        densityMultiplier =
            0.82;
    }
    else if (preset == 2)
    {
        coarsePeriodKm =
            7.0;


        erosionPeriodKm =
            2.1;


        erosionStrength =
            0.48;


        densityMultiplier =
            1.05;
    }
    else if (preset == 3)
    {
        coarsePeriodKm =
            9.0;


        erosionPeriodKm =
            2.6;


        erosionStrength =
            0.38;


        densityMultiplier =
            1.20;
    }


    // =========================================================
    // COARSE BODY
    // =========================================================
    //
    // At the center of an envelope the noise threshold is lower.
    //
    // At the outside edge only strong noise survives.
    //
    // This preserves dense cores while making irregular edges.

    float coarseNoise =
        sampleCoarseNoise(
            localPositionKm,
            coarsePeriodKm);


    float envelopeStrength =
        pow(
            clamp(
                envelope,
                0.0,
                1.0),
            0.72);


    float coarseThreshold =
        mix(
            0.72,
            0.27,
            envelopeStrength);


    float density =
        smoothstep(
            coarseThreshold,
            coarseThreshold +
                0.16,
            coarseNoise);


    // Ensure the envelope itself still fades to zero.

    density *=
        smoothstep(
            0.015,
            0.32,
            envelope);


    // =========================================================
    // EDGE EROSION
    // =========================================================
    //
    // Erosion is strongest near the outside of each cloud lobe.
    //
    // Deep cores remain dense.

    float erosionNoise =
        sampleErosionNoise(
            localPositionKm,
            erosionPeriodKm);


    float edgeAmount =
        1.0 -
        smoothstep(
            0.52,
            0.92,
            envelope);


    density -=
        (
            1.0 -
            erosionNoise
        )
        *
        edgeAmount
        *
        erosionStrength;


    // =========================================================
    // DENSER INTERNAL CORE
    // =========================================================

    float internalCore =
        smoothstep(
            0.58,
            0.96,
            envelope);


    if (preset == 2)
    {
        density =
            max(
                density,
                internalCore *
                    0.30);
    }


    if (preset == 3)
    {
        density =
            max(
                density,
                internalCore *
                    0.48);
    }


    density *=
        densityMultiplier;


    return
        clamp(
            density,
            0.0,
            1.5);
}


// =============================================================
// CAMERA-SPACE -> CLOUD-LOCAL
// =============================================================
//
// samplePositionCameraKm:
//
//     camera is at (0,0,0)
//     units are physical kilometres
//
// centerCameraKm:
//
//     cloud position relative to camera
//
// The basis turns that into local cloud coordinates.

float demoDensityAt(
    vec3 samplePositionCameraKm,
    int preset,
    vec3 centerCameraKm,
    vec3 demoForward,
    vec3 demoRight,
    vec3 demoUp)
{
    vec3 relative =
        samplePositionCameraKm -
        centerCameraKm;


    vec3 localPositionKm =
        vec3(
            dot(
                relative,
                demoRight),

            dot(
                relative,
                demoUp),

            dot(
                relative,
                demoForward));


    return
        demoDensityLocal(
            localPositionKm,
            preset);
}


// =============================================================
// HENYEY-GREENSTEIN PHASE
// =============================================================

float henyeyGreenstein(
    float cosineTheta,
    float g)
{
    float gSquared =
        g *
        g;


    float denominator =
        1.0 +
        gSquared -
        2.0 *
        g *
        cosineTheta;


    denominator =
        max(
            denominator,
            0.0001);


    return
        (
            1.0 -
            gSquared
        )
        /
        (
            4.0 *
            PI *
            pow(
                denominator,
                1.5)
        );
}


float cloudPhase(
    vec3 viewRayDirection)
{
    float cosineTheta =
        clamp(
            dot(
                viewRayDirection,
                sunDirection),
            -1.0,
            1.0);


    float forwardPhase =
        henyeyGreenstein(
            cosineTheta,
            forwardScatteringG);


    float backwardPhase =
        henyeyGreenstein(
            cosineTheta,
            backwardScatteringG);


    return
        mix(
            backwardPhase,
            forwardPhase,
            forwardScatteringWeight);
}


// =============================================================
// CLOUD -> SUN SELF SHADOW
// =============================================================

float demoSunTransmittance(
    vec3 samplePositionCameraKm,
    int preset,
    vec3 centerCameraKm,
    vec3 demoForward,
    vec3 demoRight,
    vec3 demoUp)
{
    float shadowDistanceKm =
        20.0;


    if (preset == 3)
    {
        shadowDistanceKm =
            34.0;
    }


    float stepLengthKm =
        shadowDistanceKm /
        float(
            CLOUD_LIGHT_SAMPLE_COUNT);


    float opticalDepth =
        0.0;


    for (int i = 0;
         i < CLOUD_LIGHT_SAMPLE_COUNT;
         ++i)
    {
        float distanceKm =
            (
                float(i) +
                0.5
            )
            *
            stepLengthKm;


        vec3 shadowPositionKm =
            samplePositionCameraKm +
            sunDirection *
                distanceKm;


        float density =
            demoDensityAt(
                shadowPositionKm,
                preset,
                centerCameraKm,
                demoForward,
                demoRight,
                demoUp);


        opticalDepth +=
            density *
            cloudExtinctionPerKm *
            shadowExtinctionMultiplier *
            stepLengthKm;


        if (opticalDepth >
            12.0)
        {
            return
                0.0;
        }
    }


    return
        exp(
            -opticalDepth);
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


    // F5 / normal mode:
    //
    // do nothing.
    //
    // This shader is purely a temporary morphology laboratory.

    int preset =
        cloudMorphologyDebugOverride;


    if (preset <= 0 ||
        preset > 3)
    {
        outColor =
            vec4(
                sceneColor,
                1.0);


        return;
    }


    // =========================================================
    // BUILD CAMERA-RELATIVE CLOUD PLACEMENT
    // =========================================================

    vec3 demoForward;
    vec3 demoRight;
    vec3 demoUp;


    buildDemoBasis(
        demoForward,
        demoRight,
        demoUp);


    float cloudDistanceKm =
        28.0;


    float cloudVerticalOffsetKm =
        0.0;


    float boundingRadiusKm =
        12.0;


    if (preset == 1)
    {
        cloudDistanceKm =
            30.0;


        cloudVerticalOffsetKm =
            0.0;


        boundingRadiusKm =
            12.5;
    }
    else if (preset == 2)
    {
        cloudDistanceKm =
            32.0;


        cloudVerticalOffsetKm =
            -0.5;


        boundingRadiusKm =
            11.0;
    }
    else if (preset == 3)
    {
        cloudDistanceKm =
            48.0;


        cloudVerticalOffsetKm =
            -2.0;


        boundingRadiusKm =
            17.0;
    }


    vec3 centerCameraKm =
        demoForward *
            cloudDistanceKm
        +
        demoUp *
            cloudVerticalOffsetKm;


    // =========================================================
    // CAMERA RAY
    // =========================================================

    vec3 rayDirection =
        reconstructWorldRay(
            vUV);


    float tNearKm;
    float tFarKm;


    if (!raySphereIntervalKm(
            -centerCameraKm,
            rayDirection,
            boundingRadiusKm,
            tNearKm,
            tFarKm))
    {
        outColor =
            vec4(
                sceneColor,
                1.0);


        return;
    }


    float marchStartKm =
        max(
            tNearKm,
            0.0);


    float marchEndKm =
        tFarKm;


    // =========================================================
    // SCENE DEPTH CLIPPING
    // =========================================================
    //
    // Preserve the ship and nearby local geometry.
    //
    // This uses the game's current local renderer scale:
    //
    //     1 render unit = 0.1 km.
    //
    // Point the camera into open space for the cleanest demo.

    float sceneDistanceWorld =
        texture(
            sceneLinearDepthTexture,
            vUV).r;


    if (sceneDistanceWorld >
        0.0)
    {
        float sceneDistanceKm =
            sceneDistanceWorld *
            LOCAL_KM_PER_WORLD_UNIT;


        marchEndKm =
            min(
                marchEndKm,
                sceneDistanceKm);
    }


    if (marchEndKm <=
        marchStartKm)
    {
        outColor =
            vec4(
                sceneColor,
                1.0);


        return;
    }


    // =========================================================
    // PRIMARY CLOUD MARCH
    // =========================================================

    float stepLengthKm =
        (
            marchEndKm -
            marchStartKm
        )
        /
        float(
            CLOUD_SAMPLE_COUNT);


    vec3 accumulatedRadiance =
        vec3(
            0.0);


    float accumulatedTransmittance =
        1.0;


    float phase =
        cloudPhase(
            rayDirection);


    for (int i = 0;
         i < CLOUD_SAMPLE_COUNT;
         ++i)
    {
        float sampleDistanceKm =
            marchStartKm +
            (
                float(i) +
                0.5
            )
            *
            stepLengthKm;


        vec3 samplePositionCameraKm =
            rayDirection *
            sampleDistanceKm;


        float density =
            demoDensityAt(
                samplePositionCameraKm,
                preset,
                centerCameraKm,
                demoForward,
                demoRight,
                demoUp);


        if (density <=
            0.0001)
        {
            continue;
        }


        float opticalDepth =
            cloudExtinctionPerKm *
            density *
            stepLengthKm;


        float stepTransmittance =
            exp(
                -opticalDepth);


        float scatteredFraction =
            1.0 -
            stepTransmittance;


        float sunTransmittance =
            demoSunTransmittance(
                samplePositionCameraKm,
                preset,
                centerCameraKm,
                demoForward,
                demoRight,
                demoUp);


        // =====================================================
        // DIRECT SUN
        // =====================================================

        vec3 directSunlight =
            sunRadiance *
            sunTransmittance *
            phase;


        // =====================================================
        // CHEAP MULTIPLE-SCATTERING / SKY FILL
        // =====================================================
        //
        // This is deliberately tiny.
        //
        // The planetary cloud renderer will eventually get proper
        // atmosphere-coupled fill.
        //
        // For this morphology laboratory we don't want every
        // self-shadowed cloud core to become pitch black.

        float fillStrength =
            0.018;


        if (preset == 3)
        {
            fillStrength =
                0.024;
        }


        vec3 fillLight =
            sunRadiance *
            fillStrength *
            (
                0.45 +
                0.55 *
                sunTransmittance
            );


        // A mild powder-like boost makes dense sun-facing portions
        // read more like cloud volume without changing the shape.

        float powder =
            1.0 -
            exp(
                -density *
                stepLengthKm *
                1.7);


        vec3 scatteredRadiance =
            (
                directSunlight +
                fillLight
            )
            *
            cloudScatteringAlbedo
            *
            cloudLightingIntensity
            *
            (
                1.0 +
                powder *
                0.30
            );


        accumulatedRadiance +=
            accumulatedTransmittance *
            scatteredRadiance *
            scatteredFraction;


        accumulatedTransmittance *=
            stepTransmittance;


        if (accumulatedTransmittance <
            0.001)
        {
            break;
        }
    }


    // =========================================================
    // COMPOSITE
    // =========================================================

    vec3 finalColor =
        accumulatedRadiance
        +
        sceneColor *
        accumulatedTransmittance;


    outColor =
        vec4(
            finalColor,
            1.0);
}