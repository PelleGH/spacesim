#version 450 core

layout(location = 0) out vec4 outColor;

in VS_OUT
{
    vec3 worldPosition;
    vec3 worldNormal;
    vec2 texCoord;
} fsIn;

uniform vec3 cameraPosition;

// Direction FROM the surface TOWARD the star.
uniform vec3 sunDirection;

// HDR radiance. This is deliberately allowed to be > 1.
uniform vec3 sunRadiance;

uniform vec3 baseColor;
uniform float metallic;
uniform float roughness;

const float PI =
    3.14159265359;

float distributionGGX(
    vec3 N,
    vec3 H,
    float perceptualRoughness)
{
    float a =
        perceptualRoughness *
        perceptualRoughness;

    float a2 =
        a * a;

    float NdotH =
        max(dot(N, H), 0.0);

    float NdotH2 =
        NdotH * NdotH;

    float denominator =
        NdotH2 * (a2 - 1.0) + 1.0;

    denominator =
        PI *
        denominator *
        denominator;

    return
        a2 /
        max(denominator, 0.000001);
}

float geometrySchlickGGX(
    float NdotV,
    float perceptualRoughness)
{
    float r =
        perceptualRoughness + 1.0;

    float k =
        (r * r) / 8.0;

    return
        NdotV /
        (
            NdotV *
            (1.0 - k) +
            k
        );
}

float geometrySmith(
    vec3 N,
    vec3 V,
    vec3 L,
    float perceptualRoughness)
{
    float NdotV =
        max(dot(N, V), 0.0);

    float NdotL =
        max(dot(N, L), 0.0);

    float ggxV =
        geometrySchlickGGX(
            NdotV,
            perceptualRoughness);

    float ggxL =
        geometrySchlickGGX(
            NdotL,
            perceptualRoughness);

    return ggxV * ggxL;
}

vec3 fresnelSchlick(
    float cosTheta,
    vec3 F0)
{
    return
        F0 +
        (1.0 - F0) *
        pow(
            1.0 - cosTheta,
            5.0);
}

void main()
{
    vec3 N =
        normalize(fsIn.worldNormal);

    vec3 V =
        normalize(
            cameraPosition -
            fsIn.worldPosition);

    vec3 L =
        normalize(sunDirection);

    vec3 H =
        normalize(V + L);

    float NdotL =
        max(dot(N, L), 0.0);

    float NdotV =
        max(dot(N, V), 0.0);

    vec3 F0 =
        vec3(0.04);

    F0 =
        mix(
            F0,
            baseColor,
            metallic);

    float NDF =
        distributionGGX(
            N,
            H,
            roughness);

    float G =
        geometrySmith(
            N,
            V,
            L,
            roughness);

    vec3 F =
        fresnelSchlick(
            max(dot(H, V), 0.0),
            F0);

    vec3 numerator =
        NDF *
        G *
        F;

    float denominator =
        max(
            4.0 *
            NdotV *
            NdotL,
            0.0001);

    vec3 specular =
        numerator /
        denominator;

    vec3 kS =
        F;

    vec3 kD =
        vec3(1.0) -
        kS;

    // Metals don't have diffuse reflection.
    kD *=
        1.0 - metallic;

    vec3 diffuse =
        kD *
        baseColor /
        PI;

    vec3 directLighting =
        (
            diffuse +
            specular
        ) *
        sunRadiance *
        NdotL;

    // Deliberately no fake ambient light yet.
    outColor =
        vec4(
            directLighting,
            1.0);
}