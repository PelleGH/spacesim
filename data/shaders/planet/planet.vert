#version 450 core


layout(location = 0)
in vec3 inPosition;


layout(location = 1)
in vec3 inNormal;


layout(location = 2)
in vec2 inTexCoord;


out VS_OUT
{
    vec3 worldPosition;

    vec3 worldNormal;

    // Position on the original unit sphere.
    //
    // Procedural surface data uses this instead of UV coordinates,
    // avoiding the normal longitude seam.
    vec3 planetDirection;
}
vsOut;


uniform mat4 model;

uniform mat4 view;

uniform mat4 projection;


void main()
{
    vec4 worldPosition =
        model *
        vec4(
            inPosition,
            1.0);


    mat3 normalMatrix =
        transpose(
            inverse(
                mat3(
                    model)));


    vsOut.worldPosition =
        worldPosition.xyz;


    vsOut.worldNormal =
        normalize(
            normalMatrix *
            inNormal);


    vsOut.planetDirection =
        normalize(
            inPosition);


    gl_Position =
        projection
        *
        view
        *
        worldPosition;
}