#version 450 core

layout(location = 0) in vec3 vDirection;

layout(location = 0) out vec4 outColor;

uniform samplerCube environmentMap;

uniform vec3 cameraPosition;

void main()
{
    vec3 direction =
        normalize(
            vDirection -
            cameraPosition);

    vec3 radiance =
        texture(
            environmentMap,
            direction).rgb;

    outColor =
        vec4(
            radiance,
            1.0);
}