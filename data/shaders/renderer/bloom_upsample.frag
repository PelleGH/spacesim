#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


uniform sampler2D sourceTexture;


uniform float filterRadius;


void main()
{
    const vec2 texelSize =
        1.0
        /
        vec2(
            textureSize(
                sourceTexture,
                0));


    const vec2 offset =
        texelSize *
        filterRadius;


    // =============================================================
    // 3x3 TENT FILTER
    // =============================================================
    //
    //     1 2 1
    //     2 4 2
    //     1 2 1
    //
    // This gently spreads the smaller bloom level while we scale it
    // back into the larger one.

    vec3 result =
        vec3(0.0);


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                -1.0,
                 1.0)).rgb
        *
        1.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                 0.0,
                 1.0)).rgb
        *
        2.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                 1.0,
                 1.0)).rgb
        *
        1.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                -1.0,
                 0.0)).rgb
        *
        2.0;


    result +=
        texture(
            sourceTexture,
            vUV).rgb
        *
        4.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                 1.0,
                 0.0)).rgb
        *
        2.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                -1.0,
                -1.0)).rgb
        *
        1.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                 0.0,
                -1.0)).rgb
        *
        2.0;


    result +=
        texture(
            sourceTexture,
            vUV +
            offset *
            vec2(
                 1.0,
                -1.0)).rgb
        *
        1.0;


    result /=
        16.0;


    outColor =
        vec4(
            result,
            1.0);
}