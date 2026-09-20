#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


uniform sampler2D debugTexture;


uniform float exposure;


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
            ) +
            e
        ),
        0.0,
        1.0);
}


void main()
{
    // Flip Y so the first row of our LUT appears at
    // the TOP of the debug display.
    vec2 uv =
        vec2(
            vUV.x,
            1.0 -
            vUV.y);


    vec3 value =
        texture(
            debugTexture,
            uv).rgb;


    value *=
        exposure;


    value =
        acesApprox(
            value);


    outColor =
        vec4(
            value,
            1.0);
}