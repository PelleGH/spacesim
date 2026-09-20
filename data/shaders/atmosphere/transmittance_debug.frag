#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


uniform sampler2D transmittanceLut;


void main()
{
    vec2 sampleUv;


    bool showRaw =
        vUV.x <
        0.5;


    if (showRaw)
    {
        // Left half.
        sampleUv =
            vec2(
                vUV.x *
                2.0,
                vUV.y);
    }
    else
    {
        // Right half.
        sampleUv =
            vec2(
                (
                    vUV.x -
                    0.5
                ) *
                2.0,
                vUV.y);
    }


    vec3 transmittance =
        texture(
            transmittanceLut,
            sampleUv).rgb;


    vec3 result;


    if (showRaw)
    {
        // Actual stored value.
        result =
            transmittance;
    }
    else
    {
        // Visualize the light that was REMOVED.
        //
        // This is only a debugging visualization.
        vec3 extinction =
            vec3(1.0) -
            transmittance;


        // Boost low values so subtle wavelength differences
        // are easy to see.
        result =
            pow(
                clamp(
                    extinction,
                    0.0,
                    1.0),
                vec3(
                    0.35));
    }


    outColor =
        vec4(
            result,
            1.0);
}