#version 450 core

layout(depth_less) out float gl_FragDepth;
layout(location = 0)
out vec4 outColor;


layout(location = 1)
out float outLinearDepth;


in VS_OUT
{
    vec3 worldPosition;

    vec3 worldNormal;

    vec3 planetDirection;
}
fsIn;


// =============================================================
// CAMERA / PRIMARY STAR
// =============================================================

uniform vec3 cameraPosition;


uniform vec3 sunDirection;

uniform vec3 sunRadiance;


uniform float sunAngularRadiusRadians;


// =============================================================
// TIME
// =============================================================

uniform float timeSeconds;


// =============================================================
// OCEAN
// =============================================================

uniform vec3 deepOceanColor;

uniform vec3 shallowOceanColor;

uniform float oceanRoughness;


uniform float oceanWaveScale;

uniform float oceanWaveStrength;

uniform float oceanWaveSpeed;


// =============================================================
// LAND
// =============================================================

uniform vec3 lowLandColor;

uniform vec3 highLandColor;

uniform float landRoughness;


// =============================================================
// PROCEDURAL PLANET
// =============================================================

uniform float continentScale;

uniform float detailScale;

uniform float oceanLevel;

uniform float coastWidth;

uniform float planetSeed;


// =============================================================
// ATMOSPHERE
// =============================================================

uniform int atmosphereEnabled;


uniform sampler2D atmosphereTransmittanceLut;

uniform sampler2D atmosphereSkyIrradianceLut;

uniform sampler2D atmosphereMultipleScatteringLut;


uniform vec3 atmospherePlanetCenterWorld;


uniform float atmosphereKmPerWorldUnit;

uniform float atmosphereBottomRadiusKm;

uniform float atmosphereTopRadiusKm;


uniform vec3 atmosphereRayleighScatteringPerKm;

uniform float atmosphereRayleighScaleHeightKm;


uniform vec3 atmosphereMieScatteringPerKm;

uniform vec3 atmosphereMieExtinctionPerKm;

uniform float atmosphereMieScaleHeightKm;

uniform float atmosphereMieAnisotropy;


uniform vec3 atmosphereOzoneAbsorptionPerKm;

uniform float atmosphereOzoneCenterHeightKm;

uniform float atmosphereOzoneHalfWidthKm;


// =============================================================
// CONSTANTS
// =============================================================

const float PI =
    3.14159265359;


const float GOLDEN_ANGLE =
    2.39996322973;


const int SUN_DISK_SAMPLE_COUNT =
    12;


const int SKY_REFLECTION_SAMPLE_COUNT =
    8;


// =============================================================
// PROCEDURAL NOISE
// =============================================================

float hash31(
    vec3 p)
{
    p =
        fract(
            p *
            0.1031);


    p +=
        dot(
            p,
            p.yzx +
            33.33);


    return
        fract(
            (
                p.x +
                p.y
            )
            *
            p.z);
}


float valueNoise(
    vec3 p)
{
    vec3 cell =
        floor(
            p);


    vec3 f =
        fract(
            p);


    vec3 u =
        f *
        f *
        (
            3.0
            -
            2.0 *
            f
        );


    float n000 =
        hash31(
            cell +
            vec3(
                0.0,
                0.0,
                0.0));


    float n100 =
        hash31(
            cell +
            vec3(
                1.0,
                0.0,
                0.0));


    float n010 =
        hash31(
            cell +
            vec3(
                0.0,
                1.0,
                0.0));


    float n110 =
        hash31(
            cell +
            vec3(
                1.0,
                1.0,
                0.0));


    float n001 =
        hash31(
            cell +
            vec3(
                0.0,
                0.0,
                1.0));


    float n101 =
        hash31(
            cell +
            vec3(
                1.0,
                0.0,
                1.0));


    float n011 =
        hash31(
            cell +
            vec3(
                0.0,
                1.0,
                1.0));


    float n111 =
        hash31(
            cell +
            vec3(
                1.0,
                1.0,
                1.0));


    float nx00 =
        mix(
            n000,
            n100,
            u.x);


    float nx10 =
        mix(
            n010,
            n110,
            u.x);


    float nx01 =
        mix(
            n001,
            n101,
            u.x);


    float nx11 =
        mix(
            n011,
            n111,
            u.x);


    float nxy0 =
        mix(
            nx00,
            nx10,
            u.y);


    float nxy1 =
        mix(
            nx01,
            nx11,
            u.y);


    return
        mix(
            nxy0,
            nxy1,
            u.z);
}


float fbm(
    vec3 p)
{
    float result =
        0.0;


    float amplitude =
        0.5;


    float totalAmplitude =
        0.0;


    for (int octave = 0;
         octave < 5;
         ++octave)
    {
        result +=
            valueNoise(
                p)
            *
            amplitude;


        totalAmplitude +=
            amplitude;


        p =
            p *
            2.03
            +
            vec3(
                17.1,
                31.7,
                11.3);


        amplitude *=
            0.5;
    }


    return
        result
        /
        max(
            totalAmplitude,
            0.0001);
}


// =============================================================
// OCEAN SURFACE NORMAL DETAIL
// =============================================================
//
// The ocean uses a planet-space statistical wave field rather than
// camera-space ripples or coherent latitude/longitude rows.
//
// We derive normals from the gradient of that field. The field contains
// several scales so the surface has broad swell, medium chop, and finer
// detail. Screen-space filtering fades frequencies that are too small to
// resolve, and altitude filtering removes readable wave shapes in orbit.
//
// These are still lighting normals only. Real displaced water geometry is
// a later local/surface-LOD feature.

vec3 rotateAroundAxis(
    vec3 value,
    vec3 axis,
    float angle)
{
    axis =
        normalize(
            axis);


    float c =
        cos(
            angle);


    float s =
        sin(
            angle);


    return
        value * c
        +
        cross(
            axis,
            value) * s
        +
        axis
        * dot(
            axis,
            value)
        * (1.0 - c);
}


void buildOceanTangentBasis(
    vec3 surfaceNormal,
    out vec3 tangent,
    out vec3 bitangent)
{
    vec3 helper =
        abs(
            surfaceNormal.y)
        <
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


    tangent =
        normalize(
            cross(
                helper,
                surfaceNormal));


    bitangent =
        normalize(
            cross(
                surfaceNormal,
                tangent));
}


float oceanNoiseBand(
    vec3 direction,
    float frequency,
    float amplitude,
    vec3 offset,
    float directionFootprint)
{
    float bandFootprint =
        directionFootprint
        *
        frequency;


    float visibility =
        1.0
        -
        smoothstep(
            0.35,
            1.35,
            bandFootprint);


    if (visibility <=
        0.000001)
    {
        return
            0.0;
    }


    float centeredNoise =
        valueNoise(
            direction
            *
            frequency
            +
            offset)
        *
        2.0
        -
        1.0;


    return
        centeredNoise
        *
        amplitude
        *
        visibility
        /
        max(
            frequency,
            1.0);
}


float oceanSurfaceField(
    vec3 sphereDirection,
    float directionFootprint)
{
    float baseFrequency =
        max(
            oceanWaveScale
            *
            1.8,
            1.0);


    float drift =
        timeSeconds
        *
        oceanWaveSpeed;


    vec3 directionA =
        rotateAroundAxis(
            sphereDirection,
            vec3(
                0.31,
                0.91,
                0.27),
            drift * 0.000120);


    vec3 directionB =
        rotateAroundAxis(
            sphereDirection,
            vec3(
               -0.63,
                0.22,
                0.74),
            drift * -0.000170);


    vec3 directionC =
        rotateAroundAxis(
            sphereDirection,
            vec3(
                0.71,
               -0.54,
                0.45),
            drift * 0.000230);


    vec3 directionD =
        rotateAroundAxis(
            sphereDirection,
            vec3(
               -0.18,
                0.79,
               -0.58),
            drift * -0.000310);


    vec3 directionE =
        rotateAroundAxis(
            sphereDirection,
            vec3(
                0.52,
                0.18,
               -0.83),
            drift * 0.000430);


    vec3 directionF =
        rotateAroundAxis(
            sphereDirection,
            vec3(
               -0.76,
                0.61,
                0.22),
            drift * -0.000580);


    float field =
        0.0;


    field +=
        oceanNoiseBand(
            directionA,
            baseFrequency * 0.42,
            0.52,
            vec3(
                11.3,
               -17.1,
                 5.7)
            +
            planetSeed
            *
            vec3(
                0.71,
                1.13,
                1.79),
            directionFootprint);


    field +=
        oceanNoiseBand(
            directionB,
            baseFrequency * 0.78,
            0.34,
            vec3(
               -23.9,
                 8.4,
                19.7)
            +
            planetSeed
            *
            vec3(
                1.37,
                0.53,
                2.11),
            directionFootprint);


    field +=
        oceanNoiseBand(
            directionC,
            baseFrequency * 1.35,
            0.22,
            vec3(
                31.2,
                14.6,
               -27.5)
            +
            planetSeed
            *
            vec3(
                0.43,
                1.91,
                0.97),
            directionFootprint);


    field +=
        oceanNoiseBand(
            directionD,
            baseFrequency * 2.35,
            0.14,
            vec3(
               -41.7,
                29.1,
                13.8)
            +
            planetSeed
            *
            vec3(
                2.23,
                0.67,
                1.29),
            directionFootprint);


    field +=
        oceanNoiseBand(
            directionE,
            baseFrequency * 4.10,
            0.085,
            vec3(
                53.2,
               -36.7,
                21.4)
            +
            planetSeed
            *
            vec3(
                1.61,
                2.41,
                0.37),
            directionFootprint);


    field +=
        oceanNoiseBand(
            directionF,
            baseFrequency * 7.20,
            0.045,
            vec3(
               -61.3,
                44.8,
               -32.6)
            +
            planetSeed
            *
            vec3(
                0.89,
                1.47,
                2.73),
            directionFootprint);


    return
        field;
}


float oceanResolvedDetailVisibility(
    vec3 sphereDirection)
{
    float directionFootprint =
        max(
            length(
                dFdx(
                    sphereDirection)),
            length(
                dFdy(
                    sphereDirection)));


    float effectiveBaseFrequency =
        max(
            oceanWaveScale
            *
            1.8,
            1.0);


    float pixelVisibility =
        1.0
        -
        smoothstep(
            0.45,
            1.60,
            directionFootprint
            *
            effectiveBaseFrequency);


    float altitudeVisibility =
        1.0;


    if (atmosphereEnabled !=
        0)
    {
        float cameraRadiusKm =
            length(
                (
                    cameraPosition
                    -
                    atmospherePlanetCenterWorld
                )
                *
                atmosphereKmPerWorldUnit);


        float cameraAltitudeKm =
            max(
                cameraRadiusKm
                -
                atmosphereBottomRadiusKm,
                0.0);


        altitudeVisibility =
            1.0
            -
            smoothstep(
                180.0,
                1100.0,
                cameraAltitudeKm);
    }


    return
        pixelVisibility
        *
        altitudeVisibility;
}


vec3 oceanWaveNormal(
    vec3 geometricNormal,
    vec3 sphereDirection)
{
    float detailVisibility =
        oceanResolvedDetailVisibility(
            sphereDirection);


    if (detailVisibility <=
        0.000001
        ||
        oceanWaveStrength <=
        0.000001)
    {
        return
            geometricNormal;
    }


    float directionFootprint =
        max(
            length(
                dFdx(
                    sphereDirection)),
            length(
                dFdy(
                    sphereDirection)));


    float effectiveBaseFrequency =
        max(
            oceanWaveScale
            *
            1.8,
            1.0);


    vec3 tangent;

    vec3 bitangent;


    buildOceanTangentBasis(
        geometricNormal,
        tangent,
        bitangent);


    float sampleStep =
        max(
            0.22
            /
            effectiveBaseFrequency,
            directionFootprint
            *
            0.40);


    vec3 directionTangentPositive =
        normalize(
            sphereDirection
            +
            tangent
            *
            sampleStep);


    vec3 directionTangentNegative =
        normalize(
            sphereDirection
            -
            tangent
            *
            sampleStep);


    vec3 directionBitangentPositive =
        normalize(
            sphereDirection
            +
            bitangent
            *
            sampleStep);


    vec3 directionBitangentNegative =
        normalize(
            sphereDirection
            -
            bitangent
            *
            sampleStep);


    float tangentPositive =
        oceanSurfaceField(
            directionTangentPositive,
            directionFootprint);


    float tangentNegative =
        oceanSurfaceField(
            directionTangentNegative,
            directionFootprint);


    float bitangentPositive =
        oceanSurfaceField(
            directionBitangentPositive,
            directionFootprint);


    float bitangentNegative =
        oceanSurfaceField(
            directionBitangentNegative,
            directionFootprint);


    float slopeTangent =
        (
            tangentPositive
            -
            tangentNegative
        )
        /
        (
            2.0
            *
            sampleStep
        );


    float slopeBitangent =
        (
            bitangentPositive
            -
            bitangentNegative
        )
        /
        (
            2.0
            *
            sampleStep
        );


    float slopeGain =
        oceanWaveStrength
        *
        1.45;


    vec3 perturbedNormal =
        normalize(
            geometricNormal
            -
            tangent
            *
            slopeTangent
            *
            slopeGain
            -
            bitangent
            *
            slopeBitangent
            *
            slopeGain);


    return
        normalize(
            mix(
                geometricNormal,
                perturbedNormal,
                detailVisibility));
}


float oceanFacetVariation(
    vec3 geometricNormal,
    vec3 waveNormal,
    vec3 viewDirection)
{
    float geometricFacing =
        clamp(
            dot(
                geometricNormal,
                viewDirection),
            0.0,
            1.0);


    float waveFacing =
        clamp(
            dot(
                waveNormal,
                viewDirection),
            0.0,
            1.0);


    return
        clamp(
            (
                waveFacing
                -
                geometricFacing
            )
            *
            2.5,
            -0.10,
            0.10);
}


// =============================================================
// GGX
// =============================================================

float distributionGGX(
    vec3 N,
    vec3 H,
    float materialRoughness)
{
    float a =
        materialRoughness
        *
        materialRoughness;


    float a2 =
        a
        *
        a;


    float NdotH =
        max(
            dot(
                N,
                H),
            0.0);


    float NdotH2 =
        NdotH
        *
        NdotH;


    float denominator =
        NdotH2
        *
        (
            a2
            -
            1.0
        )
        +
        1.0;


    denominator =
        PI
        *
        denominator
        *
        denominator;


    return
        a2
        /
        max(
            denominator,
            0.000001);
}


float geometrySchlickGGX(
    float NdotV,
    float materialRoughness)
{
    float r =
        materialRoughness
        +
        1.0;


    float k =
        (
            r
            *
            r
        )
        /
        8.0;


    return
        NdotV
        /
        (
            NdotV
            *
            (
                1.0
                -
                k
            )
            +
            k
        );
}


float geometrySmith(
    vec3 N,
    vec3 V,
    vec3 L,
    float materialRoughness)
{
    return
        geometrySchlickGGX(
            max(
                dot(
                    N,
                    V),
                0.0),
            materialRoughness)
        *
        geometrySchlickGGX(
            max(
                dot(
                    N,
                    L),
                0.0),
            materialRoughness);
}


vec3 fresnelSchlick(
    float cosTheta,
    vec3 F0)
{
    return
        F0
        +
        (
            vec3(1.0)
            -
            F0
        )
        *
        pow(
            1.0
            -
            cosTheta,
            5.0);
}


vec3 evaluateDirectionalSpecular(
    vec3 N,
    vec3 V,
    vec3 L,
    vec3 F0,
    float materialRoughness)
{
    float NdotL =
        max(
            dot(
                N,
                L),
            0.0);


    float NdotV =
        max(
            dot(
                N,
                V),
            0.0);


    if (NdotL <=
            0.0
        ||
        NdotV <=
            0.0)
    {
        return
            vec3(0.0);
    }


    vec3 H =
        normalize(
            V +
            L);


    float NDF =
        distributionGGX(
            N,
            H,
            materialRoughness);


    float G =
        geometrySmith(
            N,
            V,
            L,
            materialRoughness);


    vec3 F =
        fresnelSchlick(
            max(
                dot(
                    H,
                    V),
                0.0),
            F0);


    vec3 specular =
        (
            NDF
            *
            G
            *
            F
        )
        /
        max(
            4.0
            *
            NdotV
            *
            NdotL,
            0.0001);


    return
        specular
        *
        NdotL;
}


// =============================================================
// FINITE STELLAR DISK
// =============================================================

void buildStarBasis(
    vec3 centerDirection,
    out vec3 tangent,
    out vec3 bitangent)
{
    vec3 helper =
        abs(
            centerDirection.y)
        <
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


    tangent =
        normalize(
            cross(
                helper,
                centerDirection));


    bitangent =
        normalize(
            cross(
                centerDirection,
                tangent));
}


vec3 stellarDiskDirection(
    vec3 centerDirection,
    vec3 tangent,
    vec3 bitangent,
    float diskRadius,
    float azimuth)
{
    float cosTheta =
        cos(
            diskRadius);


    float sinTheta =
        sin(
            diskRadius);


    vec3 radialDirection =
        tangent
        *
        cos(
            azimuth)
        +
        bitangent
        *
        sin(
            azimuth);


    return
        normalize(
            centerDirection
            *
            cosTheta
            +
            radialDirection
            *
            sinTheta);
}


vec3 finiteStarSpecular(
    vec3 N,
    vec3 V,
    vec3 centerLightDirection,
    vec3 F0,
    float materialRoughness)
{
    if (sunAngularRadiusRadians <=
        0.000001)
    {
        return
            evaluateDirectionalSpecular(
                N,
                V,
                centerLightDirection,
                F0,
                materialRoughness);
    }


    vec3 tangent;

    vec3 bitangent;


    buildStarBasis(
        centerLightDirection,
        tangent,
        bitangent);


    vec3 accumulatedSpecular =
        vec3(0.0);


    for (int sampleIndex = 0;
         sampleIndex < SUN_DISK_SAMPLE_COUNT;
         ++sampleIndex)
    {
        float u =
            (
                float(
                    sampleIndex)
                +
                0.5
            )
            /
            float(
                SUN_DISK_SAMPLE_COUNT);


        float radialFraction =
            sqrt(
                u);


        float angularRadius =
            sunAngularRadiusRadians
            *
            radialFraction;


        float azimuth =
            float(
                sampleIndex)
            *
            GOLDEN_ANGLE;


        vec3 sampleDirection =
            stellarDiskDirection(
                centerLightDirection,
                tangent,
                bitangent,
                angularRadius,
                azimuth);


        accumulatedSpecular +=
            evaluateDirectionalSpecular(
                N,
                V,
                sampleDirection,
                F0,
                materialRoughness);
    }


    return
        accumulatedSpecular
        /
        float(
            SUN_DISK_SAMPLE_COUNT);
}


// =============================================================
// ATMOSPHERE DENSITY
// =============================================================

float atmosphereRayleighDensity(
    float altitudeKm)
{
    return
        exp(
            -max(
                altitudeKm,
                0.0)
            /
            atmosphereRayleighScaleHeightKm);
}


float atmosphereMieDensity(
    float altitudeKm)
{
    return
        exp(
            -max(
                altitudeKm,
                0.0)
            /
            atmosphereMieScaleHeightKm);
}


float atmosphereOzoneDensity(
    float altitudeKm)
{
    float distanceFromLayer =
        abs(
            altitudeKm
            -
            atmosphereOzoneCenterHeightKm);


    return
        max(
            0.0,
            1.0
            -
            distanceFromLayer
            /
            atmosphereOzoneHalfWidthKm);
}


vec3 atmosphereScatteringAtAltitude(
    float altitudeKm)
{
    return
        atmosphereRayleighScatteringPerKm
        *
        atmosphereRayleighDensity(
            altitudeKm)
        +
        atmosphereMieScatteringPerKm
        *
        atmosphereMieDensity(
            altitudeKm);
}


vec3 atmosphereExtinctionAtAltitude(
    float altitudeKm)
{
    return
        atmosphereRayleighScatteringPerKm
        *
        atmosphereRayleighDensity(
            altitudeKm)
        +
        atmosphereMieExtinctionPerKm
        *
        atmosphereMieDensity(
            altitudeKm)
        +
        atmosphereOzoneAbsorptionPerKm
        *
        atmosphereOzoneDensity(
            altitudeKm);
}


// =============================================================
// ATMOSPHERE SPHERE INTERSECTIONS
// =============================================================

bool atmosphereRaySphereIntervalKm(
    vec3 originKm,
    vec3 direction,
    float sphereRadiusKm,
    out float tNearKm,
    out float tFarKm)
{
    float scale =
        atmosphereBottomRadiusKm;


    vec3 origin =
        originKm
        /
        scale;


    float radius =
        sphereRadiusKm
        /
        scale;


    float b =
        dot(
            origin,
            direction);


    float c =
        dot(
            origin,
            origin)
        -
        radius
        *
        radius;


    float discriminant =
        b
        *
        b
        -
        c;


    if (discriminant <
        0.0)
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
        (
            -b
            -
            root
        )
        *
        scale;


    tFarKm =
        (
            -b
            +
            root
        )
        *
        scale;


    return
        tFarKm >
        0.0;
}


float atmosphereNearestSphereIntersectionKm(
    vec3 originKm,
    vec3 direction,
    float sphereRadiusKm)
{
    float tNearKm;

    float tFarKm;


    if (!atmosphereRaySphereIntervalKm(
            originKm,
            direction,
            sphereRadiusKm,
            tNearKm,
            tFarKm))
    {
        return
            -1.0;
    }


    if (tNearKm >
        0.000001)
    {
        return
            tNearKm;
    }


    if (tFarKm >
        0.000001)
    {
        return
            tFarKm;
    }


    return
        -1.0;
}


bool atmosphereRayHitsGroundKm(
    vec3 positionKm,
    vec3 direction)
{
    return
        atmosphereNearestSphereIntersectionKm(
            positionKm,
            direction,
            atmosphereBottomRadiusKm)
        >
        0.000001;
}


// =============================================================
// TRANSMITTANCE LUT
// =============================================================

vec2 atmosphereTransmittanceUv(
    vec3 positionKm,
    vec3 direction)
{
    float radius =
        length(
            positionKm);


    vec3 localUp =
        positionKm
        /
        radius;


    float mu =
        clamp(
            dot(
                localUp,
                direction),
            -1.0,
            1.0);


    float H =
        sqrt(
            max(
                atmosphereTopRadiusKm
                *
                atmosphereTopRadiusKm
                -
                atmosphereBottomRadiusKm
                *
                atmosphereBottomRadiusKm,
                0.0));


    float rho =
        sqrt(
            max(
                radius
                *
                radius
                -
                atmosphereBottomRadiusKm
                *
                atmosphereBottomRadiusKm,
                0.0));


    float discriminant =
        radius
        *
        radius
        *
        (
            mu
            *
            mu
            -
            1.0
        )
        +
        atmosphereTopRadiusKm
        *
        atmosphereTopRadiusKm;


    float distanceToTop =
        -radius
        *
        mu
        +
        sqrt(
            max(
                discriminant,
                0.0));


    float distanceMinimum =
        atmosphereTopRadiusKm
        -
        radius;


    float distanceMaximum =
        rho
        +
        H;


    float xMu =
        (
            distanceToTop
            -
            distanceMinimum
        )
        /
        max(
            distanceMaximum
            -
            distanceMinimum,
            0.000001);


    float xRadius =
        rho
        /
        max(
            H,
            0.000001);


    vec2 parameterUv =
        clamp(
            vec2(
                xMu,
                xRadius),
            vec2(0.0),
            vec2(1.0));


    vec2 size =
        vec2(
            textureSize(
                atmosphereTransmittanceLut,
                0));


    return
        (
            parameterUv
            *
            (
                size
                -
                vec2(1.0)
            )
            +
            vec2(0.5)
        )
        /
        size;
}


vec3 atmosphereSampleTransmittanceToSpace(
    vec3 positionKm,
    vec3 direction)
{
    if (atmosphereRayHitsGroundKm(
            positionKm,
            direction))
    {
        return
            vec3(0.0);
    }


    return
        textureLod(
            atmosphereTransmittanceLut,
            atmosphereTransmittanceUv(
                positionKm,
                direction),
            0.0).rgb;
}


// =============================================================
// MULTIPLE SCATTERING LUT
// =============================================================

vec3 atmosphereSampleMultipleScattering(
    vec3 positionKm,
    vec3 directionToSun)
{
    float radius =
        length(
            positionKm);


    vec3 localUp =
        positionKm
        /
        radius;


    float sunMu =
        clamp(
            dot(
                localUp,
                directionToSun),
            -1.0,
            1.0);


    float u =
        sunMu
        *
        0.5
        +
        0.5;


    float altitudeKm =
        radius
        -
        atmosphereBottomRadiusKm;


    float atmosphereThicknessKm =
        atmosphereTopRadiusKm
        -
        atmosphereBottomRadiusKm;


    float v =
        clamp(
            altitudeKm
            /
            atmosphereThicknessKm,
            0.0,
            1.0);


    return
        textureLod(
            atmosphereMultipleScatteringLut,
            vec2(
                u,
                v),
            0.0).rgb;
}


// =============================================================
// ATMOSPHERE PHASE FUNCTIONS
// =============================================================

float atmosphereRayleighPhase(
    float cosTheta)
{
    return
        3.0
        /
        (
            16.0
            *
            PI
        )
        *
        (
            1.0
            +
            cosTheta
            *
            cosTheta
        );
}


float atmosphereMiePhase(
    float cosTheta,
    float g)
{
    float g2 =
        g
        *
        g;


    float denominator =
        1.0
        +
        g2
        -
        2.0
        *
        g
        *
        cosTheta;


    return
        (
            1.0
            -
            g2
        )
        /
        (
            4.0
            *
            PI
            *
            pow(
                max(
                    denominator,
                    0.0001),
                1.5)
        );
}


// =============================================================
// LOCAL DIRECTIONAL SKY RADIANCE
// =============================================================

vec3 atmosphereIntegrateLocalSkyRadiance(
    vec3 positionKm,
    vec3 rayDirection,
    vec3 directionToSun)
{
    if (atmosphereEnabled ==
        0)
    {
        return
            vec3(0.0);
    }


    if (atmosphereRayHitsGroundKm(
            positionKm,
            rayDirection))
    {
        return
            vec3(0.0);
    }


    float distanceToTopKm =
        atmosphereNearestSphereIntersectionKm(
            positionKm,
            rayDirection,
            atmosphereTopRadiusKm);


    if (distanceToTopKm <=
        0.000001)
    {
        return
            vec3(0.0);
    }


    float scatteringAngleCosine =
        clamp(
            dot(
                rayDirection,
                directionToSun),
            -1.0,
            1.0);


    float phaseRayleigh =
        atmosphereRayleighPhase(
            scatteringAngleCosine);


    float phaseMie =
        atmosphereMiePhase(
            scatteringAngleCosine,
            atmosphereMieAnisotropy);


    vec3 accumulatedRadiance =
        vec3(0.0);


    vec3 viewTransmittance =
        vec3(1.0);


    for (int sampleIndex = 0;
         sampleIndex < SKY_REFLECTION_SAMPLE_COUNT;
         ++sampleIndex)
    {
        float u0 =
            float(
                sampleIndex)
            /
            float(
                SKY_REFLECTION_SAMPLE_COUNT);


        float u1 =
            float(
                sampleIndex
                +
                1)
            /
            float(
                SKY_REFLECTION_SAMPLE_COUNT);


        float mapped0 =
            u0
            *
            u0;


        float mapped1 =
            u1
            *
            u1;


        float sampleStartKm =
            mapped0
            *
            distanceToTopKm;


        float sampleEndKm =
            mapped1
            *
            distanceToTopKm;


        float stepLengthKm =
            sampleEndKm
            -
            sampleStartKm;


        float sampleDistanceKm =
            (
                sampleStartKm
                +
                sampleEndKm
            )
            *
            0.5;


        vec3 samplePositionKm =
            positionKm
            +
            rayDirection
            *
            sampleDistanceKm;


        float altitudeKm =
            max(
                length(
                    samplePositionKm)
                -
                atmosphereBottomRadiusKm,
                0.0);


        float densityRayleigh =
            atmosphereRayleighDensity(
                altitudeKm);


        float densityMie =
            atmosphereMieDensity(
                altitudeKm);


        vec3 localExtinction =
            atmosphereExtinctionAtAltitude(
                altitudeKm);


        vec3 segmentTransmittance =
            exp(
                -localExtinction
                *
                stepLengthKm);


        vec3 midpointTransmittance =
            viewTransmittance
            *
            sqrt(
                segmentTransmittance);


        vec3 sunTransmittance =
            atmosphereSampleTransmittanceToSpace(
                samplePositionKm,
                directionToSun);


        vec3 directScattering =
            atmosphereRayleighScatteringPerKm
            *
            densityRayleigh
            *
            phaseRayleigh
            +
            atmosphereMieScatteringPerKm
            *
            densityMie
            *
            phaseMie;


        vec3 directSource =
            sunTransmittance
            *
            directScattering;


        vec3 multipleSource =
            atmosphereScatteringAtAltitude(
                altitudeKm)
            *
            atmosphereSampleMultipleScattering(
                samplePositionKm,
                directionToSun);


        accumulatedRadiance +=
            midpointTransmittance
            *
            (
                directSource
                +
                multipleSource
            )
            *
            stepLengthKm;


        viewTransmittance *=
            segmentTransmittance;
    }


    return
        accumulatedRadiance
        *
        sunRadiance;
}


// =============================================================
// DIFFUSE SKY IRRADIANCE
// =============================================================

vec2 atmosphereSkyIrradianceUv(
    float altitudeKm,
    float sunMu)
{
    float atmosphereThicknessKm =
        atmosphereTopRadiusKm
        -
        atmosphereBottomRadiusKm;


    float encodedSunMu =
        sign(
            sunMu)
        *
        sqrt(
            abs(
                sunMu));


    float u =
        encodedSunMu
        *
        0.5
        +
        0.5;


    float altitudeFraction =
        clamp(
            altitudeKm
            /
            max(
                atmosphereThicknessKm,
                0.000001),
            0.0,
            1.0);


    float v =
        sqrt(
            altitudeFraction);


    vec2 parameterUv =
        clamp(
            vec2(
                u,
                v),
            vec2(0.0),
            vec2(1.0));


    vec2 size =
        vec2(
            textureSize(
                atmosphereSkyIrradianceLut,
                0));


    return
        (
            parameterUv
            *
            (
                size
                -
                vec2(1.0)
            )
            +
            vec2(0.5)
        )
        /
        size;
}


vec3 atmosphereSkyIrradiance(
    vec3 worldPosition,
    vec3 directionToSun,
    out vec3 localUp)
{
    localUp =
        normalize(
            fsIn.worldNormal);


    if (atmosphereEnabled ==
        0)
    {
        return
            vec3(0.0);
    }


    vec3 positionKm =
        (
            worldPosition
            -
            atmospherePlanetCenterWorld
        )
        *
        atmosphereKmPerWorldUnit;


    float radiusKm =
        length(
            positionKm);


    if (radiusKm <=
        0.000001)
    {
        return
            vec3(0.0);
    }


    localUp =
        positionKm
        /
        radiusKm;


    float altitudeKm =
        clamp(
            radiusKm
            -
            atmosphereBottomRadiusKm,
            0.0,
            atmosphereTopRadiusKm
            -
            atmosphereBottomRadiusKm);


    float sunMu =
        clamp(
            dot(
                localUp,
                directionToSun),
            -1.0,
            1.0);


    return
        textureLod(
            atmosphereSkyIrradianceLut,
            atmosphereSkyIrradianceUv(
                altitudeKm,
                sunMu),
            0.0).rgb;
}


// =============================================================
// DIRECT STAR TRANSMITTANCE
// =============================================================

vec3 atmosphereSunTransmittance(
    vec3 worldPosition,
    vec3 directionToSun)
{
    if (atmosphereEnabled ==
        0)
    {
        return
            vec3(1.0);
    }


    vec3 positionKm =
        (
            worldPosition
            -
            atmospherePlanetCenterWorld
        )
        *
        atmosphereKmPerWorldUnit;


    float radiusKm =
        length(
            positionKm);


    if (radiusKm <=
        0.000001)
    {
        return
            vec3(1.0);
    }


    vec3 localUp =
        positionKm
        /
        radiusKm;


    radiusKm =
        max(
            radiusKm,
            atmosphereBottomRadiusKm
            +
            0.001);


    positionKm =
        localUp
        *
        radiusKm;


    return
        atmosphereSampleTransmittanceToSpace(
            positionKm,
            directionToSun);
}


// =============================================================
// HORIZON-STABLE OCEAN REFLECTION
// =============================================================

vec3 makeHorizonSafeReflectionDirection(
    vec3 rawDirection,
    vec3 smoothUp,
    out float skyReflectionWeight)
{
    float rawMu =
        clamp(
            dot(
                rawDirection,
                smoothUp),
            -1.0,
            1.0);


    skyReflectionWeight =
        smoothstep(
            -0.08,
             0.05,
            rawMu);


    vec3 tangent =
        rawDirection
        -
        smoothUp
        *
        rawMu;


    float tangentLengthSquared =
        dot(
            tangent,
            tangent);


    if (tangentLengthSquared <
        0.000001)
    {
        vec3 helper =
            abs(
                smoothUp.y)
            <
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


        tangent =
            normalize(
                cross(
                    helper,
                    smoothUp));
    }
    else
    {
        tangent *=
            inversesqrt(
                tangentLengthSquared);
    }


    const float minimumUpComponent =
        0.02;


    const float tangentComponent =
        sqrt(
            max(
                1.0
                -
                minimumUpComponent
                *
                minimumUpComponent,
                0.0));


    vec3 horizonSafeDirection =
        normalize(
            tangent
            *
            tangentComponent
            +
            smoothUp
            *
            minimumUpComponent);


    float useRawDirection =
        smoothstep(
            0.02,
            0.10,
            rawMu);


    return
        normalize(
            mix(
                horizonSafeDirection,
                rawDirection,
                useRawDirection));
}


// =============================================================
// MAIN
// =============================================================


// Ownership is independent of the atmosphere and uses the same planet-space extent.
uniform int oceanSurfacePass, oceanLocalCoverage;
uniform float geometricWaveVisibility, oceanGravity;
uniform float oceanOuterKm, oceanRadiusKm, planetRadiusWorld;
uniform vec3 planetCenterWorld, oceanAnchorNormal, oceanAnchorRight, oceanAnchorForward;
uniform mat4 planetInverseModel, view, projection;

// Aperiodic, metre-scale ripples; both spatial bands are pixel-footprint filtered.
float microHeight(vec3 p,float footprint) {
    vec3 drift=vec3(.001,.00013,.0004)*timeSeconds*oceanWaveSpeed;
    float a=1.0-smoothstep(.35,1.25,footprint*90.0);
    float b=1.0-smoothstep(.35,1.25,footprint*240.0);
    return a*(valueNoise((p+drift)*90.0+planetSeed)-.5)/90.0+
        b*.4*(valueNoise((p-drift*.7)*240.0+planetSeed*2.3)-.5)/240.0;
}
vec3 localOceanRipples(vec3 normal,vec3 direction) {
    vec3 p=normalize(direction)*oceanRadiusKm;
    float footprint=max(length(dFdx(p)),length(dFdy(p)));
    float h=max(.0008,footprint*.5);
    vec3 helper=abs(direction.y)<.95?vec3(0,1,0):vec3(1,0,0);
    vec3 t=normalize(cross(helper,direction)), b=cross(direction,t);
    float dx=(microHeight(p+t*h,footprint)-microHeight(p-t*h,footprint))/(2.0*h);
    float dz=(microHeight(p+b*h,footprint)-microHeight(p-b*h,footprint))/(2.0*h);
    vec3 wt=normalize(transpose(mat3(planetInverseModel))*t);
    vec3 wb=normalize(transpose(mat3(planetInverseModel))*b);
    return normalize(normal-(wt*dx+wb*dz)*.085*clamp(oceanWaveStrength/.18,0.0,3.0));
}
void main()
{
    vec3 surfacePosition=fsIn.worldPosition;
    // Correct the fallback globe's polygon chord depth to the sea-level sphere.
    // This is enabled only while the local ocean owns part of this planet.
    bool analyticOcean=false;
    vec3 analyticDirection=vec3(0);
    if(oceanSurfacePass==0 && oceanLocalCoverage!=0) {
        vec3 ray=normalize(surfacePosition-cameraPosition);
        vec3 oc=cameraPosition-planetCenterWorld;
        float b=dot(oc,ray), c=dot(oc,oc)-planetRadiusWorld*planetRadiusWorld;
        float h=b*b-c;
        if(h>=0.0) {
            float t=-b-sqrt(h);
            if(t>0.0) {
                surfacePosition=cameraPosition+ray*t;
                analyticDirection=normalize(surfacePosition-planetCenterWorld);
                analyticOcean=true;
            }
        }
    }

    vec3 geometricNormal =
        normalize(
            fsIn.worldNormal);


    vec3 sphereDirection =
        normalize(
            fsIn.planetDirection);


    vec3 V =
        normalize(
            cameraPosition
            -
            surfacePosition);


    if(analyticOcean) {
        geometricNormal=analyticDirection;
        sphereDirection=normalize(mat3(planetInverseModel)*analyticDirection);
    }
    gl_FragDepth=gl_FragCoord.z;
    if(analyticOcean) {
        vec4 clip=projection*view*vec4(surfacePosition,1);
        gl_FragDepth=clip.z/clip.w*.5+.5;
    }
    vec3 centerLightDirection =
        normalize(
            sunDirection);


    // =========================================================
    // PROCEDURAL LAND / OCEAN
    // =========================================================

    vec3 seedOffset =
        vec3(
            planetSeed *
            1.371,

            planetSeed *
            2.113,

            planetSeed *
            3.731);


    float continents =
        fbm(
            sphereDirection
            *
            continentScale
            +
            seedOffset);


    float detail =
        fbm(
            sphereDirection
            *
            detailScale
            +
            seedOffset
            *
            2.7);


    float height =
        continents
        *
        0.82
        +
        detail
        *
        0.18;


    float landMask =
        smoothstep(
            oceanLevel
            -
            coastWidth,

            oceanLevel
            +
            coastWidth,

            height);


    if(oceanSurfacePass!=0) {
        // Coast eligibility remains shared, but the local pass never shades land.
        if(landMask>=0.5) discard;
        landMask=0.0;
    } else if(oceanLocalCoverage!=0 && landMask<0.5) {
        vec3 radial=normalize(surfacePosition-planetCenterWorld);
        float forward=dot(radial,oceanAnchorNormal);
        if(forward>0.0) {
            vec2 p=vec2(dot(radial,oceanAnchorRight),dot(radial,oceanAnchorForward))*oceanRadiusKm/forward;
            if(max(abs(p.x),abs(p.y))<oceanOuterKm) discard;
        }
    }
    float oceanMask =
        1.0
        -
        landMask;


    // =========================================================
    // OCEAN COLOR
    // =========================================================

    float shallowFactor =
        smoothstep(
            oceanLevel
            -
            0.10,

            oceanLevel,

            height);


    vec3 oceanColor =
        mix(
            deepOceanColor,
            shallowOceanColor,
            shallowFactor);


    // =========================================================
    // LAND COLOR
    // =========================================================

    float landElevation =
        clamp(
            (
                height
                -
                oceanLevel
            )
            /
            max(
                1.0
                -
                oceanLevel,
                0.0001),
            0.0,
            1.0);


    vec3 landColor =
        mix(
            lowLandColor,
            highLandColor,
            landElevation);


    vec3 baseColor =
        mix(
            oceanColor,
            landColor,
            landMask);


    // =========================================================
    // RESOLVED WAVE NORMALS
    // =========================================================

    vec3 resolvedOceanNormal =
        oceanWaveNormal(
            geometricNormal,
            sphereDirection);


    if(oceanSurfacePass!=0) {
        vec3 rippled=localOceanRipples(geometricNormal,sphereDirection);
        resolvedOceanNormal=normalize(mix(resolvedOceanNormal,rippled,geometricWaveVisibility));
    }
    vec3 reflectionOceanNormal =
        normalize(
            mix(
                geometricNormal,
                resolvedOceanNormal,
                oceanSurfacePass!=0 ? 0.65 : 0.18));


    vec3 N =
        normalize(
            mix(
                reflectionOceanNormal,
                geometricNormal,
                landMask));


    vec3 glitterNormal =
        normalize(
            mix(
                resolvedOceanNormal,
                geometricNormal,
                landMask));


    float facetVariation =
        oceanFacetVariation(
            geometricNormal,
            reflectionOceanNormal,
            V);


    vec3 waterBodyColor =
        oceanColor
        *
        (
            1.0
            +
            facetVariation
        );


    baseColor =
        mix(
            waterBodyColor,
            landColor,
            landMask);


    // =========================================================
    // MATERIAL
    // =========================================================

    float normalDeviation =
        clamp(
            1.0
            -
            dot(
                geometricNormal,
                resolvedOceanNormal),
            0.0,
            1.0);


    float resolvedOceanRoughness =
        clamp(
            oceanRoughness
            +
            normalDeviation
            *
            0.30,
            0.035,
            0.22);


    float materialRoughness =
        mix(
            resolvedOceanRoughness,
            landRoughness,
            landMask);


    materialRoughness =
        clamp(
            materialRoughness,
            0.035,
            1.0);


    float diffuseStrength =
        mix(
            0.016,
            1.0,
            landMask);


    vec3 F0 =
        mix(
            vec3(
                0.022),
            vec3(
                0.04),
            landMask);


    // =========================================================
    // DIRECT STAR DIFFUSE
    // =========================================================

    float centerNdotL =
        max(
            dot(
                geometricNormal,
                centerLightDirection),
            0.0);


    vec3 centerHalf =
        normalize(
            V
            +
            centerLightDirection);


    vec3 centerFresnel =
        fresnelSchlick(
            max(
                dot(
                    centerHalf,
                    V),
                0.0),
            F0);


    vec3 diffuseBrdf =
        (
            vec3(1.0)
            -
            centerFresnel
        )
        *
        baseColor
        *
        diffuseStrength
        /
        PI;


    vec3 sunlight =
        sunRadiance
        *
        atmosphereSunTransmittance(
            surfacePosition,
            centerLightDirection);


    vec3 directDiffuse =
        diffuseBrdf
        *
        sunlight
        *
        centerNdotL;


    // =========================================================
    // DIRECT STAR SPECULAR
    // =========================================================

    vec3 landSpecular =
        evaluateDirectionalSpecular(
            geometricNormal,
            V,
            centerLightDirection,
            F0,
            materialRoughness);


    vec3 oceanSpecular =
        finiteStarSpecular(
            glitterNormal,
            V,
            centerLightDirection,
            F0,
            materialRoughness);


    vec3 directSpecular =
        mix(
            oceanSpecular,
            landSpecular,
            landMask)
        *
        sunlight;


    // =========================================================
    // DIFFUSE ATMOSPHERIC SKY LIGHT
    // =========================================================

    vec3 localUp;


    vec3 skyResponse =
        atmosphereSkyIrradiance(
            surfacePosition,
            centerLightDirection,
            localUp);


    vec3 skyIrradiance =
        skyResponse
        *
        sunRadiance;


    float skyVisibility =
        clamp(
            0.5
            +
            0.5
            *
            dot(
                geometricNormal,
                localUp),
            0.0,
            1.0);


    vec3 skyDiffuse =
        baseColor
        *
        diffuseStrength
        *
        skyIrradiance
        *
        skyVisibility
        /
        PI;


    // =========================================================
    // SURFACE POSITION IN ATMOSPHERE SPACE
    // =========================================================

    vec3 surfacePositionKm =
        (
            surfacePosition
            -
            atmospherePlanetCenterWorld
        )
        *
        atmosphereKmPerWorldUnit;


    vec3 smoothPlanetUp =
        geometricNormal;


    if (atmosphereEnabled !=
        0)
    {
        float surfaceRadiusKm =
            length(
                surfacePositionKm);


        if (surfaceRadiusKm >
            0.000001)
        {
            smoothPlanetUp =
                surfacePositionKm
                /
                surfaceRadiusKm;


            surfacePositionKm =
                smoothPlanetUp
                *
                max(
                    surfaceRadiusKm,
                    atmosphereBottomRadiusKm
                    +
                    0.001);
        }
    }


    // =========================================================
    // OCEAN LOCAL-SKY REFLECTION
    // =========================================================

    vec3 rawReflectedSkyDirection =
        normalize(
            reflect(
                -V,
                N));


    float skyReflectionWeight;


    vec3 safeReflectedSkyDirection =
        makeHorizonSafeReflectionDirection(
            rawReflectedSkyDirection,
            smoothPlanetUp,
            skyReflectionWeight);


    vec3 localSkyRadiance =
        atmosphereIntegrateLocalSkyRadiance(
            surfacePositionKm,
            safeReflectedSkyDirection,
            centerLightDirection);


    // =========================================================
    // BELOW-HORIZON FALLBACK
    // =========================================================

    vec3 neighboringSurfaceFallback =
        skyIrradiance
        *
        (
            0.16
            /
            PI
        );


    vec3 reflectedEnvironmentRadiance =
        mix(
            neighboringSurfaceFallback,
            localSkyRadiance,
            skyReflectionWeight);


    // =========================================================
    // WATER FRESNEL
    // =========================================================

    float oceanNdotV =
        max(
            dot(
                N,
                V),
            0.0);


    vec3 oceanFresnel =
        fresnelSchlick(
            oceanNdotV,
            vec3(
                0.02));


    vec3 oceanSkyReflection =
        reflectedEnvironmentRadiance
        *
        oceanFresnel
        *
        oceanMask;


    // =========================================================
    // FINAL SURFACE RADIANCE
    // =========================================================

    vec3 finalColor =
        directDiffuse
        +
        directSpecular
        +
        skyDiffuse
        +
        oceanSkyReflection;


    outColor =
        vec4(
            finalColor,
            1.0);


    outLinearDepth =
        length(
            cameraPosition
            -
            surfacePosition);
}