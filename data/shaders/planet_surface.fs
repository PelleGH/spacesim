#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec3 fragLocalDir;

out vec4 finalColor;

uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float seed;
uniform float seaLevel;
uniform int planetType;

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
        p *= 2.01;
        amp *= 0.5;
    }

    return value;
}

vec3 WarpDirection(vec3 D, vec3 seedOffset)
{
    vec3 warp;
    warp.x = fbm(D * 1.6 + seedOffset + vec3(4.1, 0.0, 2.7));
    warp.y = fbm(D * 1.6 + seedOffset + vec3(0.0, 8.3, 5.2));
    warp.z = fbm(D * 1.6 + seedOffset + vec3(6.7, 2.4, 0.0));

    // Keep this low or the continents smear.
    warp = (warp - 0.5) * 0.18;

    return normalize(D + warp);
}

float HeightField(vec3 P, vec3 seedOffset)
{
    float continentBase = fbm(P * 1.05 + seedOffset);
    float continentSecondary = fbm(P * 2.25 + seedOffset * 1.6);
    float localDetail = fbm(P * 8.0 + seedOffset * 2.5);

    float height = continentBase * 0.82 + continentSecondary * 0.16 + localDetail * 0.02;
    height = pow(height, 1.10);

    return height;
}
float ContinentField(vec3 P, vec3 seedOffset)
{
    // Start with large readable land masses.
    float baseA = fbm(P * 0.90 + seedOffset * 0.8);
    float baseB = fbm(P * 1.75 + seedOffset * 1.4);

    float h = baseA * 0.72 + baseB * 0.28;

    // Domain-warp only the coastline detail, not the whole continent.
    vec3 coastWarp;
    coastWarp.x = fbm(P * 3.0 + seedOffset * 2.1);
    coastWarp.y = fbm(P * 3.0 + seedOffset * 3.4);
    coastWarp.z = fbm(P * 3.0 + seedOffset * 4.7);
    coastWarp = (coastWarp - 0.5) * 0.18;

    vec3 C = normalize(P + coastWarp);

    // Higher-frequency coast breakup.
    float c1 = fbm(C * 6.5 + seedOffset * 5.1);
    float c2 = fbm(C * 13.0 + seedOffset * 6.2);

    // Ridged/noisy coastal erosion.
    float ridge = 1.0 - abs(c1 * 2.0 - 1.0);
    ridge = pow(ridge, 1.8);

    // Only modify near the sea threshold.
    float sea = 0.505;
    float nearSea = 1.0 - smoothstep(0.035, 0.18, abs(h - sea));

    float coastCarve =
        (c1 - 0.5) * 0.095 +
        (c2 - 0.5) * 0.045 -
        ridge * 0.030;

    h += coastCarve * nearSea;

    return h;
}
float PlainsReliefField(vec3 P, vec3 seedOffset)
{
    // Soft, broad variation only.
    float a = fbm(P * 6.0 + seedOffset * 3.5);
    float b = fbm(P * 12.0 + seedOffset * 5.0);
    return a * 0.7 + b * 0.3;
}

float HillsReliefField(vec3 P, vec3 seedOffset)
{
    // Medium detail, rolling terrain.
    float a = fbm(P * 8.0 + seedOffset * 4.2);
    float b = fbm(P * 16.0 + seedOffset * 5.8);
    return a * 0.6 + b * 0.4;
}

float MountainsReliefField(vec3 P, vec3 seedOffset)
{
    // Ridged shapes, but much less dense than before.
    float r1 = 1.0 - abs(fbm(P * 7.5 + seedOffset * 6.4) * 2.0 - 1.0);
    float r2 = 1.0 - abs(fbm(P * 14.0 + seedOffset * 8.1) * 2.0 - 1.0);
    float base = fbm(P * 5.0 + seedOffset * 4.7);

    return r1 * 0.45 + r2 * 0.30 + base * 0.25;
}

vec3 TerrainWeights(vec3 P, vec3 seedOffset, float coast)
{
    float broad = fbm(P * 1.25 + seedOffset * 2.1);
    float mid   = fbm(P * 2.80 + seedOffset * 4.3);

    float ridge = 1.0 - abs(fbm(P * 5.50 + seedOffset * 6.2) * 2.0 - 1.0);
    ridge = pow(ridge, 1.35);

    float rugged = broad * 0.35 + mid * 0.30 + ridge * 0.35;

    // More hills, fewer infinite flat plains.
    float mountains = smoothstep(0.62, 0.78, rugged);
    float hills = smoothstep(0.36, 0.66, rugged) * (1.0 - mountains);

    // Coast slightly reduces mountains, but does not erase all inland variation.
    mountains *= 1.0 - coast * 0.35;
    hills *= 1.0 - coast * 0.15;

    float plains = clamp(1.0 - hills - mountains, 0.0, 1.0);

    return vec3(plains, hills, mountains);
}
void main()
{
    vec3 Ngeo = normalize(fragNormal);
    vec3 D = normalize(fragLocalDir);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - fragWorldPos);

    vec3 seedOffset = vec3(seed * 11.3, seed * 37.7, seed * 71.9);

    // Use a cleaner direction for continent shape.
    // This keeps coastlines from becoming smeared by the detail warp.
    vec3 Pshape = normalize(D);

    // Use warped direction only for terrain/biome detail.
    vec3 P = WarpDirection(D, seedOffset);

    // Main continent/ocean field.
    // Use continent field for the actual land/ocean outline.
    // This is much better than HeightField for coastlines.
    float continent = ContinentField(Pshape, seedOffset);

    float sea = seaLevel;

    // Small AA band so edges are crisp but not jagged.
    float aa = max(fwidth(continent) * 1.2, 0.0015);

    float landMask = smoothstep(sea - aa, sea + aa, continent);
    float water = 1.0 - landMask;
    // Dry rocky worlds have no oceans.
    if (planetType == 1 || planetType == 2 || planetType == 3)
    {
        landMask = 1.0;
        water = 0.0;
    }
    // Narrow coast band so beaches don't blur the whole outline.
    float coast = 1.0 - smoothstep(0.0, 0.005, abs(continent - sea));
    coast *= landMask;

    float latitude = abs(D.y);

    vec3 deepOcean = vec3(0.004, 0.032, 0.070);
    vec3 shallowOcean = vec3(0.018, 0.090, 0.130);
    if (planetType == 4)
    {
        deepOcean = vec3(0.002, 0.045, 0.090);
        shallowOcean = vec3(0.015, 0.125, 0.170);
    }

    vec3 oceanColor = mix(deepOcean, shallowOcean, clamp(coast * 0.30, 0.0, 1.0));

    float moisture = fbm(P * 2.4 + seedOffset * 3.1);
    float temperature = clamp(
        1.0 - latitude * 0.85 + (fbm(P * 1.6 + seedOffset * 2.3) - 0.5) * 0.18,
        0.0,
        1.0
    );

    vec3 terrainW = TerrainWeights(P, seedOffset, coast);
    float plainsW = terrainW.x;
    float hillsW = terrainW.y;
    float mountainsW = terrainW.z;

    vec3 forest = vec3(0.035, 0.095, 0.040);
    vec3 grass = vec3(0.090, 0.145, 0.065);
    vec3 dryGrass = vec3(0.165, 0.155, 0.090);
    vec3 desert = vec3(0.300, 0.255, 0.155);
    vec3 rock = vec3(0.210, 0.205, 0.185);
    vec3 beach = vec3(0.380, 0.330, 0.205);
    vec3 snow = vec3(0.700, 0.730, 0.700);
    if (planetType == 1) // desert
    {
        forest = vec3(0.180, 0.120, 0.060);
        grass = vec3(0.300, 0.205, 0.100);
        dryGrass = vec3(0.420, 0.285, 0.130);
        desert = vec3(0.620, 0.440, 0.220);
        rock = vec3(0.300, 0.220, 0.170);
        beach = vec3(0.520, 0.380, 0.190);
        snow = vec3(0.700, 0.640, 0.560);
    }

    if (planetType == 2) // ice
    {
        forest = vec3(0.520, 0.600, 0.620);
        grass = vec3(0.600, 0.680, 0.700);
        dryGrass = vec3(0.480, 0.560, 0.600);
        desert = vec3(0.700, 0.760, 0.780);
        rock = vec3(0.430, 0.470, 0.500);
        beach = vec3(0.680, 0.720, 0.720);
        snow = vec3(0.850, 0.880, 0.890);
    }

    if (planetType == 3) // barren
    {
        forest = vec3(0.120, 0.105, 0.090);
        grass = vec3(0.170, 0.150, 0.125);
        dryGrass = vec3(0.210, 0.185, 0.150);
        desert = vec3(0.260, 0.230, 0.190);
        rock = vec3(0.300, 0.295, 0.275);
        beach = vec3(0.220, 0.190, 0.160);
        snow = vec3(0.480, 0.480, 0.460);
    }

    if (planetType == 4) // ocean world islands
    {
        forest = vec3(0.025, 0.085, 0.040);
        grass = vec3(0.075, 0.140, 0.060);
        dryGrass = vec3(0.145, 0.145, 0.080);
        desert = vec3(0.260, 0.235, 0.135);
        rock = vec3(0.190, 0.190, 0.175);
        beach = vec3(0.420, 0.360, 0.210);
        snow = vec3(0.720, 0.760, 0.760);
    }
    // Biome colors for flatter land.
    float desertMask = smoothstep(0.55, 0.82, temperature) *
                    (1.0 - smoothstep(0.38, 0.65, moisture));

    float forestMask = smoothstep(0.55, 0.82, moisture) *
                    (1.0 - desertMask);

    float biomePatch = fbm(P * 1.35 + seedOffset * 8.1);
    float biomePatch2 = fbm(P * 3.25 + seedOffset * 9.7);

    vec3 plainsColor = mix(dryGrass, grass, moisture);

    // Add large regional patches so plains do not become one flat fill.
    plainsColor = mix(plainsColor, dryGrass * 1.18, smoothstep(0.55, 0.78, biomePatch) * 0.45);
    plainsColor = mix(plainsColor, forest * 1.08, smoothstep(0.58, 0.82, biomePatch2) * forestMask * 0.75);

    plainsColor = mix(plainsColor, desert, desertMask * 0.90);
    plainsColor = mix(plainsColor, forest, forestMask * 0.55);

    vec3 hillsColor = mix(dryGrass * 0.92, grass * 0.95, moisture * 0.75);
    hillsColor = mix(hillsColor, rock * 0.95, 0.38);

    vec3 mountainsColor = mix(vec3(0.125, 0.125, 0.105), rock * 1.08, 0.82);

    vec3 landColor =
        plainsColor * plainsW +
        hillsColor * hillsW +
        mountainsColor * mountainsW;

    // Prevent land from collapsing into one average green-gray tone.
    float landContrast = fbm(P * 4.0 + seedOffset * 10.4);
    landColor *= 0.86 + landContrast * 0.24;

    // Darken hills/mountains slightly so terrain regions read at orbit scale.
    landColor *= 1.0 - hillsW * 0.06 - mountainsW * 0.14;

    landColor = mix(landColor, beach, coast * 0.22);

    // Keep polar snow subtle for normal Earth/ocean worlds.
    float polar = smoothstep(0.93, 0.985, latitude);
    float snowMask = (polar * 0.45 + mountainsW * polar * 0.20) * landMask;
    landColor = mix(landColor, snow, clamp(snowMask, 0.0, 0.35));

    // Class-specific surface identity.
    if (planetType == 1) // desert
    {
        // Dune-like broad streaks and darker rocky belts.
        float duneA = fbm(vec3(P.x * 13.0, P.y * 3.0, P.z * 13.0) + seedOffset * 5.7);
        float duneB = fbm(vec3(P.x * 28.0, P.y * 5.0, P.z * 28.0) + seedOffset * 8.3);
        float dune = duneA * 0.70 + duneB * 0.30;

        float canyon = 1.0 - abs(fbm(P * 7.0 + seedOffset * 11.0) * 2.0 - 1.0);
        canyon = smoothstep(0.55, 0.82, canyon);

        landColor *= 0.92 + dune * 0.18;
        landColor = mix(landColor, vec3(0.32, 0.205, 0.115), canyon * 0.18);
    }

    if (planetType == 2) // ice
    {
        // Push away from moon-gray and toward blue-white ice.
        vec3 iceBlue = vec3(0.66, 0.78, 0.84);
        vec3 snowWhite = vec3(0.86, 0.90, 0.90);
        vec3 deepCrack = vec3(0.28, 0.38, 0.44);

        float glacier = fbm(P * 3.5 + seedOffset * 4.2);
        float iceFlow = fbm(vec3(P.x * 8.0, P.y * 2.0, P.z * 8.0) + seedOffset * 7.5);

        // Thin dark ice fracture network.
        float crackA = 1.0 - abs(fbm(P * 16.0 + seedOffset * 12.0) * 2.0 - 1.0);
        float crackB = 1.0 - abs(fbm(P * 32.0 + seedOffset * 16.0) * 2.0 - 1.0);
        float cracks = smoothstep(0.78, 0.94, crackA * 0.65 + crackB * 0.35);

        landColor = mix(iceBlue, snowWhite, glacier * 0.65 + iceFlow * 0.25);
        landColor = mix(landColor, deepCrack, cracks * 0.22);

        // Ice worlds should be brighter and less rock-like.
        landColor *= 1.08;
    }

    if (planetType == 3) // barren
    {
        // Airless rock/moon-like variation.
        float rough = fbm(P * 13.0 + seedOffset * 6.5);
        float basalt = smoothstep(0.60, 0.84, rough);

        landColor *= 0.82 + rough * 0.28;
        landColor = mix(landColor, vec3(0.115, 0.105, 0.095), basalt * 0.28);
    }

    float micro = fbm(P * 18.0 + seedOffset * 5.0);
    landColor *= 0.98 + micro * 0.04;
    oceanColor *= 0.99 + micro * 0.01;

    vec3 albedo = mix(oceanColor, landColor, landMask);

    // --- Fake normal / bump from terrain classes ---
    vec3 tangent = normalize(abs(Ngeo.y) < 0.99 ? cross(vec3(0.0, 1.0, 0.0), Ngeo)
                                                : cross(vec3(1.0, 0.0, 0.0), Ngeo));
    vec3 bitangent = normalize(cross(Ngeo, tangent));

    float eps = 0.0065;

    // Use one cheaper relief source for normals.
    // Terrain classes still affect strength, but we avoid stacking too many relief samples.
    float relief = fbm(P * 9.0 + seedOffset * 4.0) * 0.65 +
                fbm(P * 18.0 + seedOffset * 6.0) * 0.35;

    vec3 Ptx = WarpDirection(normalize(D + tangent * eps), seedOffset);
    vec3 Pty = WarpDirection(normalize(D + bitangent * eps), seedOffset);

    float rx = fbm(Ptx * 9.0 + seedOffset * 4.0) * 0.65 +
            fbm(Ptx * 18.0 + seedOffset * 6.0) * 0.35;

    float ry = fbm(Pty * 9.0 + seedOffset * 4.0) * 0.65 +
            fbm(Pty * 18.0 + seedOffset * 6.0) * 0.35;

    float drx = (rx - relief) / eps;
    float dry = (ry - relief) / eps;

    // Much subtler orbit-scale bump.
    float biomeBump =
        plainsW * 0.004 +
        hillsW * 0.014 +
        mountainsW * 0.032;

    float bumpStrength = landMask * biomeBump;
    bumpStrength *= smoothstep(0.35, 0.85, landMask);

    vec3 N = normalize(Ngeo - tangent * drx * bumpStrength - bitangent * dry * bumpStrength);
    // -----------------------------------------------------------

    float ndotlRaw = dot(N, L);
    float ndotl = max(ndotlRaw, 0.0);
    float day = smoothstep(-0.12, 0.22, ndotlRaw);

    vec3 ambient = vec3(0.015, 0.020, 0.030);
    vec3 color = albedo * (ambient + day * (0.28 + ndotl * 1.05));

    float oceanFresnel = pow(1.0 - max(dot(Ngeo, V), 0.0), 3.0);
    color += vec3(0.035, 0.115, 0.190) * oceanFresnel * water * day;

    vec3 oceanR = reflect(-L, Ngeo);
    float specAngle = max(dot(oceanR, V), 0.0);
    float oceanGlint = pow(specAngle, 650.0) * water * smoothstep(0.10, 0.50, ndotl);
    color += vec3(1.0, 0.95, 0.85) * oceanGlint * 0.24;

    // Very subtle horizon tint baked into the surface shader.
    float horizon = pow(1.0 - max(dot(Ngeo, V), 0.0), 2.8);
    color += vec3(0.020, 0.070, 0.150) * horizon * day * 0.5;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    finalColor = vec4(color, 1.0);
}