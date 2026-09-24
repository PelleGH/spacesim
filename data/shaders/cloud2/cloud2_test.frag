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
// CAMERA / PLANET
// =============================================================

uniform mat4 inverseViewProjection;


uniform vec3 cameraPositionWorld;

uniform vec3 planetCenterWorld;


uniform float kmPerWorldUnit;

uniform float planetRadiusKm;


// =============================================================
// CONTROLLED FORMATION
// =============================================================

uniform vec3 formationDirection;


uniform float formationBaseAltitudeKm;

uniform float formationHorizontalRadiusKm;

uniform float formationHeightKm;

uniform float formationSeed;

uniform float formationDensityMultiplier;


uniform float cloudExtinctionPerKm;


// =============================================================
// LIGHT
// =============================================================

uniform vec3 sunDirection;

uniform vec3 sunRadiance;


// =============================================================
// QUALITY
// =============================================================
//
// This is intentionally bounded.
//
// We are no longer marching through a planetary shell.
//
// Maximum formation diameter is only a few tens of kilometres.

const int MAX_VIEW_STEPS =
    72;


const int SHADOW_STEPS =
    5;


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
// RANDOM / NOISE
// =============================================================

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
        (
            p3.x +
            p3.y
        )
        *
        p3.z);
}


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


    return fract(
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


    vec3 local =
        fract(
            p);


    local =
        local *
        local *
        (
            3.0 -
            2.0 *
            local
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
            local.x);


    float nx10 =
        mix(
            n010,
            n110,
            local.x);


    float nx01 =
        mix(
            n001,
            n101,
            local.x);


    float nx11 =
        mix(
            n011,
            n111,
            local.x);


    float nxy0 =
        mix(
            nx00,
            nx10,
            local.y);


    float nxy1 =
        mix(
            nx01,
            nx11,
            local.y);


    return mix(
        nxy0,
        nxy1,
        local.z);
}


mat3 noiseRotation()
{
    return mat3(
         0.00,  0.80,  0.60,
        -0.80,  0.36, -0.48,
        -0.60, -0.48,  0.64);
}


// =============================================================
// FILTERED FBM
// =============================================================
//
// The underlying cloud remains the same.
//
// filterScaleKm only decides which frequencies the renderer is
// capable of resolving.
//
// That is the distinction we wanted:
//
// stable shape
//
// versus
//
// controlled filtered approximation.

float filteredFbm(
    vec3 positionKm,
    float baseWavelengthKm,
    float filterScaleKm,
    float seed)
{
    float wavelengthKm =
        baseWavelengthKm;


    float amplitude =
        0.55;


    float total =
        0.0;


    float totalWeight =
        0.0;


    vec3 p =
        positionKm;


    for (int octave = 0;
         octave < 4;
         ++octave)
    {
        float resolvedWeight =
            1.0 -
            smoothstep(
                wavelengthKm *
                    0.45,
                wavelengthKm *
                    1.35,
                filterScaleKm);


        if (resolvedWeight >
            0.001)
        {
            vec3 coordinate =
                p /
                wavelengthKm;


            coordinate +=
                vec3(
                    seed *
                        0.071,
                    seed *
                        0.113,
                    seed *
                        0.173);


            float sampleValue =
                valueNoise(
                    coordinate);


            float weight =
                amplitude *
                resolvedWeight;


            total +=
                sampleValue *
                weight;


            totalWeight +=
                weight;
        }


        p =
            noiseRotation() *
            p
            +
            vec3(
                1.7,
                -0.9,
                2.3);


        wavelengthKm *=
            0.5;


        amplitude *=
            0.5;
    }


    if (totalWeight <=
        0.0001)
    {
        return
            0.5;
    }


    return
        total /
        totalWeight;
}


// =============================================================
// FORMATION BASIS
// =============================================================

void buildFormationBasis(
    out vec3 right,
    out vec3 up,
    out vec3 forward)
{
    up =
        normalize(
            formationDirection);


    vec3 reference =
        abs(
            up.y) <
            0.95
        ?
        vec3(
            0.0,
            1.0,
            0.0)
        :
        vec3(
            0.0,
            0.0,
            1.0);


    right =
        normalize(
            cross(
                reference,
                up));


    forward =
        normalize(
            cross(
                up,
                right));
}


vec3 formationCenterPlanetKm()
{
    float centerAltitudeKm =
        formationBaseAltitudeKm +
        formationHeightKm *
            0.5;


    return
        normalize(
            formationDirection)
        *
        (
            planetRadiusKm +
            centerAltitudeKm
        );
}


vec3 positionToFormationLocal(
    vec3 planetPositionKm)
{
    vec3 right;
    vec3 up;
    vec3 forward;


    buildFormationBasis(
        right,
        up,
        forward);


    vec3 relative =
        planetPositionKm -
        formationCenterPlanetKm();


    return vec3(
        dot(
            relative,
            right),

        dot(
            relative,
            up),

        dot(
            relative,
            forward));
}


vec3 directionToFormationLocal(
    vec3 direction)
{
    vec3 right;
    vec3 up;
    vec3 forward;


    buildFormationBasis(
        right,
        up,
        forward);


    return vec3(
        dot(
            direction,
            right),

        dot(
            direction,
            up),

        dot(
            direction,
            forward));
}


vec3 formationLocalToPlanet(
    vec3 localPositionKm)
{
    vec3 right;
    vec3 up;
    vec3 forward;


    buildFormationBasis(
        right,
        up,
        forward);


    return
        formationCenterPlanetKm()

        +

        right *
        localPositionKm.x

        +

        up *
        localPositionKm.y

        +

        forward *
        localPositionKm.z;
}


// =============================================================
// FORMATION BOUNDS
// =============================================================

vec3 formationHalfBoundsKm()
{
    return vec3(
        formationHorizontalRadiusKm *
            1.08,

        formationHeightKm *
            0.52,

        formationHorizontalRadiusKm *
            1.08);
}


bool rayBoxInterval(
    vec3 origin,
    vec3 direction,
    vec3 halfExtent,
    out float tNear,
    out float tFar)
{
    vec3 safeDirection =
        direction;


    if (abs(
            safeDirection.x) <
        0.00001)
    {
        safeDirection.x =
            safeDirection.x <
                0.0
            ?
            -0.00001
            :
            0.00001;
    }


    if (abs(
            safeDirection.y) <
        0.00001)
    {
        safeDirection.y =
            safeDirection.y <
                0.0
            ?
            -0.00001
            :
            0.00001;
    }


    if (abs(
            safeDirection.z) <
        0.00001)
    {
        safeDirection.z =
            safeDirection.z <
                0.0
            ?
            -0.00001
            :
            0.00001;
    }


    vec3 inverseDirection =
        1.0 /
        safeDirection;


    vec3 t0 =
        (
            -halfExtent -
            origin
        )
        *
        inverseDirection;


    vec3 t1 =
        (
            halfExtent -
            origin
        )
        *
        inverseDirection;


    vec3 minimumT =
        min(
            t0,
            t1);


    vec3 maximumT =
        max(
            t0,
            t1);



    // Correct slab near intersection.
    tNear =
        max(
            minimumT.x,
            max(
                minimumT.y,
                minimumT.z));


    tFar =
        min(
            maximumT.x,
            min(
                maximumT.y,
                maximumT.z));


    return
        tFar >=
        max(
            tNear,
            0.0);
}


// =============================================================
// EXPLICIT CUMULUS MACRO BODY
// =============================================================
//
// This is one of the important differences from the previous
// cloud system.
//
// Noise does NOT decide the whole silhouette.
//
// These explicit volumes establish:
//
//     broad base
//     side lobes
//     central body
//     upper cauliflower masses
//
// Noise is then allowed to deform and erode them.

float ellipsoidField(
    vec3 position,
    vec3 center,
    vec3 radii)
{
    vec3 normalizedPosition =
        (
            position -
            center
        )
        /
        max(
            radii,
            vec3(
                0.001));


    return
        1.0 -
        length(
            normalizedPosition);
}


float macroCumulusField(
    vec3 p)
{
    float radius =
        formationHorizontalRadiusKm;


    float height =
        formationHeightKm;


    float field =
        -1000.0;


    // ---------------------------------------------------------
    // Broad lower body / relatively flat cloud base.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    0.0,
                    -height *
                        0.32,
                    0.0),
                vec3(
                    radius *
                        0.96,
                    height *
                        0.18,
                    radius *
                        0.80)));


    // ---------------------------------------------------------
    // Main central body.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    0.0,
                    -height *
                        0.08,
                    0.0),
                vec3(
                    radius *
                        0.55,
                    height *
                        0.37,
                    radius *
                        0.55)));


    // ---------------------------------------------------------
    // Left convective lobe.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    -radius *
                        0.42,
                    -height *
                        0.12,
                    radius *
                        0.06),
                vec3(
                    radius *
                        0.43,
                    height *
                        0.27,
                    radius *
                        0.42)));


    // ---------------------------------------------------------
    // Right convective lobe.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    radius *
                        0.40,
                    -height *
                        0.08,
                    -radius *
                        0.05),
                vec3(
                    radius *
                        0.42,
                    height *
                        0.30,
                    radius *
                        0.40)));


    // ---------------------------------------------------------
    // Rear/offset lobe prevents a perfectly symmetrical cloud.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    -radius *
                        0.10,
                    height *
                        0.02,
                    radius *
                        0.34),
                vec3(
                    radius *
                        0.42,
                    height *
                        0.31,
                    radius *
                        0.37)));


    // ---------------------------------------------------------
    // Upper cauliflower mass.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    radius *
                        0.08,
                    height *
                        0.19,
                    -radius *
                        0.05),
                vec3(
                    radius *
                        0.36,
                    height *
                        0.27,
                    radius *
                        0.36)));


    // ---------------------------------------------------------
    // Smaller high lobe.
    // ---------------------------------------------------------

    field =
        max(
            field,
            ellipsoidField(
                p,
                vec3(
                    -radius *
                        0.20,
                    height *
                        0.25,
                    -radius *
                        0.08),
                vec3(
                    radius *
                        0.25,
                    height *
                        0.21,
                    radius *
                        0.26)));


    return
        field;
}


// =============================================================
// CLOUD DENSITY
// =============================================================
//
// coarseOnly:
//
// false = view density
//         macro deformation + edge erosion
//
// true  = shadow density
//         cheaper filtered macro representation

float cloudDensityLocal(
    vec3 localPositionKm,
    float filterScaleKm,
    bool coarseOnly)
{
    vec3 halfBounds =
        formationHalfBoundsKm();


    if (
        abs(
            localPositionKm.x) >
            halfBounds.x
        ||
        abs(
            localPositionKm.y) >
            halfBounds.y
        ||
        abs(
            localPositionKm.z) >
            halfBounds.z)
    {
        return
            0.0;
    }


    float macroField =
        macroCumulusField(
            localPositionKm);


    // ---------------------------------------------------------
    // Large deformation.
    // ---------------------------------------------------------

    float shapeNoise =
        filteredFbm(
            localPositionKm,
            5.0,
            max(
                filterScaleKm,
                coarseOnly
                    ?
                    1.0
                    :
                    0.15),
            formationSeed);


    float deformedField =
        macroField

        +

        (
            shapeNoise -
            0.5
        )
        *
        (
            coarseOnly
                ?
                0.20
                :
                0.30
        );


    float body =
        smoothstep(
            -0.08,
            0.13,
            deformedField);


    // ---------------------------------------------------------
    // Physical cloud base/top envelope.
    // ---------------------------------------------------------

    float normalizedHeight =
        (
            localPositionKm.y +
            formationHeightKm *
                0.5
        )
        /
        formationHeightKm;


    float baseFade =
        smoothstep(
            0.0,
            0.045,
            normalizedHeight);


    float topFade =
        1.0 -
        smoothstep(
            0.94,
            1.0,
            normalizedHeight);


    body *=
        baseFade *
        topFade;


    if (body <=
        0.0001)
    {
        return
            0.0;
    }


    // ---------------------------------------------------------
    // Fine edge erosion.
    //
    // Only view samples receive this.
    //
    // Shadow rays use a coarser density representation.
    // ---------------------------------------------------------

    if (!coarseOnly)
    {
        float detailWeight =
            1.0 -
            smoothstep(
                0.75,
                2.0,
                filterScaleKm);


        if (detailWeight >
            0.001)
        {
            float erosion =
                filteredFbm(
                    localPositionKm
                    +
                    vec3(
                        7.1,
                        -3.7,
                        11.3),
                    1.45,
                    filterScaleKm,
                    formationSeed +
                        19.73);


            float edgeAmount =
                1.0 -
                smoothstep(
                    0.42,
                    0.88,
                    body);


            body -=
                (
                    1.0 -
                    erosion
                )
                *
                edgeAmount
                *
                0.34
                *
                detailWeight;
        }
    }


    body =
        max(
            body,
            0.0);


    body =
        smoothstep(
            0.035,
            0.92,
            body);


    return
        clamp(
            body *
            formationDensityMultiplier,
            0.0,
            1.25);
}


// =============================================================
// PLANET SHADOW
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
        -b -
        root;


    tFarKm =
        -b +
        root;


    return
        tFarKm >
        0.0;
}


bool planetBlocksSun(
    vec3 planetPositionKm)
{
    float tNearKm;
    float tFarKm;


    vec3 origin =
        planetPositionKm +
        sunDirection *
            0.01;


    if (!raySphereIntervalKm(
            origin,
            sunDirection,
            planetRadiusKm,
            tNearKm,
            tFarKm))
    {
        return
            false;
    }


    return
        tNearKm >
        0.0;
}


// =============================================================
// CLOUD SELF SHADOW
// =============================================================

float cloudShadowTransmittance(
    vec3 localPositionKm,
    vec3 localSunDirection)
{
    vec3 shadowOrigin =
        localPositionKm +
        localSunDirection *
            0.03;


    float tNearKm;
    float tFarKm;


    if (!rayBoxInterval(
            shadowOrigin,
            localSunDirection,
            formationHalfBoundsKm(),
            tNearKm,
            tFarKm))
    {
        return
            1.0;
    }


    float marchDistanceKm =
        max(
            tFarKm,
            0.0);


    if (marchDistanceKm <=
        0.001)
    {
        return
            1.0;
    }


    float stepLengthKm =
        marchDistanceKm /
        float(
            SHADOW_STEPS);


    float opticalDepth =
        0.0;


    for (int i = 0;
         i < SHADOW_STEPS;
         ++i)
    {
        float distanceKm =
            (
                float(i) +
                0.5
            )
            *
            stepLengthKm;


        vec3 sampleLocal =
            shadowOrigin +
            localSunDirection *
            distanceKm;


        float density =
            cloudDensityLocal(
                sampleLocal,
                max(
                    stepLengthKm,
                    1.0),
                true);


        opticalDepth +=
            density *
            cloudExtinctionPerKm *
            0.82 *
            stepLengthKm;


        if (opticalDepth >
            10.0)
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
// PHASE FUNCTION
// =============================================================

float henyeyGreenstein(
    float cosineTheta,
    float g)
{
    float gSquared =
        g *
        g;


    float denominator =
        max(
            1.0 +
            gSquared -
            2.0 *
            g *
            cosineTheta,
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
    vec3 viewDirection)
{
    float cosineTheta =
        clamp(
            dot(
                viewDirection,
                sunDirection),
            -1.0,
            1.0);


    float forward =
        henyeyGreenstein(
            cosineTheta,
            0.68);


    float backward =
        henyeyGreenstein(
            cosineTheta,
            -0.18);


    return
        mix(
            backward,
            forward,
            0.86);
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


    // ---------------------------------------------------------
    // Planet-centered physical km.
    // ---------------------------------------------------------

    vec3 rayOriginPlanetKm =
        (
            cameraPositionWorld -
            planetCenterWorld
        )
        *
        kmPerWorldUnit;


    vec3 rayOriginLocal =
        positionToFormationLocal(
            rayOriginPlanetKm);


    vec3 rayDirectionLocal =
        directionToFormationLocal(
            rayDirection);


    // ---------------------------------------------------------
    // Cheap formation bounds test.
    //
    // Most screen pixels stop here.
    // ---------------------------------------------------------

    float tNearKm;
    float tFarKm;


    if (!rayBoxInterval(
            rayOriginLocal,
            rayDirectionLocal,
            formationHalfBoundsKm(),
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


    // ---------------------------------------------------------
    // Real scene occlusion.
    //
    // Planet/ocean/ship geometry in front of the cloud clips it.
    // ---------------------------------------------------------

    if (sceneDistanceWorld >
        0.0)
    {
        float sceneDistanceKm =
            sceneDistanceWorld *
            kmPerWorldUnit;


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


    float segmentLengthKm =
        marchEndKm -
        marchStartKm;


    // ---------------------------------------------------------
    // Physical step size.
    //
    // Because the cloud is bounded, this does not explode at the
    // planetary horizon like the old shell marcher.
    // ---------------------------------------------------------

    float stepLengthKm =
        max(
            0.35,
            segmentLengthKm /
            float(
                MAX_VIEW_STEPS));


    // One stable spatial jitter per pixel.
    //
    // This breaks coherent marching bands without making the
    // density itself camera-dependent.
    float jitter =
        hash12(
            gl_FragCoord.xy);


    float currentDistanceKm =
        marchStartKm +
        jitter *
        stepLengthKm;


    float accumulatedTransmittance =
        1.0;


    vec3 accumulatedRadiance =
        vec3(
            0.0);


    float phase =
        cloudPhase(
            rayDirection);


    vec3 localSunDirection =
        normalize(
            directionToFormationLocal(
                sunDirection));


    // =========================================================
    // VIEW MARCH
    // =========================================================

    for (int i = 0;
         i < MAX_VIEW_STEPS;
         ++i)
    {
        if (currentDistanceKm >=
            marchEndKm)
        {
            break;
        }


        float actualStepLengthKm =
            min(
                stepLengthKm,
                marchEndKm -
                currentDistanceKm);


        float sampleDistanceKm =
            currentDistanceKm +
            actualStepLengthKm *
                0.5;


        vec3 samplePlanetKm =
            rayOriginPlanetKm +
            rayDirection *
            sampleDistanceKm;


        vec3 sampleLocalKm =
            rayOriginLocal +
            rayDirectionLocal *
            sampleDistanceKm;


        float pixelFootprintKm =
            max(
                sampleDistanceKm *
                rayAngularFootprint,
                0.01);


        float filterScaleKm =
            max(
                actualStepLengthKm,
                pixelFootprintKm);


        float density =
            cloudDensityLocal(
                sampleLocalKm,
                filterScaleKm,
                false);


        currentDistanceKm +=
            actualStepLengthKm;


        if (density <=
            0.0001)
        {
            continue;
        }


        // -----------------------------------------------------
        // VIEW TRANSMITTANCE
        // -----------------------------------------------------

        float opticalDepth =
            density *
            cloudExtinctionPerKm *
            actualStepLengthKm;


        float stepTransmittance =
            exp(
                -opticalDepth);


        float scatteredFraction =
            1.0 -
            stepTransmittance;


        // -----------------------------------------------------
        // DIRECT SUN + CLOUD SELF SHADOW
        // -----------------------------------------------------

        bool planetShadow =
            planetBlocksSun(
                samplePlanetKm);


        float cloudSunTransmittance =
            planetShadow
            ?
            0.0
            :
            cloudShadowTransmittance(
                sampleLocalKm,
                localSunDirection);


        vec3 directSunlight =
            sunRadiance *
            cloudSunTransmittance;


        // -----------------------------------------------------
        // TEMPORARY SKY / MULTIPLE-SCATTER FILL
        // -----------------------------------------------------
        //
        // This is intentionally simple.
        //
        // Once the density is proven, this will be replaced with
        // atmosphere-driven sky irradiance and a better cloud
        // multiple-scattering approximation.

        float fillStrength =
            planetShadow
            ?
            0.003
            :
            0.020;


        vec3 fillLight =
            sunRadiance *
            fillStrength;


        // HG is normalized for physical integration, but our
        // current lighting model is intentionally approximate.
        //
        // This boost keeps side-lit clouds readable while we
        // establish the density and self-shadowing.
        float phaseBoost =
            phase *
            4.0;


        vec3 scatteredRadiance =
            directSunlight *
            phaseBoost

            +

            fillLight;


        accumulatedRadiance +=
            accumulatedTransmittance *
            scatteredRadiance *
            scatteredFraction;


        accumulatedTransmittance *=
            stepTransmittance;


        if (accumulatedTransmittance <
            0.002)
        {
            break;
        }
    }


    outColor =
        vec4(
            accumulatedRadiance
            +
            sceneColor *
            accumulatedTransmittance,
            1.0);
}