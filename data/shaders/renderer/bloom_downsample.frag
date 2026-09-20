#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


uniform sampler2D sourceTexture;


void main()
{
    const vec2 texelSize =
        1.0
        /
        vec2(
            textureSize(
                sourceTexture,
                0));


    // =============================================================
    // 13-TAP DOWNSAMPLE FILTER
    // =============================================================
    //
    // Instead of just reading one texel from the smaller image,
    // gather energy from the surrounding source pixels.
    //
    // This prevents bright tiny sources such as stars from
    // disappearing as the bloom pyramid shrinks.

    const vec3 a =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                -2.0,
                 2.0)).rgb;


    const vec3 b =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 0.0,
                 2.0)).rgb;


    const vec3 c =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 2.0,
                 2.0)).rgb;


    const vec3 d =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                -2.0,
                 0.0)).rgb;


    const vec3 e =
        texture(
            sourceTexture,
            vUV).rgb;


    const vec3 f =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 2.0,
                 0.0)).rgb;


    const vec3 g =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                -2.0,
                -2.0)).rgb;


    const vec3 h =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 0.0,
                -2.0)).rgb;


    const vec3 i =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 2.0,
                -2.0)).rgb;


    const vec3 j =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                -1.0,
                 1.0)).rgb;


    const vec3 k =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 1.0,
                 1.0)).rgb;


    const vec3 l =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                -1.0,
                -1.0)).rgb;


    const vec3 m =
        texture(
            sourceTexture,
            vUV +
            texelSize *
            vec2(
                 1.0,
                -1.0)).rgb;


    vec3 result =
        e *
        0.125;


    result +=
        (
            a +
            c +
            g +
            i
        )
        *
        0.03125;


    result +=
        (
            b +
            d +
            f +
            h
        )
        *
        0.0625;


    result +=
        (
            j +
            k +
            l +
            m
        )
        *
        0.125;


    outColor =
        vec4(
            result,
            1.0);
}