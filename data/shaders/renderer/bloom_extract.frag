#version 450 core

layout(location = 0)
in vec2 vUV;

layout(location = 0)
out vec4 outColor;


uniform sampler2D hdrTexture;

uniform float threshold;


void main()
{
    vec3 color =
        texture(
            hdrTexture,
            vUV).rgb;


    // Perceived brightness.
    float luminance =
        dot(
            color,
            vec3(
                0.2126,
                0.7152,
                0.0722));


    // Instead of a hard binary cut, retain only the
    // amount above the threshold.
    float contribution =
        max(
            luminance -
            threshold,
            0.0);


    if (contribution <= 0.0)
    {
        outColor =
            vec4(
                0.0);

        return;
    }


    vec3 bloomColor =
        color *
        (
            contribution /
            max(
                luminance,
                0.0001)
        );


    outColor =
        vec4(
            bloomColor,
            1.0);
}