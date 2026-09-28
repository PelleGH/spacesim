#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outCloud;


layout(location = 1)
out float outCloudDepth;


// =============================================================
// INPUTS
// =============================================================

uniform sampler2D sceneLinearDepthTexture;


// Actual cached local cloud density.

uniform sampler3D cloudDensityVolume;


// =============================================================
// CAMERA / PLANET
// =============================================================

uniform mat4 inverseViewProjection;

uniform vec3 cameraPositionWorld;

uniform vec3 planetCenterWorld;

uniform float kmPerWorldUnit;

uniform float planetRadiusKm;


// =============================================================
// FORMATION
// =============================================================

uniform vec3 formationDirection;

uniform float formationBaseAltitudeKm;

uniform float formationHorizontalRadiusKm;

uniform float formationHeightKm;

uniform float formationDensityMultiplier;

uniform float cloudExtinctionPerKm;


// =============================================================
// DENSITY VOLUME
// =============================================================

uniform float densityVolumeResolution;

uniform float densityVolumeMaxLod;


// =============================================================
// LIGHT
// =============================================================

uniform vec3 sunDirection;

uniform vec3 sunRadiance;


// =============================================================
// SAMPLING
// =============================================================

uniform int frameIndex;


const int MAX_VIEW_STEPS =
    40;


const int SHADOW_PROBE_COUNT =
    4;


const float PI =
    3.14159265359;


// =============================================================
// EMPTY
// =============================================================

void writeEmptyCloud()
{
    outCloud =
        vec4(
            0.0,
            0.0,
            0.0,
            1.0);


    outCloudDepth =
        0.0;
}


// =============================================================
// HASH
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


    return
        fract(
            (
                p3.x +
                p3.y
            )
            *
            p3.z);
}


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


    return
        vec3(
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


    return
        vec3(
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


// =============================================================
// FORMATION BOUNDS
// =============================================================

vec3 formationHalfBoundsKm()
{
    return
        vec3(
            formationHorizontalRadiusKm *
                1.10,

            formationHeightKm *
                0.53,

            formationHorizontalRadiusKm *
                1.10);
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
// LOCAL POSITION → 3D TEXTURE
// =============================================================

bool localPositionToVolumeUv(
    vec3 localPositionKm,
    out vec3 uvw)
{
    vec3 halfBounds =
        formationHalfBoundsKm();


    vec3 normalized =
        localPositionKm /
        halfBounds;


    if (any(
            greaterThan(
                abs(
                    normalized),
                vec3(
                    1.0))))
    {
        uvw =
            vec3(
                0.0);


        return
            false;
    }


    uvw =
        normalized *
        0.5 +
        0.5;


    return
        true;
}


// =============================================================
// FILTERED VIEW DENSITY
// =============================================================
//
// The underlying density is now completely camera-independent.
//
// Camera/sample footprint only determines which filtered mip of
// that stable density we read.

float densityLodForScale(
    float filterScaleKm)
{
    vec3 halfBounds =
        formationHalfBoundsKm();


    vec3 volumeExtentKm =
        halfBounds *
        2.0;


    float longestExtentKm =
        max(
            volumeExtentKm.x,
            max(
                volumeExtentKm.y,
                volumeExtentKm.z));


    float voxelSizeKm =
        longestExtentKm /
        max(
            densityVolumeResolution,
            1.0);


    // 0.65 keeps near-field rendering biased toward the detailed
    // mip rather than becoming excessively blurry just because
    // the integration step is slightly larger than one voxel.

    float desiredFilterKm =
        max(
            filterScaleKm *
                0.65,
            voxelSizeKm);


    float lod =
        log2(
            max(
                desiredFilterKm /
                voxelSizeKm,
                1.0));


    return
        clamp(
            lod,
            0.0,
            densityVolumeMaxLod);
}


float cloudDensityLocal(
    vec3 localPositionKm,
    float filterScaleKm)
{
    vec3 uvw;


    if (!localPositionToVolumeUv(
            localPositionKm,
            uvw))
    {
        return
            0.0;
    }


    float lod =
        densityLodForScale(
            filterScaleKm);


    float density =
        textureLod(
            cloudDensityVolume,
            uvw,
            lod).r;


    return
        clamp(
            density *
            formationDensityMultiplier,
            0.0,
            1.25);
}


// =============================================================
// COARSE SHADOW DENSITY
// =============================================================
//
// Shadows now sample THE SAME density texture.
//
// They simply use a coarser mip.
//
// This removes the old situation where visible cloud shape and
// shadow cloud shape were two different procedural functions.

float cloudShadowDensityLocal(
    vec3 localPositionKm,
    float lod)
{
    vec3 uvw;


    if (!localPositionToVolumeUv(
            localPositionKm,
            uvw))
    {
        return
            0.0;
    }


    float density =
        textureLod(
            cloudDensityVolume,
            uvw,
            clamp(
                lod,
                0.0,
                densityVolumeMaxLod)).r;


    return
        clamp(
            density *
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
// SELF SHADOW
// =============================================================

float cloudShadowTransmittance(
    vec3 localPositionKm,
    vec3 localSunDirection,
    float shadowJitter)
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
        0.05)
    {
        return
            1.0;
    }


    const float probeDistances[SHADOW_PROBE_COUNT] =
        float[SHADOW_PROBE_COUNT](
            0.45,
            1.35,
            3.25,
            7.00);


    const float probeWeights[SHADOW_PROBE_COUNT] =
        float[SHADOW_PROBE_COUNT](
            0.75,
            1.35,
            2.80,
            4.60);


    float opticalDepth =
        0.0;


    float farthestValidProbeKm =
        0.0;


    float farthestDensity =
        0.0;


    for (int i = 0;
         i < SHADOW_PROBE_COUNT;
         ++i)
    {
        float probeNoise =
            fract(
                shadowJitter +
                float(i) *
                    0.38196601125);


        float distanceScale =
            mix(
                0.88,
                1.12,
                probeNoise);


        float distanceKm =
            probeDistances[i] *
            distanceScale;


        if (distanceKm >=
            marchDistanceKm)
        {
            continue;
        }


        vec3 sampleLocal =
            shadowOrigin +
            localSunDirection *
                distanceKm;


        // Nearby shadow information gets a reasonably detailed
        // mip. Farther probes become progressively coarser.

        float shadowLod =
            1.75
            +
            0.65 *
            log2(
                1.0 +
                distanceKm);


        float density =
            cloudShadowDensityLocal(
                sampleLocal,
                shadowLod);


        float availableDistance =
            max(
                marchDistanceKm -
                distanceKm,
                0.0);


        float weightKm =
            min(
                probeWeights[i],
                availableDistance +
                probeWeights[i] *
                    0.35);


        opticalDepth +=
            density *
            cloudExtinctionPerKm *
            weightKm;


        if (distanceKm >
            farthestValidProbeKm)
        {
            farthestValidProbeKm =
                distanceKm;


            farthestDensity =
                density;
        }
    }


    if (
        farthestValidProbeKm >
            0.0
        &&
        marchDistanceKm >
            farthestValidProbeKm)
    {
        float tailKm =
            min(
                marchDistanceKm -
                farthestValidProbeKm,
                4.0);


        opticalDepth +=
            farthestDensity *
            cloudExtinctionPerKm *
            tailKm *
            0.40;
    }


    opticalDepth *=
        0.92;


    return
        exp(
            -opticalDepth);
}


// =============================================================
// PHASE
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


    // =========================================================
    // PLANET / FORMATION SPACE
    // =============================================================

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


    // =========================================================
    // BOUNDS
    // =============================================================

    float tNearKm;
    float tFarKm;


    if (!rayBoxInterval(
            rayOriginLocal,
            rayDirectionLocal,
            formationHalfBoundsKm(),
            tNearKm,
            tFarKm))
    {
        writeEmptyCloud();


        return;
    }


    float marchStartKm =
        max(
            tNearKm,
            0.0);


    float marchEndKm =
        tFarKm;


    // =========================================================
    // SCENE OCCLUSION
    // =============================================================

    float sceneDistanceWorld =
        texture(
            sceneLinearDepthTexture,
            vUV).r;


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
        writeEmptyCloud();


        return;
    }


    float segmentLengthKm =
        marchEndKm -
        marchStartKm;


    float stepLengthKm =
        max(
            0.35,
            segmentLengthKm /
            float(
                MAX_VIEW_STEPS));


    // =========================================================
    // STATIC STOCHASTIC OFFSET
    // =============================================================

    float frameValue =
        float(
            frameIndex);


    vec2 temporalPixel =
        gl_FragCoord.xy +
        vec2(
            frameValue *
                23.17,
            frameValue *
                71.31);


    float temporalNoise =
        hash12(
            temporalPixel);


    float jitter =
        mix(
            0.15,
            0.85,
            temporalNoise);


    float currentDistanceKm =
        marchStartKm +
        jitter *
        stepLengthKm;


    float accumulatedTransmittance =
        1.0;


    vec3 accumulatedRadiance =
        vec3(
            0.0);


    // =========================================================
    // REPRESENTATIVE CLOUD DEPTH
    // =============================================================

    float weightedCloudDistanceKm =
        0.0;


    float cloudDistanceWeight =
        0.0;


    float phase =
        cloudPhase(
            rayDirection);


    vec3 localSunDirection =
        normalize(
            directionToFormationLocal(
                sunDirection));


    // =========================================================
    // SHADOW CACHE
    // =============================================================

    float cachedCloudSunTransmittance =
        1.0;


    bool cachedCloudSunValid =
        false;


    int occupiedSamplesSinceShadowRefresh =
        0;


    // =========================================================
    // VIEW MARCH
    // =============================================================

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
            pixelFootprintKm;


        // =====================================================
        // THIS IS NOW THE CLOUD-SHAPE QUERY
        // =====================================================
        //
        // No ellipsoid loops.
        // No FBM.
        // No erosion.
        //
        // One filtered 3D texture lookup.

        float density =
            cloudDensityLocal(
                sampleLocalKm,
                filterScaleKm);


        currentDistanceKm +=
            actualStepLengthKm;


        if (density <=
            0.0001)
        {
            cachedCloudSunValid =
                false;


            occupiedSamplesSinceShadowRefresh =
                0;


            continue;
        }


        // =====================================================
        // OPTICAL INTEGRATION
        // =============================================================

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


        float transmittanceBeforeStep =
            accumulatedTransmittance;


        // =====================================================
        // REPRESENTATIVE DEPTH
        // =============================================================

        float depthContribution =
            transmittanceBeforeStep *
            scatteredFraction;


        weightedCloudDistanceKm +=
            sampleDistanceKm *
            depthContribution;


        cloudDistanceWeight +=
            depthContribution;


        // =====================================================
        // LIGHTING
        // =============================================================

        bool planetShadow =
            planetBlocksSun(
                samplePlanetKm);


        float cloudSunTransmittance;


        if (planetShadow)
        {
            cloudSunTransmittance =
                0.0;


            cachedCloudSunValid =
                false;


            occupiedSamplesSinceShadowRefresh =
                0;
        }
        else
        {
            int shadowStride =
                1;


            if (
                density >
                    0.38
                ||
                accumulatedTransmittance <
                    0.72)
            {
                shadowStride =
                    2;
            }


            if (
                density >
                    0.72
                ||
                accumulatedTransmittance <
                    0.42)
            {
                shadowStride =
                    3;
            }


            if (accumulatedTransmittance <
                0.12)
            {
                shadowStride =
                    6;
            }


            if (cachedCloudSunValid)
            {
                occupiedSamplesSinceShadowRefresh +=
                    1;
            }


            bool refreshShadow =
                !cachedCloudSunValid
                ||
                occupiedSamplesSinceShadowRefresh >=
                    shadowStride;


            if (refreshShadow)
            {
                float shadowJitter =
                    hash12(
                        gl_FragCoord.xy +
                        vec2(
                            sampleDistanceKm *
                                17.19,

                            float(i) *
                                7.37));


                cachedCloudSunTransmittance =
                    cloudShadowTransmittance(
                        sampleLocalKm,
                        localSunDirection,
                        shadowJitter);


                cachedCloudSunValid =
                    true;


                occupiedSamplesSinceShadowRefresh =
                    0;
            }


            cloudSunTransmittance =
                cachedCloudSunTransmittance;
        }


        vec3 directSunlight =
            sunRadiance *
            cloudSunTransmittance;


        // =====================================================
        // TEMPORARY DIFFUSE FILL
        // =============================================================

        float height01 =
            clamp(
                (
                    sampleLocalKm.y +
                    formationHeightKm *
                        0.5
                )
                /
                formationHeightKm,
                0.0,
                1.0);


        float verticalSkyAccess =
            pow(
                height01,
                0.68);


        float fillStrength =
            mix(
                0.0045,
                0.023,
                verticalSkyAccess);


        fillStrength *=
            mix(
                0.62,
                1.0,
                cloudSunTransmittance);


        if (planetShadow)
        {
            fillStrength =
                0.0025;
        }


        vec3 fillLight =
            sunRadiance *
            fillStrength;


        // =====================================================
        // SCATTERING
        // =============================================================

        float phaseBoost =
            phase *
            3.7;


        float powder =
            1.0 -
            exp(
                -density *
                2.0);


        float powderBoost =
            mix(
                0.92,
                1.10,
                powder *
                cloudSunTransmittance);


        vec3 scatteredRadiance =
            directSunlight *
            phaseBoost *
            powderBoost
            +
            fillLight;


        accumulatedRadiance +=
            transmittanceBeforeStep *
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


    // =========================================================
    // OUTPUT
    // =============================================================

    float representativeCloudDepthKm =
        cloudDistanceWeight >
            0.00001
        ?
        weightedCloudDistanceKm /
            cloudDistanceWeight
        :
        0.0;


    outCloud =
        vec4(
            accumulatedRadiance,
            accumulatedTransmittance);


    outCloudDepth =
        representativeCloudDepthKm;
}