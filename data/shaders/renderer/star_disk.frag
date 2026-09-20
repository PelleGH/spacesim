#version 450 core


layout(location = 0)
in vec3 vDirection;


layout(location = 0)
out vec4 outColor;


// Keep the HDR target's linear-depth attachment explicitly empty.
//
// The star is effectively infinitely far away and must remain
// background rather than becoming normal scene geometry.
layout(location = 1)
out float outLinearDepth;


uniform vec3 cameraPosition;


uniform vec3 starDirection;

uniform vec3 starDiskRadiance;


uniform float starAngularRadiusRadians;

uniform float limbDarkening;


// =============================================================
// MAIN
// =============================================================

void main()
{
    vec3 rayDirection =
        normalize(
            vDirection -
            cameraPosition);


    vec3 directionToStar =
        normalize(
            starDirection);


    float cosAngle =
        clamp(
            dot(
                rayDirection,
                directionToStar),
            -1.0,
            1.0);


    float angularDistance =
        acos(
            cosAngle);


    // =========================================================
    // PIXEL-SMOOTH DISK EDGE
    // =========================================================
    //
    // The apparent stellar disk can be only a handful of pixels
    // across, so a hard cut would shimmer badly.
    //
    // fwidth tells us approximately how much angular distance
    // changes over one screen pixel.

    float pixelAngularWidth =
        max(
            fwidth(
                angularDistance),
            0.000001);


    float edgeWidth =
        pixelAngularWidth *
        1.25;


    float coverage =
        1.0 -
        smoothstep(
            starAngularRadiusRadians -
            edgeWidth,

            starAngularRadiusRadians +
            edgeWidth,

            angularDistance);


    if (coverage <=
        0.00001)
    {
        discard;
    }


    // =========================================================
    // LIMB DARKENING
    // =========================================================
    //
    // Stellar disks generally appear slightly dimmer toward their
    // visible edge.
    //
    // normalizedRadius:
    //
    //     0 = center
    //     1 = edge

    float normalizedRadius =
        clamp(
            angularDistance /
            max(
                starAngularRadiusRadians,
                0.000001),
            0.0,
            1.0);


    float mu =
        sqrt(
            max(
                1.0 -
                normalizedRadius *
                normalizedRadius,
                0.0));


    float centerToEdgeBrightness =
        mix(
            1.0 -
            clamp(
                limbDarkening,
                0.0,
                1.0),

            1.0,

            mu);


    vec3 stellarRadiance =
        starDiskRadiance *
        centerToEdgeBrightness;


    outColor =
        vec4(
            stellarRadiance *
            coverage,
            coverage);


    // Background remains "no geometry".
    outLinearDepth =
        0.0;
}