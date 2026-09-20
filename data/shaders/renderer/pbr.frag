#version 450 core


layout(location = 0)
out vec4 outColor;


layout(location = 1)
out float outLinearDepth;


in VS_OUT
{
    vec3 worldPosition;
    vec3 worldNormal;
    vec2 texCoord;

    vec4 lightSpacePosition;
} fsIn;


// =============================================================
// CAMERA / SUN
// =============================================================

uniform vec3 cameraPosition;

uniform vec3 sunDirection;

uniform vec3 sunRadiance;


// =============================================================
// NORMAL ENVIRONMENT IBL
// =============================================================

uniform samplerCube irradianceMap;

uniform samplerCube prefilteredEnvironmentMap;

uniform sampler2D brdfLut;


uniform vec3 environmentDiffuseMultiplier;

uniform vec3 environmentSpecularMultiplier;


// =============================================================
// ATMOSPHERE
// =============================================================

uniform int atmosphereLightingEnabled;

uniform int atmosphereSpecularEnabled;


uniform sampler2D atmosphereTransmittanceLut;

uniform sampler2D atmosphereSkyIrradianceLut;

uniform sampler2D atmosphereSkyViewLut;


uniform vec3 atmospherePlanetCenterWorld;


uniform float atmosphereKmPerWorldUnit;

uniform float atmosphereBottomRadiusKm;

uniform float atmosphereTopRadiusKm;


// Sky-View is currently a LOCAL probe around the camera/player.
//
// Objects outside this range fade away from this probe rather than
// receiving obviously incorrect reflections from another location.

uniform float atmosphereSpecularProbeRangeWorld;


// =============================================================
// SHADOWS
// =============================================================

uniform sampler2D shadowMap;


// =============================================================
// MATERIAL
// =============================================================

uniform vec3 baseColor;

uniform float metallic;

uniform float roughness;


uniform vec3 emissiveColor;

uniform float emissiveStrength;


const float PI =
    3.14159265359;


// =============================================================
// GGX
// =============================================================

float distributionGGX(
    vec3 N,
    vec3 H,
    float materialRoughness)
{
    float a =
        materialRoughness *
        materialRoughness;


    float a2 =
        a *
        a;


    float NdotH =
        max(
            dot(
                N,
                H),
            0.0);


    float NdotH2 =
        NdotH *
        NdotH;


    float denominator =
        NdotH2 *
        (
            a2 -
            1.0
        )
        +
        1.0;


    denominator =
        PI *
        denominator *
        denominator;


    return
        a2 /
        max(
            denominator,
            0.000001);
}


float geometrySchlickGGX(
    float NdotV,
    float materialRoughness)
{
    float r =
        materialRoughness +
        1.0;


    float k =
        (
            r *
            r
        )
        /
        8.0;


    return
        NdotV /
        (
            NdotV *
            (
                1.0 -
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
    float NdotV =
        max(
            dot(
                N,
                V),
            0.0);


    float NdotL =
        max(
            dot(
                N,
                L),
            0.0);


    return
        geometrySchlickGGX(
            NdotV,
            materialRoughness)
        *
        geometrySchlickGGX(
            NdotL,
            materialRoughness);
}


// =============================================================
// FRESNEL
// =============================================================

vec3 fresnelSchlick(
    float cosTheta,
    vec3 F0)
{
    return
        F0
        +
        (
            1.0 -
            F0
        )
        *
        pow(
            1.0 -
            cosTheta,
            5.0);
}


vec3 fresnelSchlickRoughness(
    float cosTheta,
    vec3 F0,
    float materialRoughness)
{
    return
        F0
        +
        (
            max(
                vec3(
                    1.0 -
                    materialRoughness),
                F0)
            -
            F0
        )
        *
        pow(
            1.0 -
            cosTheta,
            5.0);
}


// =============================================================
// SHADOWS
// =============================================================

float calculateShadow(
    vec4 lightSpacePosition,
    vec3 normal,
    vec3 lightDirection)
{
    vec3 projected =
        lightSpacePosition.xyz /
        lightSpacePosition.w;


    projected =
        projected *
        0.5 +
        0.5;


    if (projected.x < 0.0 ||
        projected.x > 1.0 ||
        projected.y < 0.0 ||
        projected.y > 1.0 ||
        projected.z < 0.0 ||
        projected.z > 1.0)
    {
        return
            0.0;
    }


    float currentDepth =
        projected.z;


    float bias =
        max(
            0.0025 *
            (
                1.0 -
                dot(
                    normal,
                    lightDirection)
            ),
            0.0005);


    vec2 texelSize =
        1.0 /
        vec2(
            textureSize(
                shadowMap,
                0));


    float shadow =
        0.0;


    for (int x = -1;
         x <= 1;
         ++x)
    {
        for (int y = -1;
             y <= 1;
             ++y)
        {
            float closestDepth =
                texture(
                    shadowMap,
                    projected.xy
                    +
                    vec2(
                        x,
                        y)
                    *
                    texelSize).r;


            if (currentDepth - bias >
                closestDepth)
            {
                shadow +=
                    1.0;
            }
        }
    }


    return
        shadow /
        9.0;
}


// =============================================================
// ATMOSPHERE GROUND INTERSECTION
// =============================================================

bool atmosphereRayHitsGround(
    vec3 positionKm,
    vec3 direction)
{
    vec3 position =
        positionKm /
        atmosphereBottomRadiusKm;


    float b =
        dot(
            position,
            direction);


    float c =
        dot(
            position,
            position)
        -
        1.0;


    float discriminant =
        b *
        b -
        c;


    if (discriminant < 0.0)
    {
        return
            false;
    }


    float nearest =
        -b -
        sqrt(
            max(
                discriminant,
                0.0));


    return
        nearest >
        0.000001;
}


// =============================================================
// TRANSMITTANCE
// =============================================================

vec2 atmosphereTransmittanceUv(
    vec3 positionKm,
    vec3 direction)
{
    float radius =
        length(
            positionKm);


    vec3 localUp =
        positionKm /
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
                atmosphereTopRadiusKm *
                atmosphereTopRadiusKm
                -
                atmosphereBottomRadiusKm *
                atmosphereBottomRadiusKm,
                0.0));


    float rho =
        sqrt(
            max(
                radius *
                radius
                -
                atmosphereBottomRadiusKm *
                atmosphereBottomRadiusKm,
                0.0));


    float discriminant =
        radius *
        radius *
        (
            mu *
            mu -
            1.0
        )
        +
        atmosphereTopRadiusKm *
        atmosphereTopRadiusKm;


    float distanceToTop =
        -radius *
        mu
        +
        sqrt(
            max(
                discriminant,
                0.0));


    float distanceMinimum =
        atmosphereTopRadiusKm -
        radius;


    float distanceMaximum =
        rho +
        H;


    float xMu =
        (
            distanceToTop -
            distanceMinimum
        )
        /
        max(
            distanceMaximum -
            distanceMinimum,
            0.000001);


    float xRadius =
        rho /
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
            parameterUv *
            (
                size -
                vec2(1.0)
            )
            +
            vec2(0.5)
        )
        /
        size;
}


vec3 atmosphereSunTransmittance(
    vec3 worldPosition,
    vec3 directionToSun)
{
    if (atmosphereLightingEnabled == 0)
    {
        return
            vec3(1.0);
    }


    vec3 positionKm =
        (
            worldPosition -
            atmospherePlanetCenterWorld
        )
        *
        atmosphereKmPerWorldUnit;


    float radiusKm =
        length(
            positionKm);


    if (radiusKm >=
        atmosphereTopRadiusKm)
    {
        return
            vec3(1.0);
    }


    if (radiusKm <= 0.000001)
    {
        return
            vec3(1.0);
    }


    vec3 localUp =
        positionKm /
        radiusKm;


    radiusKm =
        max(
            radiusKm,
            atmosphereBottomRadiusKm +
            0.001);


    positionKm =
        localUp *
        radiusKm;


    if (atmosphereRayHitsGround(
            positionKm,
            directionToSun))
    {
        return
            vec3(0.0);
    }


    return textureLod(
        atmosphereTransmittanceLut,
        atmosphereTransmittanceUv(
            positionKm,
            directionToSun),
        0.0).rgb;
}


// =============================================================
// SKY IRRADIANCE
// =============================================================

vec2 atmosphereSkyIrradianceUv(
    float altitudeKm,
    float sunMu)
{
    float atmosphereThicknessKm =
        atmosphereTopRadiusKm -
        atmosphereBottomRadiusKm;


    float encodedSunMu =
        sign(
            sunMu)
        *
        sqrt(
            abs(
                sunMu));


    float u =
        encodedSunMu *
        0.5 +
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
            parameterUv *
            (
                size -
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
        vec3(
            0.0,
            1.0,
            0.0);


    if (atmosphereLightingEnabled == 0)
    {
        return
            vec3(0.0);
    }


    vec3 positionKm =
        (
            worldPosition -
            atmospherePlanetCenterWorld
        )
        *
        atmosphereKmPerWorldUnit;


    float radiusKm =
        length(
            positionKm);


    if (radiusKm <= 0.000001)
    {
        return
            vec3(0.0);
    }


    localUp =
        positionKm /
        radiusKm;


    if (radiusKm >=
        atmosphereTopRadiusKm)
    {
        return
            vec3(0.0);
    }


    float altitudeKm =
        clamp(
            radiusKm -
            atmosphereBottomRadiusKm,
            0.0,
            atmosphereTopRadiusKm -
            atmosphereBottomRadiusKm);


    float sunMu =
        clamp(
            dot(
                localUp,
                directionToSun),
            -1.0,
            1.0);


    return textureLod(
        atmosphereSkyIrradianceLut,
        atmosphereSkyIrradianceUv(
            altitudeKm,
            sunMu),
        0.0).rgb;
}


// =============================================================
// SKY-VIEW LUT MAPPING
// =============================================================

vec2 atmosphereUnitUvToSubUv(
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


vec2 atmosphereSkyViewUv(
    vec3 rayDirection,
    vec3 probeRelativeWorld)
{
    vec3 localUp =
        normalize(
            probeRelativeWorld);


    float viewHeightKm =
        max(
            length(
                probeRelativeWorld)
            *
            atmosphereKmPerWorldUnit,
            atmosphereBottomRadiusKm +
            0.001);


    float viewZenithCosAngle =
        clamp(
            dot(
                rayDirection,
                localUp),
            -1.0,
            1.0);


    // -------------------------------------------------------------
    // SUN-RELATIVE AZIMUTH
    // -------------------------------------------------------------

    vec3 normalizedSunDirection =
        normalize(
            sunDirection);


    vec3 sunTangent =
        normalizedSunDirection
        -
        localUp *
        dot(
            normalizedSunDirection,
            localUp);


    float sunTangentLength =
        length(
            sunTangent);


    if (sunTangentLength < 0.000001)
    {
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


    if (viewZenithSinAngle >
        0.000001)
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


    // -------------------------------------------------------------
    // HORIZON MAPPING
    // -------------------------------------------------------------

    float horizonDistance =
        sqrt(
            max(
                viewHeightKm *
                viewHeightKm
                -
                atmosphereBottomRadiusKm *
                atmosphereBottomRadiusKm,
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


    // Reflections directed into the sky should normally remain in
    // this upper portion. The lower branch is retained so the
    // parameterization remains identical to AtmospherePass.

    if (viewZenithAngle <=
        zenithHorizonAngle)
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
        atmosphereUnitUvToSubUv(
            uv,
            vec2(
                textureSize(
                    atmosphereSkyViewLut,
                    0)));
}


// =============================================================
// LOCAL ATMOSPHERIC REFLECTION PROBE
// =============================================================

vec3 atmosphereSpecularRadiance(
    vec3 worldPosition,
    vec3 reflectionDirection,
    float materialRoughness)
{
    if (atmosphereSpecularEnabled == 0)
    {
        return
            vec3(0.0);
    }


    vec3 probeRelativeWorld =
        cameraPosition -
        atmospherePlanetCenterWorld;


    vec3 probePositionKm =
        probeRelativeWorld *
        atmosphereKmPerWorldUnit;


    float probeRadiusKm =
        length(
            probePositionKm);


    // Sky-View represents atmosphere around the player/camera.
    //
    // If the player is already in vacuum, don't pretend the
    // atmospheric sky is a local reflection environment.

    if (probeRadiusKm >=
        atmosphereTopRadiusKm)
    {
        return
            vec3(0.0);
    }


    if (probeRadiusKm <=
        atmosphereBottomRadiusKm)
    {
        vec3 probeUp =
            normalize(
                probePositionKm);


        probePositionKm =
            probeUp *
            (
                atmosphereBottomRadiusKm +
                0.001
            );
    }


    // -------------------------------------------------------------
    // LOCAL PROBE FADE
    // -------------------------------------------------------------

    float objectDistance =
        length(
            worldPosition -
            cameraPosition);


    float probeRange =
        max(
            atmosphereSpecularProbeRangeWorld,
            0.000001);


    float probeWeight =
        1.0
        -
        smoothstep(
            probeRange *
            0.35,
            probeRange,
            objectDistance);


    if (probeWeight <=
        0.000001)
    {
        return
            vec3(0.0);
    }


    // -------------------------------------------------------------
    // GROUND OCCLUSION
    // -------------------------------------------------------------
    //
    // Sky-View currently contains atmospheric radiance, not a
    // proper planet/ground reflection environment.
    //
    // Therefore reflections pointing into the planet should NOT
    // sample the sky probe.

    if (atmosphereRayHitsGround(
            probePositionKm,
            reflectionDirection))
    {
        return
            vec3(0.0);
    }


    vec2 skyUv =
        atmosphereSkyViewUv(
            reflectionDirection,
            probeRelativeWorld);


    float maxMip =
        float(
            textureQueryLevels(
                atmosphereSkyViewLut)
            -
            1);


    // Ordinary mipmapping is only an approximation of roughness.
    //
    // Later:
    // proper GGX-prefiltered atmospheric environment.

    float mipLevel =
        clamp(
            materialRoughness *
            maxMip,
            0.0,
            maxMip);


    vec3 skyRadiance =
        textureLod(
            atmosphereSkyViewLut,
            skyUv,
            mipLevel).rgb;


    return
        skyRadiance *
        probeWeight;
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    float materialRoughness =
        clamp(
            roughness,
            0.04,
            1.0);


    vec3 N =
        normalize(
            fsIn.worldNormal);


    vec3 V =
        normalize(
            cameraPosition -
            fsIn.worldPosition);


    vec3 L =
        normalize(
            sunDirection);


    vec3 H =
        normalize(
            V +
            L);


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


    // =========================================================
    // MATERIAL REFLECTANCE
    // =========================================================

    vec3 F0 =
        vec3(
            0.04);


    F0 =
        mix(
            F0,
            baseColor,
            metallic);


    // =========================================================
    // DIRECT STAR
    // =========================================================

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


    vec3 numerator =
        NDF *
        G *
        F;


    float denominator =
        max(
            4.0 *
            NdotV *
            NdotL,
            0.0001);


    vec3 specular =
        numerator /
        denominator;


    vec3 kS =
        F;


    vec3 kD =
        (
            vec3(1.0)
            -
            kS
        )
        *
        (
            1.0 -
            metallic
        );


    vec3 diffuse =
        kD *
        baseColor /
        PI;


    float shadow =
        calculateShadow(
            fsIn.lightSpacePosition,
            N,
            L);


    vec3 sunTransmittance =
        atmosphereSunTransmittance(
            fsIn.worldPosition,
            L);


    vec3 sunlightAtObject =
        sunRadiance *
        sunTransmittance;


    vec3 directLighting =
        (
            diffuse +
            specular
        )
        *
        sunlightAtObject
        *
        NdotL
        *
        (
            1.0 -
            shadow
        );


    // =========================================================
    // STANDARD ENVIRONMENT IBL
    // =========================================================

    vec3 environmentFresnel =
        fresnelSchlickRoughness(
            NdotV,
            F0,
            materialRoughness);


    vec3 environmentKS =
        environmentFresnel;


    vec3 environmentKD =
        (
            vec3(1.0)
            -
            environmentKS
        )
        *
        (
            1.0 -
            metallic
        );


    vec3 irradiance =
        texture(
            irradianceMap,
            N).rgb;


    irradiance *=
        environmentDiffuseMultiplier;


    vec3 indirectDiffuse =
        environmentKD
        *
        baseColor
        *
        irradiance
        /
        PI;


    vec3 reflectionDirection =
        reflect(
            -V,
            N);


    float maxReflectionMip =
        float(
            textureQueryLevels(
                prefilteredEnvironmentMap)
            -
            1);


    float reflectionMip =
        materialRoughness *
        maxReflectionMip;


    vec3 prefilteredRadiance =
        textureLod(
            prefilteredEnvironmentMap,
            reflectionDirection,
            reflectionMip).rgb;


    vec2 brdf =
        texture(
            brdfLut,
            vec2(
                NdotV,
                materialRoughness)).rg;


    vec3 indirectSpecular =
        prefilteredRadiance
        *
        (
            environmentFresnel *
            brdf.x
            +
            brdf.y
        );


    indirectSpecular *=
        environmentSpecularMultiplier;


    // =========================================================
    // ATMOSPHERIC DIFFUSE SKY
    // =========================================================

    vec3 atmosphericLocalUp;


    vec3 atmosphericSkyResponse =
        atmosphereSkyIrradiance(
            fsIn.worldPosition,
            L,
            atmosphericLocalUp);


    float skyVisibility =
        clamp(
            0.5
            +
            0.5 *
            dot(
                N,
                atmosphericLocalUp),
            0.0,
            1.0);


    vec3 atmosphericSkyIrradiance =
        atmosphericSkyResponse
        *
        sunRadiance;


    vec3 atmosphericDiffuse =
        environmentKD
        *
        baseColor
        *
        atmosphericSkyIrradiance
        /
        PI
        *
        skyVisibility;


    // =========================================================
    // ATMOSPHERIC SPECULAR SKY
    // =========================================================
    //
    // This is the new part.
    //
    // A glossy surface reflects the directional atmospheric sky
    // rather than merely receiving its integrated diffuse energy.

    vec3 atmosphericReflectionRadiance =
        atmosphereSpecularRadiance(
            fsIn.worldPosition,
            reflectionDirection,
            materialRoughness);


    vec3 atmosphericSpecular =
        atmosphericReflectionRadiance
        *
        (
            environmentFresnel *
            brdf.x
            +
            brdf.y
        );


    // =========================================================
    // EMISSIVE
    // =========================================================

    vec3 emissiveRadiance =
        emissiveColor *
        emissiveStrength;


    // =========================================================
    // FINAL HDR
    // =========================================================

    vec3 finalLighting =
        directLighting
        +
        indirectDiffuse
        +
        indirectSpecular
        +
        atmosphericDiffuse
        +
        atmosphericSpecular
        +
        emissiveRadiance;


    outColor =
        vec4(
            finalLighting,
            1.0);


    outLinearDepth =
        length(
            cameraPosition -
            fsIn.worldPosition);
}