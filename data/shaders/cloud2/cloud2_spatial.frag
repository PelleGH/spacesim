#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outCloud;


layout(location = 1)
out float outCloudDepth;


// =============================================================
// RAW HALF-RESOLUTION INPUT
// =============================================================

uniform sampler2D rawCloudTexture;

uniform sampler2D rawCloudDepthTexture;


// =============================================================
// GAUSSIAN-LIKE SPATIAL WEIGHT
// =============================================================

float spatialWeight(
    ivec2 offset)
{
    int distanceSquared =
        offset.x *
        offset.x
        +
        offset.y *
        offset.y;


    if (distanceSquared == 0)
    {
        return
            1.0;
    }


    if (distanceSquared == 1)
    {
        return
            0.68;
    }


    return
        0.46;
}


// =============================================================
// DEPTH SIMILARITY
// =============================================================
//
// Representative cloud depth is measured in physical km.
//
// Neighbors representing roughly the same part of the volume can
// be smoothed strongly.
//
// Different lobes / front-vs-back surfaces should not smear
// together.

float depthSimilarity(
    float centerDepthKm,
    float sampleDepthKm)
{
    float differenceKm =
        abs(
            centerDepthKm -
            sampleDepthKm);


    const float sigmaKm =
        0.90;


    float normalizedDifference =
        differenceKm /
        sigmaKm;


    return
        exp(
            -0.5 *
            normalizedDifference *
            normalizedDifference);
}


// =============================================================
// OPACITY SIMILARITY
// =============================================================
//
// This protects cloud silhouettes and strong density boundaries.
//
// A nearly-clear neighboring pixel should not blur heavily into
// a dense cloud pixel.

float opacitySimilarity(
    float centerOpacity,
    float sampleOpacity)
{
    float difference =
        abs(
            centerOpacity -
            sampleOpacity);


    const float sigma =
        0.22;


    float normalizedDifference =
        difference /
        sigma;


    return
        exp(
            -0.5 *
            normalizedDifference *
            normalizedDifference);
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    ivec2 textureSizePixels =
        textureSize(
            rawCloudTexture,
            0);


    ivec2 centerPixel =
        ivec2(
            gl_FragCoord.xy);


    centerPixel =
        clamp(
            centerPixel,
            ivec2(
                0),
            textureSizePixels -
            ivec2(
                1));


    vec4 centerCloud =
        texelFetch(
            rawCloudTexture,
            centerPixel,
            0);


    float centerDepthKm =
        texelFetch(
            rawCloudDepthTexture,
            centerPixel,
            0).r;


    float centerOpacity =
        1.0 -
        centerCloud.a;


    // =========================================================
    // CLEAR PIXEL
    // =========================================================
    //
    // Do NOT dilate clouds into currently empty pixels.
    //
    // This is especially important around:
    //
    //     ship edges
    //     ocean horizon
    //     cloud silhouette
    //
    // If the raw center ray saw no cloud, the filtered center ray
    // also sees no cloud.

    if (
        centerDepthKm <=
            0.0
        ||
        centerOpacity <
            0.001)
    {
        outCloud =
            centerCloud;


        outCloudDepth =
            0.0;


        return;
    }


    // =========================================================
    // BILATERAL 3x3 FILTER
    // =========================================================
    //
    // "Bilateral" means the filter considers both:
    //
    //     spatial closeness
    //
    // and
    //
    //     similarity of the underlying signal.
    //
    // Here our signal similarity is:
    //
    //     cloud depth
    //     +
    //     opacity
    //
    // so grain is smoothed without simply blurring every cloud
    // edge.

    vec4 accumulatedCloud =
        vec4(
            0.0);


    float accumulatedWeight =
        0.0;


    for (int y = -1;
         y <= 1;
         ++y)
    {
        for (int x = -1;
             x <= 1;
             ++x)
        {
            ivec2 offset =
                ivec2(
                    x,
                    y);


            ivec2 samplePixel =
                clamp(
                    centerPixel +
                    offset,
                    ivec2(
                        0),
                    textureSizePixels -
                    ivec2(
                        1));


            vec4 sampleCloud =
                texelFetch(
                    rawCloudTexture,
                    samplePixel,
                    0);


            float sampleDepthKm =
                texelFetch(
                    rawCloudDepthTexture,
                    samplePixel,
                    0).r;


            float sampleOpacity =
                1.0 -
                sampleCloud.a;


            // ---------------------------------------------
            // Do not pull clear sky into an occupied cloud
            // center.
            // ---------------------------------------------

            if (
                sampleDepthKm <=
                    0.0
                ||
                sampleOpacity <
                    0.001)
            {
                continue;
            }


            float weight =
                spatialWeight(
                    offset);


            weight *=
                depthSimilarity(
                    centerDepthKm,
                    sampleDepthKm);


            weight *=
                opacitySimilarity(
                    centerOpacity,
                    sampleOpacity);


            accumulatedCloud +=
                sampleCloud *
                weight;


            accumulatedWeight +=
                weight;
        }
    }


    // =========================================================
    // FALLBACK
    // =========================================================

    if (accumulatedWeight <=
        0.00001)
    {
        outCloud =
            centerCloud;


        outCloudDepth =
            centerDepthKm;


        return;
    }


    vec4 filteredCloud =
        accumulatedCloud /
        accumulatedWeight;


    filteredCloud.a =
        clamp(
            filteredCloud.a,
            0.0,
            1.0);


    outCloud =
        filteredCloud;


    // =========================================================
    // KEEP CURRENT PIXEL DEPTH
    // =========================================================
    //
    // We deliberately do NOT average cloud depth.
    //
    // The color is denoised between similar neighbors, but this
    // pixel continues to represent the raw ray's own geometry.
    //
    // That keeps the later full-resolution depth-aware upscale
    // conservative around the ship and cloud silhouette.

    outCloudDepth =
        centerDepthKm;
}