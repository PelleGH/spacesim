#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec3 fragLocalDir;

out vec4 finalColor;

uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float seed;

uniform vec3 colorA;
uniform vec3 colorB;
uniform vec3 colorC;

float hash(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33 + seed);
    return fract((p.x + p.y) * p.z);
}

float noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float n000 = hash(i + vec3(0,0,0));
    float n100 = hash(i + vec3(1,0,0));
    float n010 = hash(i + vec3(0,1,0));
    float n110 = hash(i + vec3(1,1,0));
    float n001 = hash(i + vec3(0,0,1));
    float n101 = hash(i + vec3(1,0,1));
    float n011 = hash(i + vec3(0,1,1));
    float n111 = hash(i + vec3(1,1,1));

    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);

    float nxy0 = mix(nx00, nx10, f.y);
    float nxy1 = mix(nx01, nx11, f.y);

    return mix(nxy0, nxy1, f.z);
}

float fbm(vec3 p)
{
    float value = 0.0;
    float amp = 0.5;

    for (int i = 0; i < 5; ++i)
    {
        value += noise(p) * amp;
        p *= 2.03;
        amp *= 0.5;
    }

    return value;
}
float StormOval(float lon, float lat, float centerLon, float centerLat, float width, float height)
{
    float dLon = lon - centerLon;
    dLon = atan(sin(dLon), cos(dLon));

    float dLat = lat - centerLat;

    return exp(-(dLon * dLon / width + dLat * dLat / height));
}
void main()
{
    vec3 N = normalize(fragNormal);
    vec3 D = normalize(fragLocalDir);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - fragWorldPos);

    float lat = D.y;
    float lon = atan(D.z, D.x);

    // Stronger horizontal flow distortion.
    float flowA = fbm(vec3(lon * 0.7, lat * 5.0, seed * 6.0));
    float flowB = fbm(vec3(lon * 2.2, lat * 11.0, seed * 13.0));
    float flow = flowA * 0.65 + flowB * 0.35;

    float warpedLat = lat + (flow - 0.5) * 0.075;

    // Main gas giant bands.
    float wideBands = sin(warpedLat * 18.0 + seed * 8.0);
    float midBands  = sin(warpedLat * 37.0 + flow * 3.5 + seed * 17.0);
    float thinBands = sin(warpedLat * 82.0 + flow * 8.0 + seed * 29.0);

    float bandValue =
        wideBands * 0.55 +
        midBands  * 0.30 +
        thinBands * 0.15;

    bandValue = bandValue * 0.5 + 0.5;

    // Long east-west streaks. This breaks the perfectly smooth band look.
    float streakA = fbm(vec3(lon * 10.0, warpedLat * 42.0, seed * 19.0));
    float streakB = fbm(vec3(lon * 24.0, warpedLat * 90.0, seed * 31.0));
    float streaks = streakA * 0.65 + streakB * 0.35;

    // Base colors.
    vec3 cream = colorB;
    vec3 tan   = colorA;
    vec3 brown = colorC;

    vec3 color = mix(brown, tan, smoothstep(0.12, 0.55, bandValue));
    color = mix(color, cream, smoothstep(0.48, 0.88, bandValue));

    // Dark thin belts.
    float darkBelts = smoothstep(0.72, 0.92, midBands * 0.5 + 0.5);
    color = mix(color, brown * 0.82, darkBelts * 0.22);

    // Bright thin zones.
    float brightZones = smoothstep(0.72, 0.94, thinBands * 0.5 + 0.5);
    color = mix(color, cream * 1.12, brightZones * 0.16);

    // Horizontal cloud texture.
    color *= 0.82 + streaks * 0.30;

// ------------------------------------------------------------
// PROCEDURAL STORMS
// ------------------------------------------------------------
// Several subtle storms, not one huge painted oval.

float stormMask = 0.0;
float stormRing = 0.0;
float stormWake = 0.0;

for (int i = 0; i < 4; ++i)
{
    float fi = float(i);

    // Deterministic pseudo-random storm placement.
    float stormCenterLon = fract(seed * 12.73 + fi * 0.271) * 6.2831853 - 3.1415926;
    float stormCenterLat = mix(-0.42, 0.42, fract(seed * 7.91 + fi * 0.413));

    float dLon = lon - stormCenterLon;
    dLon = atan(sin(dLon), cos(dLon));

    float dLat = lat - stormCenterLat;

    // Size variation.
    float width = mix(8.0, 18.0, fract(seed * 5.31 + fi * 0.619));
    float height = mix(110.0, 260.0, fract(seed * 3.17 + fi * 0.731));

    // Flow distortion so storms are not perfect ovals.
    float localFlow = fbm(vec3((lon + fi) * 8.0, lat * 32.0, seed * 18.0 + fi));
    dLon += (localFlow - 0.5) * 0.12 * exp(-dLat * dLat * 90.0);

    float oval = exp(-(dLon * dLon * width + dLat * dLat * height));

    float detail = fbm(vec3(dLon * 28.0, dLat * 90.0, seed * 43.0 + fi));
    float shaped = oval * (0.70 + detail * 0.35);

    stormMask = max(stormMask, shaped);

    float ring =
        smoothstep(0.16, 0.34, shaped) *
        (1.0 - smoothstep(0.42, 0.68, shaped));

    stormRing = max(stormRing, ring * (0.75 + detail * 0.25));

    float wake =
        exp(-(dLon * dLon * 3.0 + dLat * dLat * 55.0)) *
        smoothstep(-0.16, 0.45, dLon);

    stormWake = max(stormWake, wake * 0.45);
}

stormMask = clamp(stormMask, 0.0, 1.0);
stormRing = clamp(stormRing, 0.0, 1.0);
stormWake = clamp(stormWake, 0.0, 1.0);

// Wake gently brightens/warps nearby bands.
float wakeNoise = fbm(vec3(lon * 20.0, lat * 60.0, seed * 52.0));
vec3 wakeColor = mix(colorA * 0.92, colorB * 1.04, wakeNoise);
color = mix(color, wakeColor, stormWake * 0.22);

// Palette-relative storm colors.
// This avoids hardcoding a Jupiter-only red spot.
float core = smoothstep(0.34, 0.76, stormMask);
float eye = smoothstep(0.76, 0.94, stormMask);

vec3 stormBody = mix(colorB * 1.04, colorA * 1.10, 0.42);
vec3 stormRim = colorC * 0.38;
vec3 stormEyeColor = colorB * 1.12;

color = mix(color, stormRim, stormRing * 0.62);
color = mix(color, stormBody, core * 0.28);
color = mix(color, stormEyeColor, eye * 0.05);
    float ndotlRaw = dot(N, L);
    float ndotl = max(ndotlRaw, 0.0);
    float day = smoothstep(-0.18, 0.24, ndotlRaw);

    vec3 ambient = vec3(0.025, 0.026, 0.032);
    vec3 lit = color * (ambient + day * (0.42 + ndotl * 0.85));

    // Soft cloud-top rim.
    float horizon = pow(1.0 - max(dot(N, V), 0.0), 2.1);
    lit += cream * horizon * day * 0.10;

    lit = lit / (lit + vec3(1.0));
    lit = pow(lit, vec3(1.0 / 2.2));

    finalColor = vec4(lit, 1.0);
}