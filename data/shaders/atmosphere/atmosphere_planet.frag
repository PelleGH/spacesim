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
// ATMOSPHERE LUTS
// =============================================================
//
// The old compositor sampled a 3D aerial-perspective volume.
//
// This reference implementation does not.
//
// These two LUTs are camera-independent physical acceleration LUTs,
// so keeping them does not introduce camera-distance slices.

uniform sampler2D transmittanceLut;

uniform sampler2D multipleScatteringLut;


// Sky itself still uses the directional Sky-View LUT.
//
// This LUT is not indexed by scene distance, so it is unrelated to
// the camera-centered aerial-depth rings.

uniform sampler2D skyViewTexture;


// =============================================================
// CAMERA / PLANET / STAR
// =============================================================

uniform mat4 inverseViewProjection;


uniform vec3 cameraPositionWorld;

uniform vec3 planetCenterWorld;


uniform vec3 sunDirection;

uniform vec3 sunRadiance;


uniform float kmPerWorldUnit;

uniform float bottomRadiusKm;

uniform float topRadiusKm;


// =============================================================
// ATMOSPHERE PHYSICS
// =============================================================

uniform vec3 rayleighScatteringPerKm;

uniform float rayleighScaleHeightKm;


uniform vec3 mieScatteringPerKm;

uniform vec3 mieExtinctionPerKm;

uniform float mieScaleHeightKm;

uniform float mieAnisotropy;


uniform vec3 ozoneAbsorptionPerKm;

uniform float ozoneCenterHeightKm;

uniform float ozoneHalfWidthKm;


// =============================================================
// CONSTANTS
// =============================================================

const float PI =
    3.14159265359;


// Deliberately fixed for the reference implementation.
//
// We can optimize this after the rendering is verified.
//
// Keeping it fixed also avoids introducing visible thresholds from
// changing sample counts at different distances.

const int AERIAL_SAMPLE_COUNT =
    12;


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
// SPHERE INTERSECTION
// =============================================================
//
// Everything here is in kilometres.
//
// This avoids reconstructing an atmospheric distance from hardware
// depth and avoids the old 3D LUT depth coordinate entirely.

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
        tNearKm =
            -1.0;

        tFarKm =
            -1.0;

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


float nearestSphereIntersectionKm(
    vec3 originKm,
    vec3 direction,
    float radiusKm)
{
    float tNearKm;

    float tFarKm;


    if (!raySphereIntervalKm(
            originKm,
            direction,
            radiusKm,
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


bool rayHitsGroundKm(
    vec3 positionKm,
    vec3 direction)
{
    return
        nearestSphereIntersectionKm(
            positionKm,
            direction,
            bottomRadiusKm)
        >
        0.000001;
}


// =============================================================
// DENSITY
// =============================================================

float rayleighDensity(
    float altitudeKm)
{
    return exp(
        -max(
            altitudeKm,
            0.0)
        /
        rayleighScaleHeightKm);
}


float mieDensity(
    float altitudeKm)
{
    return exp(
        -max(
            altitudeKm,
            0.0)
        /
        mieScaleHeightKm);
}


float ozoneDensity(
    float altitudeKm)
{
    float distanceFromLayer =
        abs(
            altitudeKm -
            ozoneCenterHeightKm);


    return max(
        0.0,
        1.0 -
        distanceFromLayer /
        ozoneHalfWidthKm);
}


vec3 scatteringAtAltitude(
    float altitudeKm)
{
    return
        rayleighScatteringPerKm *
        rayleighDensity(
            altitudeKm)
        +
        mieScatteringPerKm *
        mieDensity(
            altitudeKm);
}


vec3 extinctionAtAltitude(
    float altitudeKm)
{
    return
        rayleighScatteringPerKm *
        rayleighDensity(
            altitudeKm)
        +
        mieExtinctionPerKm *
        mieDensity(
            altitudeKm)
        +
        ozoneAbsorptionPerKm *
        ozoneDensity(
            altitudeKm);
}


// =============================================================
// TRANSMITTANCE LUT
// =============================================================

vec2 transmittanceUv(
    vec3 positionKm,
    vec3 direction)
{
    float radiusKm =
        length(
            positionKm);


    vec3 localUp =
        positionKm /
        radiusKm;


    float mu =
        clamp(
            dot(
                localUp,
                direction),
            -1.0,
            1.0);


    float radius =
        radiusKm /
        bottomRadiusKm;


    float topRadius =
        topRadiusKm /
        bottomRadiusKm;


    const float bottomRadius =
        1.0;


    float H =
        sqrt(
            max(
                topRadius *
                topRadius -
                bottomRadius *
                bottomRadius,
                0.0));


    float rho =
        sqrt(
            max(
                radius *
                radius -
                bottomRadius *
                bottomRadius,
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
        topRadius *
        topRadius;


    float distanceToTop =
        -radius *
        mu
        +
        sqrt(
            max(
                discriminant,
                0.0));


    float distanceMinimum =
        topRadius -
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
                transmittanceLut,
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


vec3 sampleTransmittanceToSpace(
    vec3 positionKm,
    vec3 direction)
{
    if (rayHitsGroundKm(
            positionKm,
            direction))
    {
        return
            vec3(0.0);
    }


    return textureLod(
        transmittanceLut,
        transmittanceUv(
            positionKm,
            direction),
        0.0).rgb;
}


// =============================================================
// MULTIPLE SCATTERING
// =============================================================

vec3 sampleMultipleScattering(
    vec3 positionKm,
    vec3 directionToSun)
{
    float radiusKm =
        length(
            positionKm);


    vec3 localUp =
        positionKm /
        radiusKm;


    float sunMu =
        clamp(
            dot(
                localUp,
                directionToSun),
            -1.0,
            1.0);


    float u =
        sunMu *
        0.5 +
        0.5;


    float altitudeKm =
        radiusKm -
        bottomRadiusKm;


    float thicknessKm =
        topRadiusKm -
        bottomRadiusKm;


    float v =
        clamp(
            altitudeKm /
            max(
                thicknessKm,
                0.000001),
            0.0,
            1.0);


    return textureLod(
        multipleScatteringLut,
        vec2(
            u,
            v),
        0.0).rgb;
}


// =============================================================
// PHASE FUNCTIONS
// =============================================================

float rayleighPhase(
    float cosTheta)
{
    return
        3.0 /
        (
            16.0 *
            PI
        )
        *
        (
            1.0 +
            cosTheta *
            cosTheta
        );
}


float miePhase(
    float cosTheta,
    float g)
{
    float g2 =
        g *
        g;


    float denominator =
        1.0 +
        g2 -
        2.0 *
        g *
        cosTheta;


    return
        (
            1.0 -
            g2
        )
        /
        (
            4.0 *
            PI *
            pow(
                max(
                    denominator,
                    0.0001),
                1.5)
        );
}


// =============================================================
// DIRECT PER-PIXEL AERIAL PERSPECTIVE
// =============================================================
//
// This replaces:
//
//     3D LUT Z slice
//     → interpolate between stored path lengths
//
// with:
//
//     exact scene distance for this framebuffer pixel
//     → integrate that exact atmospheric segment
//
// There is therefore no discrete aerial-distance axis anymore.

struct AerialIntegrationResult
{
    vec3 scattering;

    vec3 transmittance;
};


AerialIntegrationResult integrateAerialPerspective(
    vec3 cameraRelativeKm,
    vec3 rayDirection,
    float rayStartKm,
    float rayEndKm)
{
    AerialIntegrationResult result;


    result.scattering =
        vec3(0.0);


    result.transmittance =
        vec3(1.0);


    float pathLengthKm =
        max(
            rayEndKm -
            rayStartKm,
            0.0);


    if (pathLengthKm <=
        0.000001)
    {
        return
            result;
    }


    vec3 directionToSun =
        normalize(
            sunDirection);


    float cosTheta =
        clamp(
            dot(
                rayDirection,
                directionToSun),
            -1.0,
            1.0);


    float phaseRayleigh =
        rayleighPhase(
            cosTheta);


    float phaseMie =
        miePhase(
            cosTheta,
            mieAnisotropy);


    // Cosine spacing concentrates samples near BOTH ends.
    //
    // That works both:
    //
    // - close to the surface, where the camera begins in dense air,
    // - from orbit, where the dense part may be near the far end.

    for (int i = 0;
         i < AERIAL_SAMPLE_COUNT;
         ++i)
    {
        float u0 =
            float(i) /
            float(
                AERIAL_SAMPLE_COUNT);


        float u1 =
            float(i + 1) /
            float(
                AERIAL_SAMPLE_COUNT);


        float mapped0 =
            0.5 -
            0.5 *
            cos(
                PI *
                u0);


        float mapped1 =
            0.5 -
            0.5 *
            cos(
                PI *
                u1);


        float sampleStartKm =
            rayStartKm +
            mapped0 *
            pathLengthKm;


        float sampleEndKm =
            rayStartKm +
            mapped1 *
            pathLengthKm;


        float stepLengthKm =
            sampleEndKm -
            sampleStartKm;


        float sampleDistanceKm =
            (
                sampleStartKm +
                sampleEndKm
            )
            *
            0.5;


        vec3 samplePositionKm =
            cameraRelativeKm +
            rayDirection *
            sampleDistanceKm;


        float altitudeKm =
            max(
                length(
                    samplePositionKm)
                -
                bottomRadiusKm,
                0.0);


        float densityRayleigh =
            rayleighDensity(
                altitudeKm);


        float densityMie =
            mieDensity(
                altitudeKm);


        vec3 localExtinction =
            extinctionAtAltitude(
                altitudeKm);


        vec3 segmentTransmittance =
            exp(
                -localExtinction *
                stepLengthKm);


        vec3 midpointTransmittance =
            result.transmittance *
            sqrt(
                segmentTransmittance);


        vec3 sunTransmittance =
            sampleTransmittanceToSpace(
                samplePositionKm,
                directionToSun);


        vec3 directScattering =
            rayleighScatteringPerKm *
            densityRayleigh *
            phaseRayleigh
            +
            mieScatteringPerKm *
            densityMie *
            phaseMie;


        vec3 directSource =
            sunRadiance *
            sunTransmittance *
            directScattering;


        vec3 multipleSource =
            scatteringAtAltitude(
                altitudeKm)
            *
            sampleMultipleScattering(
                samplePositionKm,
                directionToSun)
            *
            sunRadiance;


        result.scattering +=
            midpointTransmittance *
            (
                directSource +
                multipleSource
            )
            *
            stepLengthKm;


        result.transmittance *=
            segmentTransmittance;
    }


    return
        result;
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


    if (sunTangentLength <
        0.000001)
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
                rayDirection -
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
            horizonDistance /
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


    if (!intersectsGround)
    {
        float coord =
            viewZenithAngle /
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
                viewZenithAngle -
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
            0.5 +
            0.5;
    }


    float azimuthCoord =
        -lightViewCosAngle *
        0.5 +
        0.5;


    azimuthCoord =
        sqrt(
            clamp(
                azimuthCoord,
                0.0,
                1.0));


    uv.x =
        azimuthCoord;


    return unitUvToSubUv(
        uv,
        vec2(
            textureSize(
                skyViewTexture,
                0)));
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


    vec3 cameraRelativeWorld =
        cameraPositionWorld -
        planetCenterWorld;


    vec3 cameraRelativeKm =
        cameraRelativeWorld *
        kmPerWorldUnit;


    // =========================================================
    // EXACT ATMOSPHERIC SEGMENT FOR THIS PIXEL
    // =========================================================

    float atmosphereNearKm;

    float atmosphereFarKm;


    if (!raySphereIntervalKm(
            cameraRelativeKm,
            rayDirection,
            topRadiusKm,
            atmosphereNearKm,
            atmosphereFarKm))
    {
        outColor =
            vec4(
                sceneColor,
                1.0);

        return;
    }


    float rayStartKm =
        max(
            atmosphereNearKm,
            0.0);


    float rayEndKm =
        atmosphereFarKm;


    float groundDistanceKm =
        nearestSphereIntersectionKm(
            cameraRelativeKm,
            rayDirection,
            bottomRadiusKm);


    bool intersectsGround =
        groundDistanceKm >
        rayStartKm
        &&
        groundDistanceKm <
        rayEndKm;


    if (intersectsGround)
    {
        rayEndKm =
            groundDistanceKm;
    }


    if (rayEndKm <=
        rayStartKm)
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

    if (sceneDistanceWorld <=
        0.0)
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


        vec3 backgroundTransmittance =
            vec3(0.0);


        if (!intersectsGround)
        {
            vec3 entryPositionKm =
                cameraRelativeKm +
                rayDirection *
                rayStartKm;


            // Move just inside the atmosphere when the camera begins
            // outside it.

            entryPositionKm +=
                rayDirection *
                0.001;


            backgroundTransmittance =
                sampleTransmittanceToSpace(
                    entryPositionKm,
                    rayDirection);
        }


        outColor =
            vec4(
                sceneColor *
                backgroundTransmittance
                +
                skyRadiance,
                1.0);


        return;
    }


    // =========================================================
    // GEOMETRY / DIRECT PER-PIXEL AERIAL PERSPECTIVE
    // =========================================================

    float sceneDistanceKm =
        sceneDistanceWorld *
        kmPerWorldUnit;


    if (sceneDistanceKm <=
        rayStartKm)
    {
        // Geometry lies before the atmosphere starts.
        //
        // Example later:
        //
        // ship/cockpit geometry while the camera is in space.

        outColor =
            vec4(
                sceneColor,
                1.0);

        return;
    }


    float atmosphericEndKm =
        min(
            sceneDistanceKm,
            rayEndKm);


    AerialIntegrationResult aerial =
        integrateAerialPerspective(
            cameraRelativeKm,
            rayDirection,
            rayStartKm,
            atmosphericEndKm);


    // =========================================================
    // FINAL COMPOSITE
    // =========================================================

    outColor =
        vec4(
            sceneColor *
            aerial.transmittance
            +
            aerial.scattering,
            1.0);
}