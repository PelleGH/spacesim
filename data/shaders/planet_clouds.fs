#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec3 fragLocalDir;

out vec4 finalColor;

uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float seed;
uniform float cloudStrength;

float hash(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 19.19 + seed * 1.3);
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

    for (int i = 0; i < 6; ++i)
    {
        value += noise(p) * amp;
        p *= 2.02;
        amp *= 0.5;
    }

    return value;
}

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 D = normalize(fragLocalDir);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - fragWorldPos);

    vec3 seedOffset = vec3(seed * 9.1, seed * 21.7, seed * 47.3);

    vec3 warp;
    warp.x = fbm(D * 2.8 + seedOffset + vec3(3.0, 0.0, 7.0));
    warp.y = fbm(D * 2.8 + seedOffset + vec3(0.0, 5.0, 11.0));
    warp.z = fbm(D * 2.8 + seedOffset + vec3(9.0, 2.0, 0.0));
    warp = (warp - 0.5) * 0.12;

    vec3 P = normalize(D + warp);

    // Large weather systems.
    float systems = fbm(P * 2.2 + seedOffset);

    // Stretched/wispy detail. This gives clouds some flow instead of round blobs.
    vec3 flowP = normalize(P + vec3(
        fbm(P * 3.0 + seedOffset * 1.3),
        fbm(P * 3.0 + seedOffset * 2.1),
        fbm(P * 3.0 + seedOffset * 3.7)
    ) * 0.16);

    float bands = fbm(vec3(flowP.x * 5.5, flowP.y * 2.0, flowP.z * 5.5) + seedOffset * 1.9);
    float wisps = fbm(flowP * 18.0 + seedOffset * 3.2);

    float cloud = systems * 0.48 + bands * 0.36 + wisps * 0.16;

    // More coverage, still keeps decent shape.
    float alpha = smoothstep(0.52, 0.70, cloud);
    alpha *= 0.70 + wisps * 0.30;
    alpha *= cloudStrength;

    float ndotlRaw = dot(N, L);
    float ndotl = max(ndotlRaw, 0.0);
    float day = smoothstep(-0.10, 0.22, ndotlRaw);
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);

    vec3 cloudColor = vec3(0.88, 0.90, 0.92) * (0.16 + day * (0.55 + ndotl * 0.35));
    cloudColor += vec3(0.08, 0.18, 0.28) * rim * day * 0.10;

    cloudColor = cloudColor / (cloudColor + vec3(1.0));
    cloudColor = pow(cloudColor, vec3(1.0 / 2.2));

    finalColor = vec4(cloudColor, alpha * 0.82);
}