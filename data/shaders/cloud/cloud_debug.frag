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
// CLOUD DATA
// =============================================================

uniform sampler3D baseShapeNoise;
uniform samplerCube weatherMap;


// =============================================================
// CAMERA / PLANET
// =============================================================

uniform mat4 inverseViewProjection;

uniform vec3 cameraPositionWorld;
uniform vec3 planetCenterWorld;

uniform float kmPerWorldUnit;
uniform float planetRadiusKm;


// =============================================================
// CLOUD SHELL
// =============================================================

uniform float cloudInnerRadiusKm;
uniform float cloudOuterRadiusKm;
uniform float cloudBoundaryFadeKm;


// =============================================================
// CLOUD PARAMETERS
// =============================================================

uniform float cloudCoverage;

uniform float coarseShapePeriodKm;
uniform float baseShapePeriodKm;

uniform float cloudDensityMultiplier;


// =============================================================
// DISTANCE LOD
// =============================================================

uniform float localShapeFadeStartFootprintKm;
uniform float localShapeFadeEndFootprintKm;


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
// DEBUG MORPHOLOGY
// =============================================================
//
// F5 = 0
//     real continuous weather morphology
//
// F6 = 1
//     isolated stratus
//
// F7 = 2
//     isolated true-3D cumulus
//
// F8 = 3
//     isolated true-3D storm

uniform int cloudMorphologyDebugOverride;


// =============================================================
// QUALITY
// =============================================================
//
// F5:
//
//     adaptive physical stepping
//     maximum 128 intervals
//
// Debug labs:
//
//     fixed 64 intervals
//
// The important distinction:
//
// OLD:
//
//     entire shell distance / 64
//
// NEW:
//
//     physical km step
//
// with a maximum-step safety constraint.

const int CLOUD_MAX_VIEW_STEPS = 128;
const int CLOUD_DEBUG_VIEW_STEPS = 64;

const int CLOUD_LIGHT_SAMPLE_COUNT = 6;

const float PI = 3.14159265358979323846;


// =============================================================
// BASIC HELPERS
// =============================================================

float remap01(
    float value,
    float minimum,
    float maximum)
{
    return clamp(
        (value - minimum) /
        max(
            maximum - minimum,
            0.0001),
        0.0,
        1.0);
}


float softUnion(
    float a,
    float b)
{
    a =
        clamp(
            a,
            0.0,
            1.0);


    b =
        clamp(
            b,
            0.0,
            1.0);


    return
        1.0 -
        (1.0 - a) *
        (1.0 - b);
}


mat3 shapeRotation()
{
    return mat3(
         0.00,  0.80,  0.60,
        -0.80,  0.36, -0.48,
        -0.60, -0.48,  0.64);
}


// =============================================================
// HASH / JITTER
// =============================================================
//
// Static spatial jitter for now.
//
// This is NOT temporal reprojection.
//
// Its job is simply to stop all neighbouring pixels from sampling
// the cloud volume at exactly the same positions along their rays.
//
// Without this, fixed marching intervals produce visible:
//
//     rings
//     stripes
//     contour bands
//
// Later temporal reconstruction will clean up the resulting
// fine-grained noise.

float hash12(
    vec2 p)
{
    vec3 p3 =
        fract(
            vec3(
                p.x,
                p.y,
                p.x)
            *
            0.1031);


    p3 +=
        dot(
            p3,
            p3.yzx +
            33.33);


    return fract(
        (p3.x + p3.y) *
        p3.z);
}


// =============================================================
// F5 PHYSICAL VIEW STEP
// =============================================================
//
// Distance from camera:
//
// near:
//     ~1.25 km
//
// medium:
//     approaches ~4.5 km
//
// far:
//     approaches ~22 km
//
// The transition is continuous.
//
// These are renderer sampling distances,
// NOT cloud physical scales.

float adaptiveViewStepLengthKm(
    float distanceFromCameraKm)
{
    float nearToMedium =
        smoothstep(
            35.0,
            120.0,
            distanceFromCameraKm);


    float mediumToFar =
        smoothstep(
            220.0,
            700.0,
            distanceFromCameraKm);


    float stepLengthKm =
        mix(
            1.25,
            4.5,
            nearToMedium);


    stepLengthKm =
        mix(
            stepLengthKm,
            22.0,
            mediumToFar);


    return
        stepLengthKm;
}


// =============================================================
// F5 LOCAL DETAIL WEIGHT
// =============================================================
//
// This answers:
//
// "Can this ray actually resolve small volumetric cloud features?"
//
// Detail disappears for two separate reasons:
//
// 1. the sample is physically far from the camera
// 2. the current ray step is too large to resolve the feature
//
// This prevents us from sampling 4 km erosion when the marcher is
// jumping 15-20 km at a time.

float localDensityDetailWeight(
    float sampleDistanceKm,
    float stepLengthKm)
{
    float distanceWeight =
        1.0 -
        smoothstep(
            140.0,
            500.0,
            sampleDistanceKm);


    float stepWeight =
        1.0 -
        smoothstep(
            3.0,
            12.0,
            stepLengthKm);


    return
        clamp(
            min(
                distanceWeight,
                stepWeight),
            0.0,
            1.0);
}


// =============================================================
// CAMERA RAY
// =============================================================

vec3 reconstructWorldRay(
    vec2 uv)
{
    vec2 ndc =
        uv * 2.0 - 1.0;


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
// RAY / SPHERE
// =============================================================

bool raySphereIntervalKm(
    vec3 originKm,
    vec3 direction,
    float radiusKm,
    out float tNearKm,
    out float tFarKm)
{
    float b =
        dot(
            originKm,
            direction);


    float c =
        dot(
            originKm,
            originKm)
        -
        radiusKm *
        radiusKm;


    float discriminant =
        b * b - c;


    if (discriminant < 0.0)
    {
        tNearKm = -1.0;
        tFarKm = -1.0;

        return false;
    }


    float root =
        sqrt(
            max(
                discriminant,
                0.0));


    tNearKm =
        -b - root;


    tFarKm =
        -b + root;


    return
        tFarKm > 0.0;
}


// =============================================================
// PLANET SHADOW
// =============================================================

bool planetBlocksSun(
    vec3 samplePositionKm)
{
    vec3 origin =
        samplePositionKm +
        sunDirection * 0.01;


    float tNearKm;
    float tFarKm;


    if (!raySphereIntervalKm(
            origin,
            sunDirection,
            planetRadiusKm,
            tNearKm,
            tFarKm))
    {
        return false;
    }


    return
        tNearKm > 0.0;
}


// =============================================================
// CLOUD SHELL
// =============================================================

int buildCloudShellSegments(
    vec3 originKm,
    vec3 direction,
    out vec2 segmentA,
    out vec2 segmentB)
{
    segmentA =
        vec2(0.0);


    segmentB =
        vec2(0.0);


    float outerNearKm;
    float outerFarKm;


    if (!raySphereIntervalKm(
            originKm,
            direction,
            cloudOuterRadiusKm,
            outerNearKm,
            outerFarKm))
    {
        return 0;
    }


    float shellStartKm =
        max(
            outerNearKm,
            0.0);


    float shellEndKm =
        outerFarKm;


    if (shellEndKm <=
        shellStartKm)
    {
        return 0;
    }


    float innerNearKm;
    float innerFarKm;


    if (!raySphereIntervalKm(
            originKm,
            direction,
            cloudInnerRadiusKm,
            innerNearKm,
            innerFarKm))
    {
        segmentA =
            vec2(
                shellStartKm,
                shellEndKm);


        return 1;
    }


    float cutStartKm =
        max(
            shellStartKm,
            innerNearKm);


    float cutEndKm =
        min(
            shellEndKm,
            innerFarKm);


    if (cutEndKm <=
        cutStartKm)
    {
        segmentA =
            vec2(
                shellStartKm,
                shellEndKm);


        return 1;
    }


    int count =
        0;


    if (cutStartKm >
        shellStartKm)
    {
        segmentA =
            vec2(
                shellStartKm,
                cutStartKm);


        count =
            1;
    }


    if (cutEndKm <
        shellEndKm)
    {
        if (count == 0)
        {
            segmentA =
                vec2(
                    cutEndKm,
                    shellEndKm);
        }
        else
        {
            segmentB =
                vec2(
                    cutEndKm,
                    shellEndKm);
        }


        count +=
            1;
    }


    return count;
}


// =============================================================
// WEATHER
// =============================================================

vec4 sampleWeather(
    vec3 samplePositionKm)
{
    return textureLod(
        weatherMap,
        normalize(
            samplePositionKm),
        0.0);
}


float calculateLocalCoverage(
    vec4 weather)
{
    float coverageBias =
        (cloudCoverage - 0.5) *
        0.85;


    return clamp(
        weather.r +
        coverageBias,
        0.0,
        1.0);
}


// =============================================================
// CONTINUOUS WEATHER MORPHOLOGY
// =============================================================
//
// R = coverage
// G = stratiform -> convective
// B = storminess
// A = vertical development

vec3 weatherMorphologyControls(
    vec4 weather)
{
    float convective =
        smoothstep(
            0.24,
            0.72,
            weather.g);


    float storm =
        smoothstep(
            0.56,
            0.88,
            weather.b);


    storm *=
        smoothstep(
            0.18,
            0.72,
            convective +
            weather.a * 0.35);


    float verticalDevelopment =
        clamp(
            max(
                weather.a,
                storm),
            0.0,
            1.0);


    return vec3(
        convective,
        storm,
        verticalDevelopment);
}


// =============================================================
// DEBUG / LIGHTING FAMILY WEIGHTS
// =============================================================

vec3 cloudTypeWeights(
    vec4 weather)
{
    if (cloudMorphologyDebugOverride == 1)
    {
        return vec3(
            1.0,
            0.0,
            0.0);
    }


    if (cloudMorphologyDebugOverride == 2)
    {
        return vec3(
            0.0,
            1.0,
            0.0);
    }


    if (cloudMorphologyDebugOverride == 3)
    {
        return vec3(
            0.0,
            0.0,
            1.0);
    }


    vec3 controls =
        weatherMorphologyControls(
            weather);


    float convective =
        controls.x;


    float storm =
        controls.y;


    float stratusWeight =
        1.0 -
        convective;


    float stormWeight =
        convective *
        storm;


    float cumulusWeight =
        convective *
        (1.0 - storm);


    vec3 weights =
        vec3(
            stratusWeight,
            cumulusWeight,
            stormWeight);


    return
        weights /
        max(
            weights.x +
            weights.y +
            weights.z,
            0.0001);
}


// =============================================================
// CLOUD HEIGHT
// =============================================================

float normalizedCloudHeight(
    float radiusKm)
{
    return clamp(
        (radiusKm - cloudInnerRadiusKm) /
        max(
            cloudOuterRadiusKm -
            cloudInnerRadiusKm,
            0.0001),
        0.0,
        1.0);
}


// =============================================================
// SCREEN FOOTPRINT
// =============================================================

float pixelFootprintKm(
    float sampleDistanceKm,
    float rayAngularFootprint)
{
    return max(
        sampleDistanceKm *
        rayAngularFootprint,
        0.0001);
}


// =============================================================
// NOISE LOD
// =============================================================

float noiseLod(
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    float physicalPeriodKm)
{
    float pixelSizeKm =
        pixelFootprintKm(
            sampleDistanceKm,
            rayAngularFootprint);


    float samplingFootprintKm =
        max(
            pixelSizeKm,
            stepLengthKm);


    ivec3 resolution =
        textureSize(
            baseShapeNoise,
            0);


    float voxelSizeKm =
        physicalPeriodKm /
        float(
            max(
                resolution.x,
                1));


    float lod =
        log2(
            max(
                samplingFootprintKm /
                max(
                    voxelSizeKm,
                    0.0001),
                1.0));


    float maximumLod =
        float(
            max(
                textureQueryLevels(
                    baseShapeNoise)
                -
                1,
                0));


    return clamp(
        lod,
        0.0,
        maximumLod);
}


// =============================================================
// LOCAL TANGENT BASIS
// =============================================================

void buildLocalTangents(
    vec3 radialDirection,
    out vec3 tangentA,
    out vec3 tangentB)
{
    tangentA =
        shapeRotation() *
        radialDirection;


    tangentA -=
        radialDirection *
        dot(
            tangentA,
            radialDirection);


    if (length(tangentA) <
        0.0001)
    {
        vec3 fallback =
            abs(radialDirection.y) <
            0.95
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


        tangentA =
            cross(
                fallback,
                radialDirection);
    }


    tangentA =
        normalize(
            tangentA);


    tangentB =
        normalize(
            cross(
                radialDirection,
                tangentA));
}


// =============================================================
// REUSABLE 3D FIELD
// =============================================================

float sample3DField(
    vec3 positionKm,
    float physicalPeriodKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    vec3 offsetA,
    vec3 offsetB)
{
    float lodA =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            physicalPeriodKm);


    vec3 uvA =
        positionKm /
        physicalPeriodKm;


    uvA +=
        offsetA;


    float a =
        textureLod(
            baseShapeNoise,
            uvA,
            lodA).r;


    float secondPeriodKm =
        physicalPeriodKm *
        1.577;


    float lodB =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            secondPeriodKm);


    vec3 uvB =
        (
            shapeRotation() *
            positionKm
        )
        /
        secondPeriodKm;


    uvB +=
        offsetB;


    float b =
        textureLod(
            baseShapeNoise,
            uvB,
            lodB).r;


    return mix(
        a,
        b,
        0.34);
}


// =============================================================
// =============================================================
// ISOLATED STRATUS — F6
// =============================================================
// =============================================================

float sampleStratusDensity(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    vec4 weather,
    float coverage)
{
    if (coverage <=
        0.01)
    {
        return 0.0;
    }


    float radiusKm =
        length(
            samplePositionKm);


    float height =
        normalizedCloudHeight(
            radiusKm);


    float topHeight =
        mix(
            0.16,
            0.27,
            weather.a);


    float bottomFade =
        smoothstep(
            0.0,
            0.045,
            height);


    float topFade =
        1.0 -
        smoothstep(
            max(
                topHeight -
                0.08,
                0.04),
            topHeight,
            height);


    float heightProfile =
        bottomFade *
        topFade;


    if (heightProfile <=
        0.0001)
    {
        return 0.0;
    }


    vec3 radialDirection =
        normalize(
            samplePositionKm);


    float altitudeKm =
        max(
            radiusKm -
            cloudInnerRadiusKm,
            0.0);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    vec3 shapedPositionKm =
        basePositionKm
        +
        radialDirection *
        altitudeKm *
        3.5;


    float field =
        sample3DField(
            shapedPositionKm,
            coarseShapePeriodKm *
            1.35,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.31,
                0.67,
                0.19),
            vec3(
                0.73,
                0.17,
                0.43));


    float threshold =
        mix(
            0.72,
            0.47,
            coverage);


    float body =
        smoothstep(
            threshold,
            threshold +
            0.14,
            field);


    float weatherMask =
        smoothstep(
            0.10,
            0.45,
            coverage);


    body *=
        weatherMask;


    return clamp(
        body *
        heightProfile *
        cloudDensityMultiplier,
        0.0,
        1.5);
}


// =============================================================
// =============================================================
// ISOLATED TRUE-3D CUMULUS — F7
// =============================================================
// =============================================================

float sampleCumulusCellStrength(
    vec3 samplePositionKm,
    float coverage)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    const float cellSpacingKm =
        16.0;


    vec3 uvA =
        basePositionKm /
        cellSpacingKm
        +
        vec3(
            0.173,
            0.417,
            0.731);


    float a =
        textureLod(
            baseShapeNoise,
            uvA,
            0.0).r;


    vec3 uvB =
        (
            shapeRotation() *
            basePositionKm
        )
        /
        (
            cellSpacingKm *
            1.727
        )
        +
        vec3(
            0.619,
            0.227,
            0.491);


    float b =
        textureLod(
            baseShapeNoise,
            uvB,
            0.0).r;


    float field =
        mix(
            a,
            b,
            0.38);


    float threshold =
        mix(
            0.55,
            0.40,
            coverage);


    return smoothstep(
        threshold,
        threshold +
        0.25,
        field);
}


vec3 cumulusVolumePosition(
    vec3 samplePositionKm,
    float cellStrength)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    float radiusKm =
        length(
            samplePositionKm);


    float altitudeKm =
        max(
            radiusKm -
            cloudInnerRadiusKm,
            0.0);


    float height =
        normalizedCloudHeight(
            radiusKm);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    vec3 tangentA;
    vec3 tangentB;


    buildLocalTangents(
        radialDirection,
        tangentA,
        tangentB);


    float driftA =
        textureLod(
            baseShapeNoise,
            basePositionKm /
            52.0
            +
            vec3(
                0.27,
                0.61,
                0.11),
            0.0).g
        *
        2.0
        -
        1.0;


    float driftB =
        textureLod(
            baseShapeNoise,
            (
                shapeRotation() *
                basePositionKm
            )
            /
            61.0
            +
            vec3(
                0.73,
                0.19,
                0.47),
            0.0).g
        *
        2.0
        -
        1.0;


    float driftDistanceKm =
        height *
        mix(
            1.5,
            4.5,
            cellStrength);


    vec3 horizontalDrift =
        tangentA *
        driftA *
        driftDistanceKm

        +

        tangentB *
        driftB *
        driftDistanceKm;


    return
        basePositionKm

        +

        radialDirection *
        altitudeKm *
        1.70

        +

        horizontalDrift;
}


float sampleCumulusErosion(
    vec3 volumePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm)
{
    const float periodKm =
        4.0;


    float lod =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            periodKm);


    vec4 a =
        textureLod(
            baseShapeNoise,
            volumePositionKm /
            periodKm
            +
            vec3(
                0.347,
                0.913,
                0.121),
            lod);


    vec4 b =
        textureLod(
            baseShapeNoise,
            (
                shapeRotation() *
                volumePositionKm
            )
            /
            (
                periodKm *
                1.419
            )
            +
            vec3(
                0.781,
                0.359,
                0.563),
            lod);


    float worley =
        mix(
            a.b,
            b.b,
            0.42);


    float combined =
        mix(
            a.r,
            b.r,
            0.42);


    return mix(
        worley,
        combined,
        0.25);
}


float sampleCumulusDensity(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    vec4 weather,
    float coverage)
{
    if (cloudMorphologyDebugOverride == 2)
    {
        coverage =
            max(
                coverage,
                0.60);
    }


    if (coverage <=
        0.03)
    {
        return 0.0;
    }


    float radiusKm =
        length(
            samplePositionKm);


    float height =
        normalizedCloudHeight(
            radiusKm);


    float cellStrength =
        sampleCumulusCellStrength(
            samplePositionKm,
            coverage);


    float cellGate =
        smoothstep(
            0.16,
            0.38,
            cellStrength);


    if (cellGate <=
        0.0001)
    {
        return 0.0;
    }


    float verticalPotential =
        cloudMorphologyDebugOverride == 2
            ?
            0.74
            :
            mix(
                0.48,
                0.74,
                weather.a);


    float bottomFade =
        smoothstep(
            0.0,
            0.040,
            height);


    float topFade =
        1.0 -
        smoothstep(
            max(
                verticalPotential -
                0.14,
                0.20),
            verticalPotential,
            height);


    if (topFade <=
        0.0001)
    {
        return 0.0;
    }


    vec3 volumePositionKm =
        cumulusVolumePosition(
            samplePositionKm,
            cellStrength);


    float macroField =
        sample3DField(
            volumePositionKm,
            18.0,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.137,
                0.593,
                0.317),
            vec3(
                0.719,
                0.263,
                0.881));


    float lobeField =
        sample3DField(
            volumePositionKm,
            9.5,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.413,
                0.827,
                0.239),
            vec3(
                0.191,
                0.533,
                0.761));


    float field =
        macroField *
        0.72
        +
        lobeField *
        0.28;


    float heightPenalty =
        height *
        0.06
        +
        height *
        height *
        0.12;


    float convectiveSupport =
        cellStrength *
        0.16;


    float coverageSupport =
        max(
            coverage -
            0.45,
            0.0)
        *
        0.10;


    float threshold =
        0.51
        +
        heightPenalty
        -
        convectiveSupport
        -
        coverageSupport;


    float body =
        smoothstep(
            threshold,
            threshold +
            0.13,
            field);


    body *=
        cellGate;


    if (body <=
        0.0001)
    {
        return 0.0;
    }


    float erosion =
        sampleCumulusErosion(
            volumePositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm);


    float edgeAmount =
        1.0 -
        smoothstep(
            0.42,
            0.88,
            body);


    body -=
        (1.0 - erosion)
        *
        edgeAmount
        *
        0.34;


    body =
        max(
            body,
            0.0);


    body =
        remap01(
            body,
            0.055,
            0.92);


    body =
        pow(
            body,
            1.22);


    return clamp(
        body *
        bottomFade *
        topFade *
        cloudDensityMultiplier,
        0.0,
        1.5);
}


// =============================================================
// =============================================================
// TRUE-3D STORM SUPPORT
// =============================================================
// =============================================================

vec3 sampleStormSystemData(
    vec3 samplePositionKm,
    float coverage,
    float stormPotential)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    const float systemSpacingKm =
        56.0;


    vec4 a =
        textureLod(
            baseShapeNoise,
            basePositionKm /
            systemSpacingKm
            +
            vec3(
                0.193,
                0.487,
                0.829),
            0.0);


    vec4 b =
        textureLod(
            baseShapeNoise,
            (
                shapeRotation() *
                basePositionKm
            )
            /
            (
                systemSpacingKm *
                1.683
            )
            +
            vec3(
                0.647,
                0.281,
                0.413),
            0.0);


    float systemNoise =
        mix(
            a.r,
            b.r,
            0.38);


    float threshold =
        mix(
            0.56,
            0.43,
            coverage);


    threshold -=
        stormPotential *
        0.06;


    float strength =
        smoothstep(
            threshold,
            threshold +
            0.23,
            systemNoise);


    strength *=
        mix(
            0.60,
            1.0,
            stormPotential);


    float driftA =
        mix(
            a.g,
            b.b,
            0.35)
        *
        2.0
        -
        1.0;


    float driftB =
        mix(
            a.b,
            b.g,
            0.35)
        *
        2.0
        -
        1.0;


    return vec3(
        strength,
        driftA,
        driftB);
}


vec3 stormVolumePosition(
    vec3 samplePositionKm,
    vec3 systemData,
    float verticalScale,
    float driftScaleKm)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    float radiusKm =
        length(
            samplePositionKm);


    float altitudeKm =
        max(
            radiusKm -
            cloudInnerRadiusKm,
            0.0);


    float height =
        normalizedCloudHeight(
            radiusKm);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    vec3 tangentA;
    vec3 tangentB;


    buildLocalTangents(
        radialDirection,
        tangentA,
        tangentB);


    float driftDistanceKm =
        height *
        driftScaleKm *
        mix(
            0.55,
            1.0,
            systemData.x);


    vec3 horizontalDrift =
        tangentA *
        systemData.y *
        driftDistanceKm

        +

        tangentB *
        systemData.z *
        driftDistanceKm;


    return
        basePositionKm

        +

        radialDirection *
        altitudeKm *
        verticalScale

        +

        horizontalDrift;
}


float sampleStormErosion(
    vec3 volumePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm)
{
    const float periodKm =
        4.5;


    float lod =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            periodKm);


    vec4 a =
        textureLod(
            baseShapeNoise,
            volumePositionKm /
            periodKm
            +
            vec3(
                0.417,
                0.117,
                0.893),
            lod);


    vec4 b =
        textureLod(
            baseShapeNoise,
            (
                shapeRotation() *
                volumePositionKm
            )
            /
            (
                periodKm *
                1.431
            )
            +
            vec3(
                0.743,
                0.527,
                0.181),
            lod);


    float cellular =
        mix(
            a.b,
            b.b,
            0.40);


    float combined =
        mix(
            a.r,
            b.r,
            0.40);


    return mix(
        cellular,
        combined,
        0.24);
}


// =============================================================
// ISOLATED F8 STORM
// =============================================================

float sampleStormDensity(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    vec4 weather,
    float coverage)
{
    float stormPotential =
        max(
            weather.b,
            weather.a *
            0.55);


    if (cloudMorphologyDebugOverride == 3)
    {
        coverage =
            max(
                coverage,
                0.55);


        stormPotential =
            1.0;
    }


    if (coverage <=
        0.03)
    {
        return 0.0;
    }


    float radiusKm =
        length(
            samplePositionKm);


    float height =
        normalizedCloudHeight(
            radiusKm);


    float bottomFade =
        smoothstep(
            0.0,
            0.035,
            height);


    float stormTop =
        cloudMorphologyDebugOverride == 3
            ?
            1.0
            :
            mix(
                0.78,
                1.0,
                max(
                    weather.a,
                    weather.b));


    float topFade =
        1.0 -
        smoothstep(
            max(
                stormTop -
                0.10,
                0.60),
            stormTop,
            height);


    if (topFade <=
        0.0001)
    {
        return 0.0;
    }


    vec3 systemData =
        sampleStormSystemData(
            samplePositionKm,
            coverage,
            stormPotential);


    float systemStrength =
        systemData.x;


    float systemGate =
        smoothstep(
            0.14,
            0.34,
            systemStrength);


    if (systemGate <=
        0.0001)
    {
        return 0.0;
    }


    // ---------------------------------------------------------
    // LOWER BODY
    // ---------------------------------------------------------

    vec3 lowerPositionKm =
        stormVolumePosition(
            samplePositionKm,
            systemData,
            1.45,
            3.0);


    float lowerField =
        sample3DField(
            lowerPositionKm,
            34.0,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.127,
                0.683,
                0.359),
            vec3(
                0.829,
                0.211,
                0.547));


    float lowerThreshold =
        0.50
        +
        height *
        0.08
        -
        systemStrength *
        0.13
        -
        stormPotential *
        0.04;


    float lowerBody =
        smoothstep(
            lowerThreshold,
            lowerThreshold +
            0.13,
            lowerField);


    lowerBody *=
        1.0 -
        smoothstep(
            0.50,
            0.78,
            height);


    lowerBody *=
        systemGate;


    // ---------------------------------------------------------
    // TOWER
    // ---------------------------------------------------------

    vec3 towerPositionKm =
        stormVolumePosition(
            samplePositionKm,
            systemData,
            2.55,
            5.5);


    float towerField =
        sample3DField(
            towerPositionKm,
            17.0,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.331,
                0.817,
                0.143),
            vec3(
                0.691,
                0.373,
                0.937));


    float towerHeightPenalty =
        height *
        0.055
        +
        height *
        height *
        0.105;


    float towerThreshold =
        0.52
        +
        towerHeightPenalty
        -
        systemStrength *
        0.19
        -
        stormPotential *
        0.055;


    float towerBody =
        smoothstep(
            towerThreshold,
            towerThreshold +
            0.12,
            towerField);


    towerBody *=
        systemGate;


    towerBody *=
        mix(
            0.72,
            1.15,
            systemStrength);


    // ---------------------------------------------------------
    // ANVIL
    // ---------------------------------------------------------

    vec3 anvilPositionKm =
        stormVolumePosition(
            samplePositionKm,
            systemData,
            1.85,
            10.0);


    float anvilField =
        sample3DField(
            anvilPositionKm,
            25.0,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.557,
                0.197,
                0.773),
            vec3(
                0.239,
                0.911,
                0.419));


    float upperBand =
        smoothstep(
            0.68,
            0.79,
            height)
        *
        (
            1.0 -
            smoothstep(
                0.97,
                1.0,
                height)
        );


    float anvilStrength =
        smoothstep(
            0.55,
            0.82,
            systemStrength)
        *
        smoothstep(
            0.50,
            0.82,
            stormPotential);


    float distanceFromAnvilCenter =
        abs(
            height -
            0.87);


    float anvilThreshold =
        0.51
        +
        distanceFromAnvilCenter *
        0.16
        -
        systemStrength *
        0.10;


    float anvilBody =
        smoothstep(
            anvilThreshold,
            anvilThreshold +
            0.13,
            anvilField);


    anvilBody *=
        upperBand *
        anvilStrength;


    float body =
        softUnion(
            lowerBody,
            towerBody);


    body =
        softUnion(
            body,
            anvilBody);


    if (body <=
        0.0001)
    {
        return 0.0;
    }


    vec3 erosionPositionKm =
        mix(
            lowerPositionKm,
            towerPositionKm,
            smoothstep(
                0.22,
                0.68,
                height));


    erosionPositionKm =
        mix(
            erosionPositionKm,
            anvilPositionKm,
            smoothstep(
                0.72,
                0.92,
                height));


    float erosion =
        sampleStormErosion(
            erosionPositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm);


    float edgeAmount =
        1.0 -
        smoothstep(
            0.46,
            0.90,
            body);


    body -=
        (1.0 - erosion)
        *
        edgeAmount
        *
        0.29;


    body =
        max(
            body,
            0.0);


    body =
        remap01(
            body,
            0.045,
            0.94);


    body =
        pow(
            body,
            1.12);


    return clamp(
        body *
        bottomFade *
        topFade *
        cloudDensityMultiplier *
        1.08,
        0.0,
        1.5);
}


// =============================================================
// =============================================================
// F5 SHARED WEATHER MORPHOLOGY
// =============================================================
// =============================================================

// =============================================================
// SHARED CONVECTIVE CELL DATA
// =============================================================
//
// x = cell strength
// y = drift A
// z = drift B

vec3 sampleSharedConvectiveData(
    vec3 samplePositionKm,
    float coverage)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    const float cellScaleKm =
        21.0;


    vec4 a =
        textureLod(
            baseShapeNoise,
            basePositionKm /
            cellScaleKm
            +
            vec3(
                0.157,
                0.439,
                0.811),
            0.0);


    vec4 b =
        textureLod(
            baseShapeNoise,
            (
                shapeRotation() *
                basePositionKm
            )
            /
            (
                cellScaleKm *
                1.691
            )
            +
            vec3(
                0.673,
                0.251,
                0.527),
            0.0);


    float cellNoise =
        mix(
            a.r,
            b.r,
            0.38);


    float threshold =
        mix(
            0.57,
            0.41,
            coverage);


    float strength =
        smoothstep(
            threshold,
            threshold +
            0.25,
            cellNoise);


    float driftA =
        mix(
            a.g,
            b.b,
            0.36)
        *
        2.0
        -
        1.0;


    float driftB =
        mix(
            a.b,
            b.g,
            0.36)
        *
        2.0
        -
        1.0;


    return vec3(
        strength,
        driftA,
        driftB);
}


// =============================================================
// SHARED WEATHER VOLUME POSITION
// =============================================================

vec3 sharedWeatherVolumePosition(
    vec3 samplePositionKm,
    vec3 cellData,
    float convective,
    float storm)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    float radiusKm =
        length(
            samplePositionKm);


    float altitudeKm =
        max(
            radiusKm -
            cloudInnerRadiusKm,
            0.0);


    float height =
        normalizedCloudHeight(
            radiusKm);


    vec3 basePositionKm =
        radialDirection *
        cloudInnerRadiusKm;


    vec3 tangentA;
    vec3 tangentB;


    buildLocalTangents(
        radialDirection,
        tangentA,
        tangentB);


    float driftScaleKm =
        mix(
            0.4,
            4.5,
            convective);


    driftScaleKm =
        mix(
            driftScaleKm,
            7.0,
            storm);


    float driftDistanceKm =
        height *
        driftScaleKm *
        mix(
            0.55,
            1.0,
            cellData.x);


    vec3 horizontalDrift =
        tangentA *
        cellData.y *
        driftDistanceKm

        +

        tangentB *
        cellData.z *
        driftDistanceKm;


    float verticalScale =
        mix(
            3.2,
            1.75,
            convective);


    verticalScale =
        mix(
            verticalScale,
            1.60,
            storm);


    return
        basePositionKm

        +

        radialDirection *
        altitudeKm *
        verticalScale

        +

        horizontalDrift;
}


// =============================================================
// SHARED EROSION
// =============================================================

float sampleSharedErosion(
    vec3 volumePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm)
{
    const float periodKm =
        4.5;


    float lod =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            periodKm);


    vec4 a =
        textureLod(
            baseShapeNoise,
            volumePositionKm /
            periodKm
            +
            vec3(
                0.383,
                0.907,
                0.149),
            lod);


    vec4 b =
        textureLod(
            baseShapeNoise,
            (
                shapeRotation() *
                volumePositionKm
            )
            /
            (
                periodKm *
                1.437
            )
            +
            vec3(
                0.769,
                0.337,
                0.581),
            lod);


    float cellular =
        mix(
            a.b,
            b.b,
            0.42);


    float combined =
        mix(
            a.r,
            b.r,
            0.42);


    return mix(
        cellular,
        combined,
        0.24);
}


// =============================================================
// F5 WEATHER-DRIVEN DENSITY
// =============================================================

float sampleWeatherDrivenDensity(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    vec4 weather,
    float coverage)
{
    if (coverage <=
        0.01)
    {
        return 0.0;
    }


    vec3 controls =
        weatherMorphologyControls(
            weather);


    float convective =
        controls.x;


    float storm =
        controls.y;


    float verticalDevelopment =
        controls.z;


    // =========================================================
    // DISTANCE DETAIL LOD
    // =========================================================

    float detailWeight =
        localDensityDetailWeight(
            sampleDistanceKm,
            stepLengthKm);


    // =========================================================
    // HEIGHT
    // =========================================================

    float radiusKm =
        length(
            samplePositionKm);


    float height =
        normalizedCloudHeight(
            radiusKm);


    // =========================================================
    // WEATHER EXISTENCE
    // =========================================================

    float weatherMask =
        smoothstep(
            0.08,
            0.42,
            coverage);


    if (weatherMask <=
        0.0001)
    {
        return 0.0;
    }


    // =========================================================
    // LOCAL CONVECTIVE DATA
    // =========================================================

    vec3 cellData =
        sampleSharedConvectiveData(
            samplePositionKm,
            coverage);


    float cellStrength =
        cellData.x;


    float cellGate =
        smoothstep(
            0.14,
            0.40,
            cellStrength);


    // =========================================================
    // DISTANCE-AWARE BREAKUP
    // =========================================================
    //
    // Nearby:
    //
    //     fully carve the broad weather field into individual
    //     convective cells.
    //
    // Far away:
    //
    //     keep only a small amount of cell breakup.
    //
    // This prevents the 20 km cell pattern from being projected
    // across an entire planet from orbit.

    float breakupAmount =
        smoothstep(
            0.12,
            0.78,
            convective);


    breakupAmount *=
        mix(
            0.20,
            1.0,
            detailWeight);


    float commonExistence =
        mix(
            1.0,
            cellGate,
            breakupAmount);


    // =========================================================
    // SHARED VOLUME POSITION
    // =========================================================

    vec3 volumePositionKm =
        sharedWeatherVolumePosition(
            samplePositionKm,
            cellData,
            convective,
            storm);


    // =========================================================
    // MACRO SCALE LOD
    // =========================================================
    //
    // Near convective cloud:
    //
    //     ~24 km
    //
    // Far convective cloud:
    //
    //     ~90 km
    //
    // The same weather still exists, but tiny individual cells are
    // deliberately aggregated into larger cloud masses.

    float scaleMorph =
        smoothstep(
            0.08,
            0.88,
            convective);


    float convectiveMacroPeriodKm =
        mix(
            90.0,
            24.0,
            detailWeight);


    float macroPeriodKm =
        mix(
            coarseShapePeriodKm *
            1.10,
            convectiveMacroPeriodKm,
            scaleMorph);


    float stormMacroPeriodKm =
        mix(
            120.0,
            30.0,
            detailWeight);


    macroPeriodKm =
        mix(
            macroPeriodKm,
            stormMacroPeriodKm,
            storm *
            0.45);


    float macroField =
        sample3DField(
            volumePositionKm,
            macroPeriodKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            vec3(
                0.293,
                0.641,
                0.173),
            vec3(
                0.731,
                0.219,
                0.487));


    // =========================================================
    // LOCAL LOBES
    // =========================================================
    //
    // Completely disappear as the renderer loses the physical
    // sampling resolution necessary to represent them.

    float lobeField =
        macroField;


    float localConvectiveDetail =
        convective *
        detailWeight;


    if (localConvectiveDetail >
        0.05)
    {
        lobeField =
            sample3DField(
                volumePositionKm,
                10.0,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm,
                vec3(
                    0.417,
                    0.853,
                    0.227),
                vec3(
                    0.193,
                    0.569,
                    0.787));
    }


    float convectiveField =
        macroField *
        0.70
        +
        lobeField *
        0.30;


    float field =
        mix(
            macroField,
            convectiveField,
            localConvectiveDetail);


    // =========================================================
    // VERTICAL ENVELOPE
    // =========================================================

    float stratusTop =
        mix(
            0.16,
            0.28,
            weather.a);


    float cumulusTop =
        mix(
            0.46,
            0.74,
            max(
                weather.a,
                cellStrength *
                0.85));


    float stormTop =
        mix(
            0.78,
            1.0,
            max(
                weather.a,
                weather.b));


    float permittedTop =
        mix(
            stratusTop,
            cumulusTop,
            convective);


    permittedTop =
        mix(
            permittedTop,
            stormTop,
            storm);


    float bottomFadeWidth =
        mix(
            0.055,
            0.035,
            convective);


    float bottomFade =
        smoothstep(
            0.0,
            bottomFadeWidth,
            height);


    float upperFadeWidth =
        mix(
            0.08,
            0.14,
            convective);


    upperFadeWidth =
        mix(
            upperFadeWidth,
            0.10,
            storm);


    float topFade =
        1.0 -
        smoothstep(
            max(
                permittedTop -
                upperFadeWidth,
                0.05),
            permittedTop,
            height);


    if (topFade <=
        0.0001)
    {
        return 0.0;
    }


    // =========================================================
    // MACRO BODY THRESHOLD
    // =========================================================

    float threshold =
        mix(
            0.70,
            0.48,
            coverage);


    threshold +=
        convective *
        0.025;


    // Local cells should strongly influence the cloud only while
    // those local cells are still resolvable.

    float cellInfluence =
        mix(
            0.25,
            1.0,
            detailWeight);


    threshold -=
        cellStrength *
        convective *
        0.13 *
        cellInfluence;


    float heightPenalty =
        height *
        mix(
            0.015,
            0.070,
            convective)

        +

        height *
        height *
        mix(
            0.015,
            0.105,
            convective);


    threshold +=
        heightPenalty;


    threshold -=
        storm *
        cellStrength *
        0.075 *
        cellInfluence;


    float body =
        smoothstep(
            threshold,
            threshold +
            0.14,
            field);


    body *=
        weatherMask *
        commonExistence;


    // =========================================================
    // STORM DETAIL LOD
    // =========================================================
    //
    // Far weather can still be stormy.
    //
    // But individual 17 km towers and 25 km anvils are not sampled
    // when the ray itself is jumping by ~10-20 km.
    //
    // The coarse macro field carries the storm region from orbit.

    float stormDetail =
        storm *
        detailWeight;


    if (stormDetail >
        0.02)
    {
        vec3 stormData =
            cellData;


        float stormCellGate =
            smoothstep(
                0.24,
                0.58,
                cellStrength);


        float effectiveStorm =
            stormDetail *
            stormCellGate;


        // -----------------------------------------------------
        // TOWER
        // -----------------------------------------------------

        vec3 towerPositionKm =
            stormVolumePosition(
                samplePositionKm,
                stormData,
                2.45,
                5.5);


        float towerField =
            sample3DField(
                towerPositionKm,
                17.0,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm,
                vec3(
                    0.331,
                    0.817,
                    0.143),
                vec3(
                    0.691,
                    0.373,
                    0.937));


        float towerThreshold =
            0.53
            +
            height *
            0.05
            +
            height *
            height *
            0.10
            -
            cellStrength *
            0.18
            -
            verticalDevelopment *
            0.05;


        float tower =
            smoothstep(
                towerThreshold,
                towerThreshold +
                0.12,
                towerField);


        tower *=
            effectiveStorm;


        body =
            softUnion(
                body,
                tower);


        // -----------------------------------------------------
        // ANVIL
        // -----------------------------------------------------

        float upperDevelopment =
            stormDetail *
            smoothstep(
                0.55,
                0.80,
                cellStrength);


        if (upperDevelopment >
            0.005)
        {
            vec3 anvilPositionKm =
                stormVolumePosition(
                    samplePositionKm,
                    stormData,
                    1.80,
                    10.0);


            float anvilField =
                sample3DField(
                    anvilPositionKm,
                    25.0,
                    sampleDistanceKm,
                    rayAngularFootprint,
                    stepLengthKm,
                    vec3(
                        0.557,
                        0.197,
                        0.773),
                    vec3(
                        0.239,
                        0.911,
                        0.419));


            float upperBand =
                smoothstep(
                    0.67,
                    0.80,
                    height)
                *
                (
                    1.0 -
                    smoothstep(
                        0.97,
                        1.0,
                        height)
                );


            float anvilThreshold =
                0.52
                +
                abs(
                    height -
                    0.87)
                *
                0.15
                -
                cellStrength *
                0.09;


            float anvil =
                smoothstep(
                    anvilThreshold,
                    anvilThreshold +
                    0.13,
                    anvilField);


            anvil *=
                upperBand *
                upperDevelopment;


            body =
                softUnion(
                    body,
                    anvil);
        }
    }


    if (body <=
        0.0001)
    {
        return 0.0;
    }


    // =========================================================
    // EROSION LOD
    // =========================================================

    if (
        convective >
            0.025
        &&
        detailWeight >
            0.05)
    {
        float erosion =
            sampleSharedErosion(
                volumePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm);


        float edgeAmount =
            1.0 -
            smoothstep(
                0.43,
                0.89,
                body);


        float erosionStrength =
            mix(
                0.0,
                0.33,
                convective);


        erosionStrength =
            mix(
                erosionStrength,
                0.29,
                storm);


        erosionStrength *=
            detailWeight;


        body -=
            (1.0 - erosion)
            *
            edgeAmount
            *
            erosionStrength;
    }


    body =
        max(
            body,
            0.0);


    float densityCutoff =
        mix(
            0.025,
            0.055,
            convective);


    body =
        remap01(
            body,
            densityCutoff,
            0.94);


    float densityExponent =
        mix(
            1.10,
            1.22,
            convective);


    densityExponent =
        mix(
            densityExponent,
            1.12,
            storm);


    body =
        pow(
            body,
            densityExponent);


    float density =
        body *
        bottomFade *
        topFade *
        cloudDensityMultiplier;


    return clamp(
        density,
        0.0,
        1.5);
}


// =============================================================
// UNIFIED CLOUD DENSITY
// =============================================================

float sampleCloudDensity(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm)
{
    vec4 weather =
        sampleWeather(
            samplePositionKm);


    float coverage =
        calculateLocalCoverage(
            weather);


    // F6
    if (cloudMorphologyDebugOverride == 1)
    {
        return
            sampleStratusDensity(
                samplePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm,
                weather,
                coverage);
    }


    // F7
    if (cloudMorphologyDebugOverride == 2)
    {
        return
            sampleCumulusDensity(
                samplePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm,
                weather,
                max(
                    coverage,
                    0.60));
    }


    // F8
    if (cloudMorphologyDebugOverride == 3)
    {
        return
            sampleStormDensity(
                samplePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm,
                weather,
                max(
                    coverage,
                    0.55));
    }


    // F5
    return
        sampleWeatherDrivenDensity(
            samplePositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            weather,
            coverage);
}


// =============================================================
// STRATUS SHADOW DENSITY
// =============================================================

float sampleStratusShadowDensity(
    vec3 samplePositionKm,
    float shadowStepLengthKm)
{
    vec4 weather =
        sampleWeather(
            samplePositionKm);


    float coverage =
        calculateLocalCoverage(
            weather);


    if (coverage <=
        0.01)
    {
        return 0.0;
    }


    vec3 weights =
        cloudTypeWeights(
            weather);


    if (weights.x <=
        0.05)
    {
        return 0.0;
    }


    return
        sampleStratusDensity(
            samplePositionKm,
            0.0,
            0.0,
            shadowStepLengthKm,
            weather,
            coverage)
        *
        weights.x;
}


// =============================================================
// SUN TRANSMITTANCE
// =============================================================

float sunTransmittanceAt(
    vec3 samplePositionKm)
{
    if (planetBlocksSun(
            samplePositionKm))
    {
        return 0.0;
    }


    float outerNearKm;
    float outerFarKm;


    if (!raySphereIntervalKm(
            samplePositionKm,
            sunDirection,
            cloudOuterRadiusKm,
            outerNearKm,
            outerFarKm))
    {
        return 1.0;
    }


    float marchDistanceKm =
        max(
            outerFarKm,
            0.0);


    if (marchDistanceKm <=
        0.001)
    {
        return 1.0;
    }


    float stepLengthKm =
        marchDistanceKm /
        float(
            CLOUD_LIGHT_SAMPLE_COUNT);


    float opticalDepth =
        0.0;


    for (int i = 0;
         i < CLOUD_LIGHT_SAMPLE_COUNT;
         ++i)
    {
        float distanceKm =
            (float(i) + 0.5)
            *
            stepLengthKm;


        vec3 positionKm =
            samplePositionKm
            +
            sunDirection *
            distanceKm;


        float density =
            sampleStratusShadowDensity(
                positionKm,
                stepLengthKm);


        opticalDepth +=
            density *
            cloudExtinctionPerKm *
            shadowExtinctionMultiplier *
            stepLengthKm;


        if (opticalDepth >
            12.0)
        {
            return 0.0;
        }
    }


    return exp(
        -opticalDepth);
}


// =============================================================
// PHASE FUNCTION
// =============================================================

float henyeyGreenstein(
    float cosineTheta,
    float g)
{
    float gSquared =
        g * g;


    float denominator =
        max(
            1.0 +
            gSquared -
            2.0 *
            g *
            cosineTheta,
            0.0001);


    return
        (1.0 - gSquared)
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


    return mix(
        backwardPhase,
        forwardPhase,
        forwardScatteringWeight);
}


// =============================================================
// CAMERA MARCH
// =============================================================

void marchCloudSegment(
    vec3 originKm,
    vec3 direction,
    vec2 segmentKm,
    float sceneDistanceKm,
    float rayAngularFootprint,
    inout vec3 accumulatedRadiance,
    inout float accumulatedTransmittance)
{
    float startKm =
        segmentKm.x;


    float endKm =
        segmentKm.y;


    if (sceneDistanceKm >
        0.0)
    {
        endKm =
            min(
                endKm,
                sceneDistanceKm);
    }


    // ---------------------------------------------------------
    // DEBUG LAB LIMITS
    // ---------------------------------------------------------

    if (cloudMorphologyDebugOverride == 2)
    {
        endKm =
            min(
                endKm,
                startKm +
                80.0);
    }


    if (cloudMorphologyDebugOverride == 3)
    {
        endKm =
            min(
                endKm,
                startKm +
                110.0);
    }


    if (endKm <=
        startKm)
    {
        return;
    }


    float segmentLengthKm =
        endKm -
        startKm;


    bool debugLab =
        cloudMorphologyDebugOverride != 0;


    // ---------------------------------------------------------
    // DEBUG FIXED STEP
    // ---------------------------------------------------------

    float debugStepLengthKm =
        segmentLengthKm /
        float(
            CLOUD_DEBUG_VIEW_STEPS);


    // ---------------------------------------------------------
    // F5 MAX-STEP SAFETY
    // ---------------------------------------------------------
    //
    // This guarantees that even a very long grazing shell segment
    // can fit inside the maximum loop count.
    //
    // Example:
    //
    // 1000 km segment / 128
    //
    // = minimum step of 7.8 km
    //
    // We therefore never accidentally spend all 128 samples in the
    // first 150 km and leave the rest of the cloud shell unrendered.

    float minimumF5StepLengthKm =
        segmentLengthKm /
        float(
            CLOUD_MAX_VIEW_STEPS);


    float phase =
        cloudPhase(
            direction);


    float currentDistanceKm =
        startKm;


    for (int i = 0;
         i < CLOUD_MAX_VIEW_STEPS;
         ++i)
    {
        if (currentDistanceKm >=
            endKm)
        {
            break;
        }


        if (
            debugLab &&
            i >=
            CLOUD_DEBUG_VIEW_STEPS)
        {
            break;
        }


        float stepLengthKm;


        if (debugLab)
        {
            stepLengthKm =
                debugStepLengthKm;
        }
        else
        {
            float adaptiveStepKm =
                adaptiveViewStepLengthKm(
                    currentDistanceKm);


            stepLengthKm =
                max(
                    adaptiveStepKm,
                    minimumF5StepLengthKm);
        }


        stepLengthKm =
            min(
                stepLengthKm,
                endKm -
                currentDistanceKm);


        if (stepLengthKm <=
            0.0001)
        {
            break;
        }


        // -----------------------------------------------------
        // STABLE PER-PIXEL / PER-INTERVAL JITTER
        // -----------------------------------------------------

        float sampleJitter =
            hash12(
                gl_FragCoord.xy
                +
                vec2(
                    float(i) *
                    13.37,
                    float(i) *
                    41.73));


        float intervalSamplePosition =
            mix(
                0.25,
                0.75,
                sampleJitter);


        float sampleDistanceKm =
            currentDistanceKm
            +
            intervalSamplePosition *
            stepLengthKm;


        // Advance now so that all continue paths still progress.

        currentDistanceKm +=
            stepLengthKm;


        vec3 samplePositionKm =
            originKm
            +
            direction *
            sampleDistanceKm;


        float density =
            sampleCloudDensity(
                samplePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm);


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


        vec4 lightingWeather =
            sampleWeather(
                samplePositionKm);


        vec3 lightingWeights =
            cloudTypeWeights(
                lightingWeather);


        bool convectiveCloud =
            lightingWeights.y +
            lightingWeights.z >
            0.15;


        bool sunBlocked =
            planetBlocksSun(
                samplePositionKm);


        float sunTransmittance;


        if (sunBlocked)
        {
            sunTransmittance =
                0.0;
        }
        else if (
            convectiveCloud ||
            cloudMorphologyDebugOverride == 2 ||
            cloudMorphologyDebugOverride == 3)
        {
            sunTransmittance =
                1.0;
        }
        else
        {
            sunTransmittance =
                sunTransmittanceAt(
                    samplePositionKm);
        }


        vec3 incidentSunlight =
            sunRadiance *
            sunTransmittance;


        // -----------------------------------------------------
        // TEMPORARY AMBIENT FILL
        // -----------------------------------------------------

        float fillStrength =
            lightingWeights.x *
            0.010
            +
            lightingWeights.y *
            0.018
            +
            lightingWeights.z *
            0.013;


        if (sunBlocked)
        {
            fillStrength =
                0.0;
        }


        vec3 diffuseFill =
            sunRadiance *
            fillStrength;


        vec3 scatteredRadiance =
            (
                incidentSunlight *
                phase

                +

                diffuseFill
            )
            *
            cloudScatteringAlbedo
            *
            cloudLightingIntensity;


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


    float sceneDistanceWorld =
        texture(
            sceneLinearDepthTexture,
            vUV).r;


    float sceneDistanceKm =
        sceneDistanceWorld >
        0.0
            ?
            sceneDistanceWorld *
            kmPerWorldUnit
            :
            0.0;


    vec3 rayDirection =
        reconstructWorldRay(
            vUV);


    vec3 rayDerivativeX =
        dFdx(
            rayDirection);


    vec3 rayDerivativeY =
        dFdy(
            rayDirection);


    float rayAngularFootprint =
        max(
            length(
                rayDerivativeX),
            length(
                rayDerivativeY));


    vec3 originKm =
        (
            cameraPositionWorld -
            planetCenterWorld
        )
        *
        kmPerWorldUnit;


    vec2 segmentA;
    vec2 segmentB;


    int segmentCount =
        buildCloudShellSegments(
            originKm,
            rayDirection,
            segmentA,
            segmentB);


    if (segmentCount == 0)
    {
        outColor =
            vec4(
                sceneColor,
                1.0);


        return;
    }


    vec3 accumulatedRadiance =
        vec3(0.0);


    float accumulatedTransmittance =
        1.0;


    marchCloudSegment(
        originKm,
        rayDirection,
        segmentA,
        sceneDistanceKm,
        rayAngularFootprint,
        accumulatedRadiance,
        accumulatedTransmittance);


    if (
        segmentCount >
            1
        &&
        accumulatedTransmittance >
            0.001)
    {
        marchCloudSegment(
            originKm,
            rayDirection,
            segmentB,
            sceneDistanceKm,
            rayAngularFootprint,
            accumulatedRadiance,
            accumulatedTransmittance);
    }


    outColor =
        vec4(
            accumulatedRadiance
            +
            sceneColor *
            accumulatedTransmittance,
            1.0);
}