#version 450 core


layout(location = 0)
in vec3 inDirection;


layout(location = 1)
in vec3 inRadiance;


layout(location = 2)
in float inSizePixels;


layout(location = 0)
out vec3 vRadiance;


uniform mat4 view;

uniform mat4 projection;


void main()
{
    // =============================================================
    // INFINITE-DISTANCE STAR
    // =============================================================
    //
    // We use only the ROTATION part of the view matrix.
    //
    // Camera translation therefore has no effect on background-star
    // positions.
    //
    // This is exactly what we want for stars that are effectively
    // infinitely far away.

    vec3 viewDirection =
        mat3(
            view)
        *
        normalize(
            inDirection);


    // Put the point at an arbitrary finite distance for projection.
    //
    // Only the direction matters.
    vec3 viewPosition =
        viewDirection *
        100.0;


    gl_Position =
        projection
        *
        vec4(
            viewPosition,
            1.0);


    gl_PointSize =
        inSizePixels;


    vRadiance =
        inRadiance;
}