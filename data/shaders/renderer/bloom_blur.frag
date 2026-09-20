#version 450 core

layout(location = 0)
in vec2 vUV;

layout(location = 0)
out vec4 outColor;


uniform sampler2D sourceTexture;

uniform int horizontal;


void main()
{
    vec2 texelSize =
        1.0 /
        vec2(
            textureSize(
                sourceTexture,
                0));


    vec2 direction =
        horizontal != 0
            ? vec2(
                texelSize.x,
                0.0)
            : vec2(
                0.0,
                texelSize.y);


    // Approximate Gaussian weights.
    float weights[5] =
        float[](
            0.227027,
            0.1945946,
            0.1216216,
            0.054054,
            0.016216);


    vec3 result =
        texture(
            sourceTexture,
            vUV).rgb *
        weights[0];


    for (int i = 1;
         i < 5;
         ++i)
    {
        vec2 offset =
            direction *
            float(i);


        result +=
            texture(
                sourceTexture,
                vUV +
                offset).rgb *
            weights[i];


        result +=
            texture(
                sourceTexture,
                vUV -
                offset).rgb *
            weights[i];
    }


    outColor =
        vec4(
            result,
            1.0);
}