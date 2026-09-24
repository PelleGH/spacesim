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
// WEATHER / SHAPE
// =============================================================

uniform float cloudCoverage;
uniform float coarseShapePeriodKm;
uniform float baseShapePeriodKm;
uniform float cloudDensityMultiplier;


// =============================================================
// LOCAL SHAPE LOD
// =============================================================

uniform float localShapeFadeStartFootprintKm;
uniform float localShapeFadeEndFootprintKm;


// =============================================================
// CLOUD LIGHTING
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
// QUALITY
// =============================================================

const int CLOUD_SAMPLE_COUNT = 64;
const int CLOUD_LIGHT_SAMPLE_COUNT = 6;

const float PI = 3.14159265358979323846;


// =============================================================
// HELPERS
// =============================================================

float remap01(
    float value,
    float minValue,
    float maxValue)
{
    return clamp(
        (value - minValue) / max(maxValue - minValue, 0.0001),
        0.0,
        1.0);
}


vec3 reconstructWorldRay(
    vec2 uv)
{
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 farPoint =
        inverseViewProjection *
        vec4(ndc, 1.0, 1.0);

    farPoint /= farPoint.w;

    return normalize(farPoint.xyz - cameraPositionWorld);
}


bool raySphereIntervalKm(
    vec3 originKm,
    vec3 direction,
    float radiusKm,
    out float tNearKm,
    out float tFarKm)
{
    float b = dot(originKm, direction);
    float c = dot(originKm, originKm) - radiusKm * radiusKm;
    float discriminant = b * b - c;

    if (discriminant < 0.0)
    {
        tNearKm = -1.0;
        tFarKm = -1.0;
        return false;
    }

    float root = sqrt(max(discriminant, 0.0));

    tNearKm = -b - root;
    tFarKm = -b + root;

    return tFarKm > 0.0;
}


bool planetBlocksSun(
    vec3 samplePositionKm)
{
    vec3 origin = samplePositionKm + sunDirection * 0.01;

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

    return tNearKm > 0.0;
}


int buildCloudShellSegments(
    vec3 originKm,
    vec3 direction,
    out vec2 segmentA,
    out vec2 segmentB)
{
    segmentA = vec2(0.0);
    segmentB = vec2(0.0);

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

    float shellStartKm = max(outerNearKm, 0.0);
    float shellEndKm = outerFarKm;

    if (shellEndKm <= shellStartKm)
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
        segmentA = vec2(shellStartKm, shellEndKm);
        return 1;
    }

    float cutStartKm = max(shellStartKm, innerNearKm);
    float cutEndKm = min(shellEndKm, innerFarKm);

    if (cutEndKm <= cutStartKm)
    {
        segmentA = vec2(shellStartKm, shellEndKm);
        return 1;
    }

    int count = 0;

    if (cutStartKm > shellStartKm)
    {
        segmentA = vec2(shellStartKm, cutStartKm);
        count = 1;
    }

    if (cutEndKm < shellEndKm)
    {
        if (count == 0)
        {
            segmentA = vec2(cutEndKm, shellEndKm);
        }
        else
        {
            segmentB = vec2(cutEndKm, shellEndKm);
        }

        count += 1;
    }

    return count;
}


// =============================================================
// WEATHER
// =============================================================

vec4 sampleWeather(
    vec3 samplePositionKm)
{
    vec3 planetDirection = normalize(samplePositionKm);

    return textureLod(
        weatherMap,
        planetDirection,
        0.0);
}


float calculateLocalCoverage(
    vec4 weather)
{
    float coverageBias =
        (cloudCoverage - 0.5) * 0.85;

    return clamp(
        weather.r + coverageBias,
        0.0,
        1.0);
}


vec3 cloudTypeWeights(
    vec4 weather)
{
    float type =
        clamp(
            max(
                weather.g,
                weather.b * 0.90),
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
            1.0 - stratus - towering,
            0.0);

    vec3 weights = vec3(stratus, cumulus, towering);
    float weightSum = max(weights.x + weights.y + weights.z, 0.0001);

    return weights / weightSum;
}


float normalizedCloudHeight(
    float radiusKm)
{
    return clamp(
        (radiusKm - cloudInnerRadiusKm) /
        max(cloudOuterRadiusKm - cloudInnerRadiusKm, 0.0001),
        0.0,
        1.0);
}


// =============================================================
// CLOUD PROFILES
// =============================================================

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
            max(topHeight - 0.10, 0.05),
            topHeight,
            height);

    return bottom * top;
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
            weather.a * 0.65 +
            localCoverage * 0.35,
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
            max(topHeight - 0.24, 0.10),
            topHeight,
            height);

    return bottom * top;
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
            localCoverage * 0.75);

    float topHeight =
        mix(
            0.72,
            1.0,
            clamp(development, 0.0, 1.0));

    float top =
        1.0 -
        smoothstep(
            max(topHeight - 0.20, 0.20),
            topHeight,
            height);

    return bottom * top;
}


float cloudHeightProfile(
    float radiusKm,
    float localCoverage,
    vec4 weather)
{
    float height =
        normalizedCloudHeight(radiusKm);

    vec3 typeWeights =
        cloudTypeWeights(weather);

    float stratus =
        stratusHeightProfile(height, weather);

    float cumulus =
        cumulusHeightProfile(height, localCoverage, weather);

    float towering =
        toweringHeightProfile(height, localCoverage, weather);

    return
        stratus * typeWeights.x +
        cumulus * typeWeights.y +
        towering * typeWeights.z;
}


// =============================================================
// PIXEL FOOTPRINT / LOD
// =============================================================

float pixelFootprintKm(
    float sampleDistanceKm,
    float rayAngularFootprint)
{
    return max(
        sampleDistanceKm * rayAngularFootprint,
        0.0001);
}


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

    ivec3 textureResolution =
        textureSize(baseShapeNoise, 0);

    float voxelSizeKm =
        physicalPeriodKm /
        float(max(textureResolution.x, 1));

    float lod =
        log2(
            max(
                samplingFootprintKm /
                max(voxelSizeKm, 0.0001),
                1.0));

    float maximumLod =
        float(
            max(
                textureQueryLevels(baseShapeNoise) - 1,
                0));

    return clamp(lod, 0.0, maximumLod);
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


mat3 shapeRotation()
{
    return mat3(
         0.00,  0.80,  0.60,
        -0.80,  0.36, -0.48,
        -0.60, -0.48,  0.64);
}


// =============================================================
// ANISOTROPIC CLOUD COORDINATES
// =============================================================
//
// This is the key fix.
//
// We exaggerate vertical/radial variation before sampling cloud
// noise. That gives the density field meaningful structure through
// the 10.5 km cloud thickness instead of behaving like a thin slice
// through a giant isotropic volume.

vec3 cloudShapePosition(
    vec3 samplePositionKm,
    float verticalScale)
{
    vec3 radialDirection =
        normalize(samplePositionKm);

    float radiusKm =
        length(samplePositionKm);

    float altitudeAboveBaseKm =
        max(radiusKm - cloudInnerRadiusKm, 0.0);

    vec3 baseAnchorKm =
        radialDirection * cloudInnerRadiusKm;

    return
        baseAnchorKm +
        radialDirection *
            altitudeAboveBaseKm *
            verticalScale;
}


float coarseVerticalScaleForWeather(
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(weather);

    return
        weights.x * 3.5 +   // stratus
        weights.y * 12.0 +  // cumulus
        weights.z * 20.0;   // towering
}


float fineVerticalScaleForWeather(
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(weather);

    return
        weights.x * 5.0 +   // stratus
        weights.y * 16.0 +  // cumulus
        weights.z * 26.0;   // towering
}


// =============================================================
// CLOUD-TYPE PHYSICAL SCALE
// =============================================================

float coarsePeriodForWeather(
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(weather);

    float multiplier =
        weights.x * 1.35 +
        weights.y * 0.82 +
        weights.z * 0.62;

    return coarseShapePeriodKm * multiplier;
}


// =============================================================
// SHAPE SAMPLING
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
        max(physicalPeriodKm, 0.001);

    float shapeA =
        textureLod(
            baseShapeNoise,
            uvA,
            lodA).g;

    float secondPeriodKm =
        physicalPeriodKm * 1.731;

    float lodB =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            secondPeriodKm);

    vec3 uvB =
        (shapeRotation() * shapedPositionKm) /
        max(secondPeriodKm, 0.001);

    uvB += vec3(0.31, 0.67, 0.19);

    float shapeB =
        textureLod(
            baseShapeNoise,
            uvB,
            lodB).g;

    return mix(shapeA, shapeB, 0.35);
}


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
        max(baseShapePeriodKm, 0.001);

    float shapeA =
        textureLod(
            baseShapeNoise,
            uvA,
            lodA).r;

    float secondPeriodKm =
        baseShapePeriodKm * 1.6180339;

    float lodB =
        noiseLod(
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            secondPeriodKm);

    vec3 uvB =
        (shapeRotation() * shapedPositionKm) /
        max(secondPeriodKm, 0.001);

    uvB += vec3(0.37, 0.11, 0.73);

    float shapeB =
        textureLod(
            baseShapeNoise,
            uvB,
            lodB).r;

    return mix(shapeA, shapeB, 0.35);
}


// =============================================================
// MORPHOLOGY
// =============================================================

float morphologyThresholdBias(
    float normalizedHeight,
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(weather);

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
        weather.b *
        (1.0 - smoothstep(0.72, 0.94, normalizedHeight)) *
        0.045;

    return
        stratusBias * weights.x +
        cumulusBias * weights.y +
        toweringBias * weights.z;
}


float coarseThresholdForWeather(
    float localCoverage,
    vec4 weather,
    float normalizedHeight)
{
    vec3 weights =
        cloudTypeWeights(weather);

    float threshold =
        mix(
            0.72,
            0.48,
            localCoverage);

    threshold +=
        weights.y * 0.035;

    threshold +=
        weights.z * 0.055;

    threshold +=
        morphologyThresholdBias(
            normalizedHeight,
            weather);

    return threshold;
}


float fineShapeModifier(
    float fineShape,
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(weather);

    float fineResponse =
        smoothstep(
            0.28,
            0.76,
            fineShape);

    float minimumModifier =
        weights.x * 0.78 +
        weights.y * 0.20 +
        weights.z * 0.26;

    float maximumModifier =
        weights.x * 1.08 +
        weights.y * 1.38 +
        weights.z * 1.48;

    return mix(
        minimumModifier,
        maximumModifier,
        fineResponse);
}


// =============================================================
// DENSITY CORE REMAP
// =============================================================
//
// This is the second important fix.
//
// It suppresses weak low-density haze and keeps stronger cloud cores.
// Without this, long ray paths through "almost empty" cloud still
// integrate into a uniform fog bank.

float densityCoreRemap(
    float rawDensity,
    vec4 weather)
{
    vec3 weights =
        cloudTypeWeights(weather);

    float cutoff =
        weights.x * 0.12 +
        weights.y * 0.22 +
        weights.z * 0.18;

    float exponent =
        weights.x * 1.2 +
        weights.y * 1.8 +
        weights.z * 1.6;

    float core =
        remap01(
            rawDensity,
            cutoff,
            1.0);

    return pow(core, exponent);
}


// =============================================================
// FULL CLOUD DENSITY
// =============================================================

float sampleCloudDensity(
    vec3 samplePositionKm,
    float sampleDistanceKm,
    float rayAngularFootprint,
    float stepLengthKm)
{
    vec4 weather =
        sampleWeather(samplePositionKm);

    float localCoverage =
        calculateLocalCoverage(weather);

    if (localCoverage <= 0.01)
    {
        return 0.0;
    }

    float radiusKm =
        length(samplePositionKm);

    float normalizedHeight =
        normalizedCloudHeight(radiusKm);

    float heightProfile =
        cloudHeightProfile(
            radiusKm,
            localCoverage,
            weather);

    if (heightProfile <= 0.0001)
    {
        return 0.0;
    }

    float weatherMask =
        smoothstep(
            0.10,
            0.45,
            localCoverage);

    if (weatherMask <= 0.0001)
    {
        return 0.0;
    }

    float physicalCoarsePeriodKm =
        coarsePeriodForWeather(weather);

    float coarseVerticalScale =
        coarseVerticalScaleForWeather(weather);

    float fineVerticalScale =
        fineVerticalScaleForWeather(weather);

    float coarseShape =
        sampleCoarseShape(
            samplePositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            physicalCoarsePeriodKm,
            coarseVerticalScale);

    float coarseThreshold =
        coarseThresholdForWeather(
            localCoverage,
            weather,
            normalizedHeight);

    float coarseDensity =
        remap01(
            coarseShape,
            coarseThreshold,
            coarseThreshold + 0.14);

    coarseDensity =
        coarseDensity * coarseDensity * (3.0 - 2.0 * coarseDensity);

    coarseDensity *= weatherMask;

    if (coarseDensity <= 0.0001)
    {
        return 0.0;
    }

    float fineShape =
        sampleFineShape(
            samplePositionKm,
            sampleDistanceKm,
            rayAngularFootprint,
            stepLengthKm,
            fineVerticalScale);

    float fineModifier =
        fineShapeModifier(
            fineShape,
            weather);

    float detailWeight =
        fineShapeWeight(
            sampleDistanceKm,
            rayAngularFootprint);

    float shapeDensity =
        coarseDensity *
        mix(
            1.0,
            fineModifier,
            detailWeight);

    shapeDensity =
        densityCoreRemap(
            shapeDensity,
            weather);

    float density =
        shapeDensity *
        heightProfile *
        weatherMask *
        cloudDensityMultiplier;

    return clamp(density, 0.0, 1.5);
}


// =============================================================
// SHADOW DENSITY
// =============================================================

float sampleCloudShadowDensity(
    vec3 samplePositionKm,
    float shadowStepLengthKm)
{
    vec4 weather =
        sampleWeather(samplePositionKm);

    float localCoverage =
        calculateLocalCoverage(weather);

    if (localCoverage <= 0.01)
    {
        return 0.0;
    }

    float radiusKm =
        length(samplePositionKm);

    float normalizedHeight =
        normalizedCloudHeight(radiusKm);

    float heightProfile =
        cloudHeightProfile(
            radiusKm,
            localCoverage,
            weather);

    if (heightProfile <= 0.0001)
    {
        return 0.0;
    }

    float weatherMask =
        smoothstep(
            0.10,
            0.45,
            localCoverage);

    if (weatherMask <= 0.0001)
    {
        return 0.0;
    }

    float physicalCoarsePeriodKm =
        coarsePeriodForWeather(weather);

    float coarseVerticalScale =
        coarseVerticalScaleForWeather(weather);

    float coarseShape =
        sampleCoarseShape(
            samplePositionKm,
            0.0,
            0.0,
            shadowStepLengthKm,
            physicalCoarsePeriodKm,
            coarseVerticalScale);

    float coarseThreshold =
        coarseThresholdForWeather(
            localCoverage,
            weather,
            normalizedHeight);

    float coarseDensity =
        remap01(
            coarseShape,
            coarseThreshold,
            coarseThreshold + 0.14);

    coarseDensity =
        coarseDensity * coarseDensity * (3.0 - 2.0 * coarseDensity);

    float rawDensity =
        coarseDensity *
        heightProfile *
        weatherMask;

    float density =
        densityCoreRemap(
            rawDensity,
            weather) *
        cloudDensityMultiplier;

    return clamp(density, 0.0, 1.5);
}


// =============================================================
// CLOUD -> SUN LIGHT MARCH
// =============================================================

float sunTransmittanceAt(
    vec3 samplePositionKm)
{
    if (planetBlocksSun(samplePositionKm))
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
        max(outerFarKm, 0.0);

    if (marchDistanceKm <= 0.001)
    {
        return 1.0;
    }

    float stepLengthKm =
        marchDistanceKm /
        float(CLOUD_LIGHT_SAMPLE_COUNT);

    float opticalDepth = 0.0;

    for (int i = 0; i < CLOUD_LIGHT_SAMPLE_COUNT; ++i)
    {
        float distanceKm =
            (float(i) + 0.5) *
            stepLengthKm;

        vec3 positionKm =
            samplePositionKm +
            sunDirection * distanceKm;

        float density =
            sampleCloudShadowDensity(
                positionKm,
                stepLengthKm);

        opticalDepth +=
            density *
            cloudExtinctionPerKm *
            shadowExtinctionMultiplier *
            stepLengthKm;

        if (opticalDepth > 12.0)
        {
            return 0.0;
        }
    }

    return exp(-opticalDepth);
}


// =============================================================
// PHASE FUNCTION
// =============================================================

float henyeyGreenstein(
    float cosineTheta,
    float g)
{
    float gSquared = g * g;

    float denominator =
        1.0 +
        gSquared -
        2.0 * g * cosineTheta;

    denominator = max(denominator, 0.0001);

    return
        (1.0 - gSquared) /
        (4.0 * PI * pow(denominator, 1.5));
}


float cloudPhase(
    vec3 viewRayDirection)
{
    float cosineTheta =
        clamp(
            dot(viewRayDirection, sunDirection),
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
// CAMERA RAY MARCH
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
    float startKm = segmentKm.x;
    float endKm = segmentKm.y;

    if (sceneDistanceKm > 0.0)
    {
        endKm = min(endKm, sceneDistanceKm);
    }

    if (endKm <= startKm)
    {
        return;
    }

    float stepLengthKm =
        (endKm - startKm) /
        float(CLOUD_SAMPLE_COUNT);

    float phase =
        cloudPhase(direction);

    for (int i = 0; i < CLOUD_SAMPLE_COUNT; ++i)
    {
        float sampleDistanceKm =
            startKm +
            (float(i) + 0.5) *
            stepLengthKm;

        vec3 samplePositionKm =
            originKm +
            direction * sampleDistanceKm;

        float density =
            sampleCloudDensity(
                samplePositionKm,
                sampleDistanceKm,
                rayAngularFootprint,
                stepLengthKm);

        if (density <= 0.0001)
        {
            continue;
        }

        float opticalDepth =
            cloudExtinctionPerKm *
            density *
            stepLengthKm;

        float stepTransmittance =
            exp(-opticalDepth);

        float scatteredFraction =
            1.0 - stepTransmittance;

        float sunTransmittance =
            sunTransmittanceAt(samplePositionKm);

        vec3 incidentSunlight =
            sunRadiance * sunTransmittance;

        vec3 scatteredRadiance =
            incidentSunlight *
            phase *
            cloudScatteringAlbedo *
            cloudLightingIntensity;

        accumulatedRadiance +=
            accumulatedTransmittance *
            scatteredRadiance *
            scatteredFraction;

        accumulatedTransmittance *=
            stepTransmittance;

        if (accumulatedTransmittance < 0.001)
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
        texture(sceneColorTexture, vUV).rgb;

    float sceneDistanceWorld =
        texture(sceneLinearDepthTexture, vUV).r;

    float sceneDistanceKm =
        sceneDistanceWorld > 0.0
            ? sceneDistanceWorld * kmPerWorldUnit
            : 0.0;

    vec3 rayDirection =
        reconstructWorldRay(vUV);

    vec3 rayDerivativeX =
        dFdx(rayDirection);

    vec3 rayDerivativeY =
        dFdy(rayDirection);

    float rayAngularFootprint =
        max(
            length(rayDerivativeX),
            length(rayDerivativeY));

    vec3 originKm =
        (cameraPositionWorld - planetCenterWorld) *
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
        outColor = vec4(sceneColor, 1.0);
        return;
    }

    vec3 accumulatedRadiance = vec3(0.0);
    float accumulatedTransmittance = 1.0;

    marchCloudSegment(
        originKm,
        rayDirection,
        segmentA,
        sceneDistanceKm,
        rayAngularFootprint,
        accumulatedRadiance,
        accumulatedTransmittance);

    if (segmentCount > 1 &&
        accumulatedTransmittance > 0.001)
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
            accumulatedRadiance +
            sceneColor * accumulatedTransmittance,
            1.0);
}