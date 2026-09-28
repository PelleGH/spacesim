#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outCloud;


layout(location = 1)
out float outCloudDepth;


// =============================================================
// CURRENT FRAME
// =============================================================

uniform sampler2D currentCloudTexture;

uniform sampler2D currentCloudDepthTexture;


// =============================================================
// PREVIOUS STABILIZED FRAME
// =============================================================

uniform sampler2D previousCloudTexture;

uniform sampler2D previousCloudDepthTexture;


uniform int historyValid;


// =============================================================
// CURRENT CAMERA
// =============================================================

uniform vec3 currentCameraPositionPlanetKm;

uniform vec3 currentForward;

uniform vec3 currentRight;

uniform vec3 currentUp;

uniform float currentAspectRatio;

uniform float currentTanHalfFov;


// =============================================================
// PREVIOUS CAMERA
// =============================================================

uniform vec3 previousCameraPositionPlanetKm;

uniform vec3 previousForward;

uniform vec3 previousRight;

uniform vec3 previousUp;

uniform float previousAspectRatio;

uniform float previousTanHalfFov;


// =============================================================
// RAYS
// =============================================================

vec3 currentRayDirection(
    vec2 uv)
{
    vec2 ndc =
        uv *
        2.0 -
        1.0;


    return normalize(
        currentForward
        +
        currentRight *
        ndc.x *
        currentAspectRatio *
        currentTanHalfFov
        +
        currentUp *
        ndc.y *
        currentTanHalfFov);
}


vec3 previousRayDirection(
    vec2 uv)
{
    vec2 ndc =
        uv *
        2.0 -
        1.0;


    return normalize(
        previousForward
        +
        previousRight *
        ndc.x *
        previousAspectRatio *
        previousTanHalfFov
        +
        previousUp *
        ndc.y *
        previousTanHalfFov);
}


// =============================================================
// PREVIOUS-FRAME PROJECTION
// =============================================================

bool projectIntoPreviousCamera(
    vec3 planetPositionKm,
    out vec2 previousUv)
{
    vec3 relative =
        planetPositionKm -
        previousCameraPositionPlanetKm;


    float forwardDistance =
        dot(
            relative,
            previousForward);


    if (forwardDistance <=
        0.001)
    {
        previousUv =
            vec2(
                -1.0);


        return
            false;
    }


    float denominatorX =
        forwardDistance *
        previousAspectRatio *
        previousTanHalfFov;


    float denominatorY =
        forwardDistance *
        previousTanHalfFov;


    if (
        abs(
            denominatorX) <
            0.00001
        ||
        abs(
            denominatorY) <
            0.00001)
    {
        previousUv =
            vec2(
                -1.0);


        return
            false;
    }


    vec2 previousNdc;


    previousNdc.x =
        dot(
            relative,
            previousRight)
        /
        denominatorX;


    previousNdc.y =
        dot(
            relative,
            previousUp)
        /
        denominatorY;


    previousUv =
        previousNdc *
        0.5 +
        0.5;


    return
        all(
            greaterThanEqual(
                previousUv,
                vec2(
                    0.0)))
        &&
        all(
            lessThanEqual(
                previousUv,
                vec2(
                    1.0)));
}


// =============================================================
// CURRENT NEIGHBORHOOD CLAMP
// =============================================================
//
// Five taps instead of the previous 3x3 nine-tap search.
//
// We retain history protection while making the temporal pass
// materially cheaper at fullscreen resolutions.

void currentNeighborhoodRange(
    ivec2 pixel,
    out vec4 neighborhoodMinimum,
    out vec4 neighborhoodMaximum)
{
    ivec2 size =
        textureSize(
            currentCloudTexture,
            0);


    const ivec2 offsets[5] =
        ivec2[5](
            ivec2(
                0,
                0),

            ivec2(
                -1,
                0),

            ivec2(
                1,
                0),

            ivec2(
                0,
                -1),

            ivec2(
                0,
                1));


    neighborhoodMinimum =
        vec4(
            1e20);


    neighborhoodMaximum =
        vec4(
            -1e20);


    for (int i = 0;
         i < 5;
         ++i)
    {
        ivec2 coordinate =
            clamp(
                pixel +
                offsets[i],
                ivec2(
                    0),
                size -
                ivec2(
                    1));


        vec4 value =
            texelFetch(
                currentCloudTexture,
                coordinate,
                0);


        neighborhoodMinimum =
            min(
                neighborhoodMinimum,
                value);


        neighborhoodMaximum =
            max(
                neighborhoodMaximum,
                value);
    }


    vec4 range =
        neighborhoodMaximum -
        neighborhoodMinimum;


    vec4 margin =
        max(
            range *
            0.25,
            vec4(
                0.002));


    neighborhoodMinimum -=
        margin;


    neighborhoodMaximum +=
        margin;


    neighborhoodMinimum.a =
        clamp(
            neighborhoodMinimum.a,
            0.0,
            1.0);


    neighborhoodMaximum.a =
        clamp(
            neighborhoodMaximum.a,
            0.0,
            1.0);
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    ivec2 pixel =
        ivec2(
            gl_FragCoord.xy);


    vec4 currentCloud =
        texelFetch(
            currentCloudTexture,
            pixel,
            0);


    float currentDepthKm =
        texelFetch(
            currentCloudDepthTexture,
            pixel,
            0).r;


    float currentOpacity =
        1.0 -
        currentCloud.a;


    // =========================================================
    // CURRENTLY CLEAR
    // =========================================================
    //
    // This is important around the ship.
    //
    // If current scene depth says no cloud exists along this raw
    // ray, absolutely no old cloud history is allowed to remain.

    if (
        currentOpacity <
            0.001
        ||
        currentDepthKm <=
            0.0)
    {
        outCloud =
            currentCloud;


        outCloudDepth =
            0.0;


        return;
    }


    // =========================================================
    // NO USABLE HISTORY
    // =========================================================

    if (historyValid == 0)
    {
        outCloud =
            currentCloud;


        outCloudDepth =
            currentDepthKm;


        return;
    }


    // =========================================================
    // RECONSTRUCT THE CURRENT REPRESENTATIVE 3D CLOUD POINT
    // =========================================================

    vec3 currentRay =
        currentRayDirection(
            vUV);


    vec3 currentCloudPointKm =
        currentCameraPositionPlanetKm
        +
        currentRay *
        currentDepthKm;


    // =========================================================
    // PROJECT THAT ACTUAL POINT INTO THE PREVIOUS CAMERA
    // =========================================================

    vec2 previousUv;


    if (!projectIntoPreviousCamera(
            currentCloudPointKm,
            previousUv))
    {
        outCloud =
            currentCloud;


        outCloudDepth =
            currentDepthKm;


        return;
    }


    vec4 previousCloud =
        texture(
            previousCloudTexture,
            previousUv);


    float previousDepthKm =
        texture(
            previousCloudDepthTexture,
            previousUv).r;


    float previousOpacity =
        1.0 -
        previousCloud.a;


    if (
        previousDepthKm <=
            0.0
        ||
        previousOpacity <
            0.001)
    {
        outCloud =
            currentCloud;


        outCloudDepth =
            currentDepthKm;


        return;
    }


    // =========================================================
    // RECONSTRUCT WHERE THE PREVIOUS HISTORY CLAIMS THE CLOUD WAS
    // =========================================================
    //
    // This is the key validation missing from the first temporal
    // implementation.
    //
    // Both frames now produce real planet-relative 3D points.

    vec3 previousRay =
        previousRayDirection(
            previousUv);


    vec3 previousCloudPointKm =
        previousCameraPositionPlanetKm
        +
        previousRay *
        previousDepthKm;


    float positionErrorKm =
        length(
            previousCloudPointKm -
            currentCloudPointKm);


    // Normal per-frame ray-jitter depth variation can easily be
    // a few hundred metres.
    //
    // Do not reject that.
    //
    // But history pointing more than roughly a kilometre away
    // becomes progressively untrustworthy.

    float positionConfidence =
        1.0 -
        smoothstep(
            0.45,
            1.75,
            positionErrorKm);


    if (positionConfidence <=
        0.001)
    {
        outCloud =
            currentCloud;


        outCloudDepth =
            currentDepthKm;


        return;
    }


    // =========================================================
    // TRANSMITTANCE VALIDATION
    // =========================================================

    float transmittanceDifference =
        abs(
            previousCloud.a -
            currentCloud.a);


    float transmittanceConfidence =
        1.0 -
        smoothstep(
            0.12,
            0.45,
            transmittanceDifference);


    // =========================================================
    // NEIGHBORHOOD CLAMP
    // =========================================================

    vec4 neighborhoodMinimum;

    vec4 neighborhoodMaximum;


    currentNeighborhoodRange(
        pixel,
        neighborhoodMinimum,
        neighborhoodMaximum);


    previousCloud =
        clamp(
            previousCloud,
            neighborhoodMinimum,
            neighborhoodMaximum);


    // =========================================================
    // TEMPORAL WEIGHT
    // =========================================================
    //
    // There is NO special "inside cloud = 30%" penalty anymore.
    //
    // If the 3D history position is valid, being inside the cloud
    // is exactly where we most need strong temporal accumulation.

    float historyWeight =
        0.93
        *
        positionConfidence
        *
        transmittanceConfidence;


    historyWeight =
        clamp(
            historyWeight,
            0.0,
            0.93);


    // =========================================================
    // RESOLVE
    // =========================================================

    vec4 resolvedCloud =
        mix(
            currentCloud,
            previousCloud,
            historyWeight);


    resolvedCloud.a =
        clamp(
            resolvedCloud.a,
            0.0,
            1.0);


    outCloud =
        resolvedCloud;


    // =========================================================
    // DEPTH IS NOT TEMPORALLY BLENDED
    // =========================================================
    //
    // Always retain this frame's representative depth.
    //
    // We want geometry validation to track the current cloud, not
    // lag several frames behind it.

    outCloudDepth =
        currentDepthKm;
}