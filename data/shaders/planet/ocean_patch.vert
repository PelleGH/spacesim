#version 450 core

layout(location = 0) in vec3 inPosition;

out VS_OUT
{
    vec2 patchPositionKm;
    vec3 worldNormal;
}
vsOut;


uniform mat4 view;
uniform mat4 projection;

uniform vec3 cameraPosition;
uniform vec3 planetCenterWorld;

uniform vec3 oceanAnchorNormal;
uniform vec3 oceanAnchorRight;
uniform vec3 oceanAnchorForward;
uniform vec3 oceanAnchorRelative;

uniform float planetRadiusWorld;
uniform float oceanRadiusKm;
uniform float worldUnitsPerKm;
uniform float oceanOuterKm;

uniform float oceanWaveScale;
uniform float oceanWaveStrength;
uniform float oceanWaveSpeed;
uniform float planetSeed;
uniform float timeSeconds;

uniform float geometricWaveVisibility;
uniform float oceanGravity;


const float PI =
    3.14159265359;


// =============================================================
// GERSTNER WAVE
// =============================================================
//
// Geometry and its wave normal are evaluated here, once per
// vertex.
//
// The fragment shader does NOT reconstruct the Gerstner surface.
// That keeps the ocean substantially cheaper than the diagnostic
// version that pushed GPU use back toward 100%.
//

void wave(
    vec3 up,
    vec3 axis,
    float wavelength,
    float amplitude,
    float steepness,
    float phaseOffset,
    float spacing,
    float fade,
    vec3 tangent,
    vec3 bitangent,
    inout vec3 displacement,
    inout vec3 dx,
    inout vec3 dz)
{
    axis =
        normalize(
            axis);


    wavelength *=
        clamp(
            850.0
            /
            max(
                oceanWaveScale,
                1.0),
            0.65,
            1.6);


    amplitude *=
        clamp(
            oceanWaveStrength
            /
            0.18,
            0.0,
            3.0)
        *
        fade
        *
        geometricWaveVisibility;


    // Filter wavelengths that cannot be represented by the
    // current clipmap spacing.
    amplitude *=
        smoothstep(
            4.0,
            8.0,
            wavelength
            /
            max(
                spacing,
                0.004));


    float k =
        2.0
        *
        PI
        /
        wavelength;


    float phase =
        k
        *
        dot(
            up
            *
            oceanRadiusKm,
            axis)
        -
        sqrt(
            max(
                oceanGravity,
                0.0001)
            *
            k)
        *
        timeSeconds
        *
        oceanWaveSpeed
        +
        planetSeed
        *
        phaseOffset;


    vec3 projected =
        axis
        -
        up
        *
        dot(
            axis,
            up);


    float projectedLength =
        length(
            projected);


    if (projectedLength <
        0.00001)
    {
        return;
    }


    vec3 direction =
        projected
        /
        projectedLength;


    float sinPhase =
        sin(
            phase);


    float cosPhase =
        cos(
            phase);


    displacement +=
        amplitude
        *
        (
            up
            *
            sinPhase
            +
            steepness
            *
            direction
            *
            cosPhase
        );


    // Analytic Gerstner tangent derivative.
    //
    // This includes horizontal displacement rather than treating
    // the wave as height-only.
    vec3 derivative =
        k
        *
        amplitude
        *
        (
            up
            *
            cosPhase
            -
            steepness
            *
            direction
            *
            sinPhase
        );


    dx +=
        derivative
        *
        dot(
            axis,
            tangent);


    dz +=
        derivative
        *
        dot(
            axis,
            bitangent);
}


// =============================================================
// MAIN
// =============================================================

void main()
{
    vec2 p =
        inPosition.xz;


    // Preserve the continuous clipmap parameter coordinate.
    //
    // The fragment shader uses this ONLY to reconstruct the smooth
    // underlying sea-level sphere. It does not recalculate the
    // Gerstner waves.
    vsOut.patchPositionKm =
        p;


    float q =
        dot(
            p,
            p);


    float r =
        sqrt(
            oceanRadiusKm
            *
            oceanRadiusKm
            +
            q);


    float factor =
        oceanRadiusKm
        /
        r;


    vec3 up =
        normalize(
            oceanAnchorNormal
            *
            oceanRadiusKm
            +
            oceanAnchorRight
            *
            p.x
            +
            oceanAnchorForward
            *
            p.y);


    // Stable spherical sagitta.
    //
    // This avoids subtracting two nearly identical planetary
    // radii.
    float sag =
        -q
        *
        oceanRadiusKm
        /
        (
            r
            *
            (
                r
                +
                oceanRadiusKm
            )
        );


    vec3 baseOffset =
        oceanAnchorRight
        *
        (
            p.x
            *
            factor
        )
        +
        oceanAnchorForward
        *
        (
            p.y
            *
            factor
        )
        +
        oceanAnchorNormal
        *
        sag;


    vec3 tangent =
        normalize(
            oceanAnchorRight
            -
            up
            *
            dot(
                up,
                oceanAnchorRight));


    vec3 bitangent =
        cross(
            up,
            tangent);


    vec3 dx =
        tangent;


    vec3 dz =
        bitangent;


    vec3 displacement =
        vec3(
            0.0);


    float extent =
        max(
            abs(
                p.x),
            abs(
                p.y));


    float spacing =
        max(
            0.004,
            extent
            *
            (
                4.0
                /
                128.0
            ));


    float fade =
        1.0
        -
        smoothstep(
            oceanOuterKm
            *
            0.75,
            oceanOuterKm
            *
            0.95,
            extent);


    wave(
        up,
        vec3(
            0.81,
            0.24,
            0.53),
        0.650,
        0.0032,
        0.46,
        1.17,
        spacing,
        fade,
        tangent,
        bitangent,
        displacement,
        dx,
        dz);


    wave(
        up,
        vec3(
            -0.41,
            0.86,
            0.29),
        0.430,
        0.0018,
        0.50,
        2.83,
        spacing,
        fade,
        tangent,
        bitangent,
        displacement,
        dx,
        dz);


    wave(
        up,
        vec3(
            0.17,
            -0.51,
            0.84),
        0.310,
        0.0011,
        0.48,
        4.97,
        spacing,
        fade,
        tangent,
        bitangent,
        displacement,
        dx,
        dz);


    wave(
        up,
        vec3(
            -0.77,
            0.39,
            0.51),
        0.180,
        0.00035,
        0.35,
        7.41,
        spacing,
        fade,
        tangent,
        bitangent,
        displacement,
        dx,
        dz);


    wave(
        up,
        vec3(
            0.58,
            0.74,
            -0.34),
        0.055,
        0.00012,
        0.30,
        11.23,
        spacing,
        fade,
        tangent,
        bitangent,
        displacement,
        dx,
        dz);


    // This is the real displaced rasterized surface.
    vec3 relative =
        oceanAnchorRelative
        +
        (
            baseOffset
            +
            displacement
        )
        *
        worldUnitsPerKm;


    // Retain the analytic per-vertex Gerstner normal.
    vsOut.worldNormal =
        normalize(
            cross(
                dx,
                dz));


    gl_Position =
        projection
        *
        vec4(
            mat3(
                view)
            *
            relative,
            1.0);
}