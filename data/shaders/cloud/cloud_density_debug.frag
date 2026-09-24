#version 450 core

layout(location = 0)
in vec2 vUV;

layout(location = 0)
out vec4 outColor;


// =============================================================
// CHANGE ONLY THIS LINE BETWEEN TESTS
// =============================================================
//
// 1 = weather coverage
// 2 = raw coarse 3D shape
// 3 = thresholded coarse cloud body
// 4 = coarse body after vertical profile
// 5 = raw fine 3D shape
// 6 = final cloud density
// 7 = accumulated optical opacity
// 8 = fraction of samples containing non-zero density
// 9 = cloud type RGB
//
//     red   = stratus
//     green = cumulus
//     blue  = towering / storm

const int CLOUD_DEBUG_VIEW = 1;


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

uniform float localShapeFadeStartFootprintKm;
uniform float localShapeFadeEndFootprintKm;

uniform float cloudExtinctionPerKm;


// =============================================================
// QUALITY
// =============================================================

const int DEBUG_SAMPLE_COUNT = 64;


// =============================================================
// DEBUG SAMPLE DATA
// =============================================================

struct DensityStages
{
    float localCoverage;

    vec3 typeWeights;

    float heightProfile;

    float coarseShape;

    float coarseBody;

    float coarseWithHeight;

    float fineShape;

    float finalDensity;
};


// =============================================================
// BASIC HELPERS
// =============================================================

float remap01(
    float value,
    float minValue,
    float maxValue)
{
    return clamp(
        (value - minValue)
        /
        max(
            maxValue - minValue,
            0.0001),
        0.0,
        1.0);
}


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
        b *
        b -
        c;


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
        (
            cloudCoverage -
            0.5
        )
        *
        0.85;


    return clamp(
        weather.r +
        coverageBias,
        0.0,
        1.0);
}


// =============================================================
// CLOUD TYPE
// =============================================================

vec3 cloudTypeWeights(
    vec4 weather)
{
    float type =
        clamp(
            max(
                weather.g,
                weather.b *
                0.90),
            0.0,
            1.0);


    float stratus =
        1.0 -
        smoothstep(
            0.22,
            0.48,
            type);


    float towering =
        smoothstep(
            0.58,
            0.86,
            type);


    float cumulus =
        max(
            1.0 -
            stratus -
            towering,
            0.0);


    vec3 weights =
        vec3(
            stratus,
            cumulus,
            towering);


    return weights /
        max(
            weights.x +
            weights.y +
            weights.z,
            0.0001);
}


// =============================================================
// HEIGHT PROFILE
// =============================================================

float normalizedCloudHeight(
    float radiusKm)
{
    return clamp(
        (
            radiusKm -
            cloudInnerRadiusKm
        )
        /
        max(
            cloudOuterRadiusKm -
            cloudInnerRadiusKm,
            0.0001),
        0.0,
        1.0);
}


float stratusHeightProfile(
    float height,
    vec4 weather)
{
    float bottom =
        smoothstep(
            0.0,
            0.055,
            height);


    float topHeight =
        mix(
            0.18,
            0.32,
            weather.a);


    float top =
        1.0 -
        smoothstep(
            max(
                topHeight -
                0.10,
                0.05),
            topHeight,
            height);


    return
        bottom *
        top;
}


float cumulusHeightProfile(
    float height,
    float localCoverage,
    vec4 weather)
{
    float bottom =
        smoothstep(
            0.0,
            0.065,
            height);


    float development =
        clamp(
            weather.a *
            0.65
            +
            localCoverage *
            0.35,
            0.0,
            1.0);


    float topHeight =
        mix(
            0.48,
            0.76,
            development);


    float top =
        1.0 -
        smoothstep(
            max(
                topHeight -
                0.24,
                0.10),
            topHeight,
            height);


    return
        bottom *
        top;
}


float toweringHeightProfile(
    float height,
    float localCoverage,
    vec4 weather)
{
    float bottom =
        smoothstep(
            0.0,
            0.055,
            height);


    float development =
        max(
            weather.a,
            weather.b);


    development =
        max(
            development,
            localCoverage *
            0.75);


    float topHeight =
        mix(
            0.72,
            1.0,
            clamp(
                development,
                0.0,
                1.0));


    float top =
        1.0 -
        smoothstep(
            max(
                topHeight -
                0.20,
                0.20),
            topHeight,
            height);


    return
        bottom *
        top;
}


float cloudHeightProfile(
    float radiusKm,
    float localCoverage,
    vec4 weather)
{
    float height =
        normalizedCloudHeight(
            radiusKm);


    vec3 weights =
        cloudTypeWeights(
            weather);


    return
        stratusHeightProfile(
            height,
            weather)
        *
        weights.x

        +

        cumulusHeightProfile(
            height,
            localCoverage,
            weather)
        *
        weights.y

        +

        toweringHeightProfile(
            height,
            localCoverage,
            weather)
        *
        weights.z;
}


// =============================================================
// PIXEL FOOTPRINT / LOD
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


float noiseLod(
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    float physicalPeriodKm)
{
    float samplingFootprintKm =
        max(
            pixelFootprintKm(
                sampleDistanceKm,
                rayAngularFootprint),
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


float fineShapeWeight(
    float sampleDistanceKm,
    float rayAngularFootprint)
{
    float footprintKm =
        pixelFootprintKm(
            sampleDistanceKm,
            rayAngularFootprint);


    return
        1.0 -
        smoothstep(
            localShapeFadeStartFootprintKm,
            localShapeFadeEndFootprintKm,
            footprintKm);
}


// =============================================================
// ANISOTROPIC CLOUD COORDINATES
// =============================================================

vec3 cloudShapePosition(
    vec3 samplePositionKm,
    float verticalScale)
{
    vec3 radialDirection =
        normalize(
            samplePositionKm);


    float radiusKm =
        length(
            samplePositionKm);


    float altitudeAboveBaseKm =
        max(
            radiusKm -
            cloudInnerRadiusKm,
            0.0);


    vec3 baseAnchorKm =
        radialDirection *
        cloudInnerRadiusKm;


    return
        baseAnchorKm
        +
        radialDirection
        *
        altitudeAboveBaseKm
        *
        verticalScale;
}


float coarseVerticalScaleForWeather(
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    return
        weights.x *
        3.5
        +
        weights.y *
        12.0
        +
        weights.z *
        20.0;
}


float fineVerticalScaleForWeather(
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    return
        weights.x *
        5.0
        +
        weights.y *
        16.0
        +
        weights.z *
        26.0;
}


// =============================================================
// CLOUD PERIOD
// =============================================================

float coarsePeriodForWeather(
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    float multiplier =
        weights.x *
        1.35
        +
        weights.y *
        0.82
        +
        weights.z *
        0.62;


    return
        coarseShapePeriodKm *
        multiplier;
}


mat3 shapeRotation()
{
    return mat3(
         0.00,  0.80,  0.60,
        -0.80,  0.36, -0.48,
        -0.60, -0.48,  0.64);
}


// =============================================================
// COARSE SHAPE
// =============================================================

float sampleCoarseShape(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    float physicalPeriodKm,
    float verticalScale)
{
    vec3 shapedPositionKm =
        cloudShapePosition(
            samplePositionKm,
            verticalScale);


    float lodA =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            physicalPeriodKm);


    vec3 uvA =
        shapedPositionKm /
        max(
            physicalPeriodKm,
            0.001);


    float shapeA =
        textureLod(
            baseShapeNoise,
            uvA,
            lodA).g;


    float secondPeriodKm =
        physicalPeriodKm *
        1.731;


    float lodB =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            secondPeriodKm);


    vec3 uvB =
        (
            shapeRotation()
            *
            shapedPositionKm
        )
        /
        max(
            secondPeriodKm,
            0.001);


    uvB +=
        vec3(
            0.31,
            0.67,
            0.19);


    float shapeB =
        textureLod(
            baseShapeNoise,
            uvB,
            lodB).g;


    return mix(
        shapeA,
        shapeB,
        0.35);
}


// =============================================================
// FINE SHAPE
// =============================================================

float sampleFineShape(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm,
    float verticalScale)
{
    vec3 shapedPositionKm =
        cloudShapePosition(
            samplePositionKm,
            verticalScale);


    float lodA =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            baseShapePeriodKm);


    vec3 uvA =
        shapedPositionKm /
        max(
            baseShapePeriodKm,
            0.001);


    float shapeA =
        textureLod(
            baseShapeNoise,
            uvA,
            lodA).r;


    float secondPeriodKm =
        baseShapePeriodKm *
        1.6180339;


    float lodB =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            secondPeriodKm);


    vec3 uvB =
        (
            shapeRotation()
            *
            shapedPositionKm
        )
        /
        max(
            secondPeriodKm,
            0.001);


    uvB +=
        vec3(
            0.37,
            0.11,
            0.73);


    float shapeB =
        textureLod(
            baseShapeNoise,
            uvB,
            lodB).r;


    return mix(
        shapeA,
        shapeB,
        0.35);
}


// =============================================================
// MORPHOLOGY THRESHOLD
// =============================================================

float morphologyThresholdBias(
    float normalizedHeight,
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    float stratusBias =
        mix(
            -0.02,
            0.18,
            smoothstep(
                0.10,
                0.34,
                normalizedHeight));


    float cumulusBias =
        mix(
            -0.05,
            0.16,
            smoothstep(
                0.12,
                0.78,
                normalizedHeight));


    float toweringBias =
        mix(
            -0.07,
            0.18,
            smoothstep(
                0.30,
                0.96,
                normalizedHeight));


    toweringBias -=
        weather.b
        *
        (
            1.0 -
            smoothstep(
                0.72,
                0.94,
                normalizedHeight)
        )
        *
        0.045;


    return
        stratusBias *
        weights.x

        +

        cumulusBias *
        weights.y

        +

        toweringBias *
        weights.z;
}


float coarseThresholdForWeather(
    float localCoverage,
    vec4 weather,
    float normalizedHeight)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    float threshold =
        mix(
            0.72,
            0.48,
            localCoverage);


    threshold +=
        weights.y *
        0.035;


    threshold +=
        weights.z *
        0.055;


    threshold +=
        morphologyThresholdBias(
            normalizedHeight,
            weather);


    return threshold;
}


// =============================================================
// FINE MODULATION
// =============================================================

float fineShapeModifier(
    float fineShape,
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    float response =
        smoothstep(
            0.28,
            0.76,
            fineShape);


    float minimumModifier =
        weights.x *
        0.78

        +

        weights.y *
        0.20

        +

        weights.z *
        0.26;


    float maximumModifier =
        weights.x *
        1.08

        +

        weights.y *
        1.38

        +

        weights.z *
        1.48;


    return mix(
        minimumModifier,
        maximumModifier,
        response);
}


// =============================================================
// DENSITY CORE REMAP
// =============================================================

float densityCoreRemap(
    float rawDensity,
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(
            weather);


    float cutoff =
        weights.x *
        0.12

        +

        weights.y *
        0.22

        +

        weights.z *
        0.18;


    float exponent =
        weights.x *
        1.2

        +

        weights.y *
        1.8

        +

        weights.z *
        1.6;


    float core =
        remap01(
            rawDensity,
            cutoff,
            1.0);


    return pow(
        core,
        exponent);
}


// =============================================================
// EVALUATE EVERY DENSITY STAGE
// =============================================================

DensityStages evaluateDensityStages(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm)
{
    DensityStages result;


    result.localCoverage = 0.0;

    result.typeWeights =
        vec3(0.0);

    result.heightProfile = 0.0;

    result.coarseShape = 0.0;

    result.coarseBody = 0.0;

    result.coarseWithHeight = 0.0;

    result.fineShape = 0.0;

    result.finalDensity = 0.0;


    // =========================================================
    // WEATHER
    // =========================================================

    vec4 weather =
        sampleWeather(
            samplePositionKm);


    float localCoverage =
        calculateLocalCoverage(
            weather);


    result.localCoverage =
        localCoverage;


    result.typeWeights =
        cloudTypeWeights(
            weather);


    // =========================================================
    // HEIGHT
    // =========================================================

    float radiusKm =
        length(
            samplePositionKm);


    float normalizedHeight =
        normalizedCloudHeight(
            radiusKm);


    float heightProfile =
        cloudHeightProfile(
            radiusKm,
            localCoverage,
            weather);


    result.heightProfile =
        heightProfile;


    // =========================================================
    // SHAPE
    // =========================================================

    float weatherMask =
        smoothstep(
            0.10,
            0.45,
            localCoverage);


    float physicalCoarsePeriodKm =
        coarsePeriodForWeather(
            weather);


    float coarseVerticalScale =
        coarseVerticalScaleForWeather(
            weather);


    float fineVerticalScale =
        fineVerticalScaleForWeather(
            weather);


    float coarseShape =
        sampleCoarseShape(
            samplePositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            physicalCoarsePeriodKm,
            coarseVerticalScale);


    result.coarseShape =
        coarseShape;


    float coarseThreshold =
        coarseThresholdForWeather(
            localCoverage,
            weather,
            normalizedHeight);


    float coarseDensity =
        remap01(
            coarseShape,
            coarseThreshold,
            coarseThreshold +
            0.14);


    coarseDensity =
        coarseDensity
        *
        coarseDensity
        *
        (
            3.0 -
            2.0 *
            coarseDensity
        );


    coarseDensity *=
        weatherMask;


    result.coarseBody =
        coarseDensity;


    result.coarseWithHeight =
        coarseDensity
        *
        heightProfile;


    // =========================================================
    // FINE SHAPE
    // =========================================================

    float fineShape =
        sampleFineShape(
            samplePositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            fineVerticalScale);


    result.fineShape =
        fineShape;


    float fineModifier =
        fineShapeModifier(
            fineShape,
            weather);


    float detailWeight =
        fineShapeWeight(
            sampleDistanceKm,
            rayAngularFootprint);


    float shapeDensity =
        coarseDensity
        *
        mix(
            1.0,
            fineModifier,
            detailWeight);


    shapeDensity =
        densityCoreRemap(
            shapeDensity,
            weather);


    float finalDensity =
        shapeDensity
        *
        heightProfile
        *
        weatherMask
        *
        cloudDensityMultiplier;


    result.finalDensity =
        clamp(
            finalDensity,
            0.0,
            1.5);


    return result;
}


// =============================================================
// DEBUG ACCUMULATION
// =============================================================

void debugMarchSegment(
    vec3 originKm,
    vec3 direction,
    vec2 segmentKm,
    float sceneDistanceKm,
    float rayAngularFootprint,
    inout float accumulatedValue,
    inout float accumulatedSamples,
    inout float occupiedSamples,
    inout float accumulatedOpticalDepth,
    inout vec3 accumulatedTypeWeights)
{
    float startKm =
        segmentKm.x;


    float endKm =
        segmentKm.y;


    if (sceneDistanceKm > 0.0)
    {
        endKm =
            min(
                endKm,
                sceneDistanceKm);
    }


    if (endKm <= startKm)
    {
        return;
    }


    float stepLengthKm =
        (
            endKm -
            startKm
        )
        /
        float(
            DEBUG_SAMPLE_COUNT);


    for (int i = 0;
         i < DEBUG_SAMPLE_COUNT;
         ++i)
    {
        float sampleDistanceKm =
            startKm
            +
            (
                float(i) +
                0.5
            )
            *
            stepLengthKm;


        vec3 samplePositionKm =
            originKm
            +
            direction
            *
            sampleDistanceKm;


        DensityStages stages =
            evaluateDensityStages(
                samplePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm);


        float selectedValue =
            0.0;


        if (CLOUD_DEBUG_VIEW == 1)
        {
            selectedValue =
                stages.localCoverage;
        }
        else if (CLOUD_DEBUG_VIEW == 2)
        {
            selectedValue =
                stages.coarseShape;
        }
        else if (CLOUD_DEBUG_VIEW == 3)
        {
            selectedValue =
                stages.coarseBody;
        }
        else if (CLOUD_DEBUG_VIEW == 4)
        {
            selectedValue =
                stages.coarseWithHeight;
        }
        else if (CLOUD_DEBUG_VIEW == 5)
        {
            selectedValue =
                stages.fineShape;
        }
        else if (CLOUD_DEBUG_VIEW == 6)
        {
            selectedValue =
                stages.finalDensity;
        }


        accumulatedValue +=
            selectedValue;


        accumulatedTypeWeights +=
            stages.typeWeights;


        accumulatedSamples +=
            1.0;


        if (stages.finalDensity >
            0.001)
        {
            occupiedSamples +=
                1.0;
        }


        accumulatedOpticalDepth +=
            stages.finalDensity
            *
            cloudExtinctionPerKm
            *
            stepLengthKm;
    }
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    float sceneDistanceWorld =
        texture(
            sceneLinearDepthTexture,
            vUV).r;


    float sceneDistanceKm =
        sceneDistanceWorld >
        0.0
            ?
            sceneDistanceWorld
            *
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


    // Black means:
    //
    // this pixel's ray does not touch the cloud shell at all.
    if (segmentCount == 0)
    {
        outColor =
            vec4(
                0.0,
                0.0,
                0.0,
                1.0);

        return;
    }


    float accumulatedValue =
        0.0;


    float accumulatedSamples =
        0.0;


    float occupiedSamples =
        0.0;


    float accumulatedOpticalDepth =
        0.0;


    vec3 accumulatedTypeWeights =
        vec3(0.0);


    debugMarchSegment(
        originKm,
        rayDirection,
        segmentA,
        sceneDistanceKm,
        rayAngularFootprint,
        accumulatedValue,
        accumulatedSamples,
        occupiedSamples,
        accumulatedOpticalDepth,
        accumulatedTypeWeights);


    if (segmentCount > 1)
    {
        debugMarchSegment(
            originKm,
            rayDirection,
            segmentB,
            sceneDistanceKm,
            rayAngularFootprint,
            accumulatedValue,
            accumulatedSamples,
            occupiedSamples,
            accumulatedOpticalDepth,
            accumulatedTypeWeights);
    }


    if (accumulatedSamples <=
        0.0)
    {
        // Real scene geometry was encountered before cloud.
        //
        // Leave a dim copy of it visible so you can still orient
        // yourself while using the debug view.

        vec3 sceneColor =
            texture(
                sceneColorTexture,
                vUV).rgb;


        outColor =
            vec4(
                sceneColor *
                0.15,
                1.0);

        return;
    }


    // =========================================================
    // MODES 1–6
    // =========================================================
    //
    // Show average value along the visible cloud-shell path.
    //
    // Average is intentional:
    //
    // MAX would turn almost any long horizon ray white if it touched
    // one cloud somewhere hundreds of kilometres away.

    if (CLOUD_DEBUG_VIEW >= 1 &&
        CLOUD_DEBUG_VIEW <= 6)
    {
        float averageValue =
            accumulatedValue /
            accumulatedSamples;


        // Final density can exceed 1 slightly.
        // Clamp only for display.
        averageValue =
            clamp(
                averageValue,
                0.0,
                1.0);


        outColor =
            vec4(
                vec3(
                    averageValue),
                1.0);

        return;
    }


    // =========================================================
    // MODE 7 — OPTICAL OPACITY
    // =========================================================
    //
    // This is extremely important for the current bug.
    //
    // It answers:
    //
    // "After integrating all of the tiny density values along this
    //  ray, how opaque does the cloud system become?"
    //
    // White:
    //     effectively opaque
    //
    // Black:
    //     essentially clear

    if (CLOUD_DEBUG_VIEW == 7)
    {
        float opacity =
            1.0 -
            exp(
                -accumulatedOpticalDepth);


        outColor =
            vec4(
                vec3(
                    clamp(
                        opacity,
                        0.0,
                        1.0)),
                1.0);

        return;
    }


    // =========================================================
    // MODE 8 — NON-ZERO OCCUPANCY
    // =========================================================
    //
    // This directly answers:
    //
    // "What percentage of ray samples contain ANY cloud?"
    //
    // For discrete cloud fields we want plenty of BLACK regions.
    //
    // If most of the atmosphere is medium/white here, that proves
    // we still have residual density almost everywhere.

    if (CLOUD_DEBUG_VIEW == 8)
    {
        float occupancy =
            occupiedSamples /
            accumulatedSamples;


        outColor =
            vec4(
                vec3(
                    occupancy),
                1.0);

        return;
    }


    // =========================================================
    // MODE 9 — CLOUD TYPE
    // =========================================================
    //
    // red   = stratus
    // green = cumulus
    // blue  = towering

    if (CLOUD_DEBUG_VIEW == 9)
    {
        vec3 averageWeights =
            accumulatedTypeWeights /
            accumulatedSamples;


        outColor =
            vec4(
                averageWeights,
                1.0);

        return;
    }


    // Invalid mode.
    outColor =
        vec4(
            1.0,
            0.0,
            1.0,
            1.0);
}