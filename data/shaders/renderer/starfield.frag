#version 450 core


layout(location = 0)
in vec3 vRadiance;


layout(location = 0)
out vec4 outColor;


layout(location = 1)
out float outLinearDepth;


void main()
{
    // =============================================================
    // CIRCULAR POINT SPRITE
    // =============================================================
    //
    // gl_PointCoord runs from:
    //
    //     (0,0) -> (1,1)
    //
    // across the rendered point.
    //
    // Convert that into:
    //
    //     -1 -> +1
    //
    // so we can create a circular star instead of a square point.

    vec2 point =
        gl_PointCoord *
        2.0
        -
        1.0;


    float radius =
        length(
            point);


    if (radius >
        1.0)
    {
        discard;
    }


    // =============================================================
    // SOFT SUB-PIXEL EDGE
    // =============================================================
    //
    // Keep the center mostly sharp but soften the outer edge enough
    // to avoid harsh square/pixel artifacts.

    float coverage =
        1.0
        -
        smoothstep(
            0.65,
            1.0,
            radius);


    float centerProfile =
        exp(
            -radius *
            radius *
            1.6);


    float intensity =
        coverage
        *
        (
            0.70
            +
            0.30 *
            centerProfile
        );


    outColor =
        vec4(
            vRadiance *
            intensity,
            intensity);


    // Background source:
    // no normal scene geometry distance.
    outLinearDepth =
        0.0;
}