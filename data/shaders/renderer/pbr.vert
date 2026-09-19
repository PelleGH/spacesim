#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

out VS_OUT
{
    vec3 worldPosition;
    vec3 worldNormal;
    vec2 texCoord;

    vec4 lightSpacePosition;
} vsOut;

void main()
{
    vec4 worldPosition =
        model * vec4(inPosition, 1.0);
    vsOut.lightSpacePosition =
        lightSpaceMatrix *
        worldPosition;
        
    vsOut.worldPosition =
        worldPosition.xyz;

    mat3 normalMatrix =
        mat3(transpose(inverse(model)));

    vsOut.worldNormal =
        normalize(normalMatrix * inNormal);

    vsOut.texCoord =
        inTexCoord;

    gl_Position =
        projection *
        view *
        worldPosition;
}