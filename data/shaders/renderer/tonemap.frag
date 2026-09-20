#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


uniform sampler2D hdrTexture;

uniform sampler2D bloomTexture;

uniform sampler2D autoExposureTexture;


uniform int autoExposureEnabled;


uniform float exposureCompensation;

uniform float bloomStrength;


vec3 acesApprox(
    vec3 color)
{
    const float a =
        2.51;

    const float b =
        0.03;

    const float c =
        2.43;

    const float d =
        0.59;

    const float e =
        0.14;


    return clamp(
        (
            color *
            (
                a *
                color +
                b
            )
        )
        /
        (
            color *
            (
                c *
                color +
                d
            )
            +
            e
        ),
        0.0,
        1.0);
}


void main()
{
    const vec3 hdrColor =
        texture(
            hdrTexture,
            vUV).rgb;


    const vec3 bloom =
        texture(
            bloomTexture,
            vUV).rgb;


    // Bloom remains in HDR space.
    vec3 combinedHdr =
        hdrColor
        +
        bloom *
        bloomStrength;


    float exposure =
        exposureCompensation;


    if (autoExposureEnabled !=
        0)
    {
        exposure *=
            texelFetch(
                autoExposureTexture,
                ivec2(
                    0,
                    0),
                0).r;
    }


    combinedHdr *=
        exposure;


    const vec3 mappedColor =
        acesApprox(
            combinedHdr);


    outColor =
        vec4(
            mappedColor,
            1.0);
}