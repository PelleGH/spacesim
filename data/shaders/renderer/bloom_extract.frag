#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


uniform sampler2D hdrTexture;


uniform float threshold;

uniform float softKnee;


void main()
{
    const vec3 color =
        max(
            texture(
                hdrTexture,
                vUV).rgb,
            vec3(0.0));


    const float luminance =
        dot(
            color,
            vec3(
                0.2126,
                0.7152,
                0.0722));


    // =============================================================
    // SOFT BLOOM THRESHOLD
    // =============================================================
    //
    // Hard thresholds tend to create a visible border where bloom
    // suddenly begins.
    //
    // The knee gives us a gradual transition around the threshold.

    const float knee =
        max(
            threshold *
            softKnee,
            0.0001);


    float softContribution =
        luminance
        -
        threshold
        +
        knee;


    softContribution =
        clamp(
            softContribution,
            0.0,
            2.0 *
            knee);


    softContribution =
        softContribution *
        softContribution
        /
        (
            4.0 *
            knee
            +
            0.0001
        );


    const float hardContribution =
        max(
            luminance
            -
            threshold,
            0.0);


    const float contribution =
        max(
            hardContribution,
            softContribution);


    if (contribution <=
        0.0)
    {
        outColor =
            vec4(0.0);


        return;
    }


    const vec3 bloomColor =
        color
        *
        (
            contribution
            /
            max(
                luminance,
                0.0001)
        );


    outColor =
        vec4(
            bloomColor,
            1.0);
}