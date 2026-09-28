#version 450 core


layout(location = 0)
in vec2 vUV;


layout(location = 0)
out vec4 outColor;


// =============================================================
// FULL-RES SCENE
// =============================================================

uniform sampler2D sceneColorTexture;

uniform sampler2D sceneLinearDepthTexture;


// =============================================================
// HALF-RES STABILIZED CLOUD
// =============================================================

uniform sampler2D cloudVolumeTexture;

uniform sampler2D cloudDepthTexture;


// =============================================================
// SCALE
// =============================================================

uniform float kmPerWorldUnit;


// =============================================================
// EMPTY CLOUD
// =============================================================

vec4 emptyCloud()
{
    return vec4(
        0.0,
        0.0,
        0.0,
        1.0);
}


// =============================================================
// DEPTH-AWARE HALF-RES SAMPLE
// =============================================================
//
// If a half-resolution cloud texel represents cloud BEHIND the
// full-resolution scene geometry at this pixel, discard that
// cloud texel.
//
// This is what prevents cloud/history from bleeding across the
// ship and ocean/terrain edges.

vec4 fetchDepthAwareCloud(
    ivec2 coordinate,
    float sceneDistanceKm)
{
    ivec2 size =
        textureSize(
            cloudVolumeTexture,
            0);


    coordinate =
        clamp(
            coordinate,
            ivec2(
                0),
            size -
            ivec2(
                1));


    vec4 cloud =
        texelFetch(
            cloudVolumeTexture,
            coordinate,
            0);


    float opacity =
        1.0 -
        cloud.a;


    if (opacity <
        0.001)
    {
        return
            cloud;
    }


    float cloudDepthKm =
        texelFetch(
            cloudDepthTexture,
            coordinate,
            0).r;


    if (cloudDepthKm <=
        0.0)
    {
        return
            emptyCloud();
    }


    if (sceneDistanceKm >
        0.0)
    {
        // A small tolerance avoids unstable clipping when cloud
        // and geometry happen to have almost identical depth.
        //
        // 0.03 km = 30 metres.

        const float depthToleranceKm =
            0.03;


        if (
            sceneDistanceKm +
                depthToleranceKm
            <
            cloudDepthKm)
        {
            return
                emptyCloud();
        }
    }


    return
        cloud;
}


// =============================================================
// MANUAL DEPTH-AWARE BILINEAR UPSCALE
// =============================================================
//
// We cannot simply call:
//
//     texture(cloudVolumeTexture, vUV)
//
// anymore.
//
// Ordinary bilinear filtering happily interpolates a cloud pixel
// behind the ship into a neighboring ship pixel.
//
// Instead:
//
// 1. gather the four half-res neighbors,
// 2. apply full-res scene-depth rejection to each,
// 3. then perform the same bilinear interpolation manually.

vec4 reconstructCloud(
    vec2 uv,
    float sceneDistanceKm)
{
    ivec2 cloudSize =
        textureSize(
            cloudVolumeTexture,
            0);


    vec2 cloudPixel =
        uv *
        vec2(
            cloudSize)
        -
        0.5;


    ivec2 basePixel =
        ivec2(
            floor(
                cloudPixel));


    vec2 fraction =
        fract(
            cloudPixel);


    vec4 c00 =
        fetchDepthAwareCloud(
            basePixel +
            ivec2(
                0,
                0),
            sceneDistanceKm);


    vec4 c10 =
        fetchDepthAwareCloud(
            basePixel +
            ivec2(
                1,
                0),
            sceneDistanceKm);


    vec4 c01 =
        fetchDepthAwareCloud(
            basePixel +
            ivec2(
                0,
                1),
            sceneDistanceKm);


    vec4 c11 =
        fetchDepthAwareCloud(
            basePixel +
            ivec2(
                1,
                1),
            sceneDistanceKm);


    vec4 row0 =
        mix(
            c00,
            c10,
            fraction.x);


    vec4 row1 =
        mix(
            c01,
            c11,
            fraction.x);


    return
        mix(
            row0,
            row1,
            fraction.y);
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    vec3 sceneColor =
        texture(
            sceneColorTexture,
            vUV).rgb;


    float sceneDistanceWorld =
        texture(
            sceneLinearDepthTexture,
            vUV).r;


    float sceneDistanceKm =
        sceneDistanceWorld >
            0.0
        ?
        sceneDistanceWorld *
            kmPerWorldUnit
        :
        0.0;


    vec4 cloud =
        reconstructCloud(
            vUV,
            sceneDistanceKm);


    float cloudTransmittance =
        clamp(
            cloud.a,
            0.0,
            1.0);


    vec3 finalColor =
        cloud.rgb
        +
        sceneColor *
        cloudTransmittance;


    outColor =
        vec4(
            finalColor,
            1.0);
}