#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec3 fragLocalDir;

out vec4 finalColor;

uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float atmosphereStrength;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - fragWorldPos);

    float nv = dot(N, V);
    if (nv <= 0.0)
    {
        discard;
    }

    float viewDot = clamp(nv, 0.0, 1.0);
    float rim = pow(1.0 - viewDot, 4.5);
    float day = smoothstep(-0.18, 0.32, dot(N, L));

    vec3 atmoColor = vec3(0.07, 0.22, 0.60);
    float alpha = rim * day * 0.08 * atmosphereStrength;
    alpha += pow(1.0 - viewDot, 12.0) * day * 0.035 * atmosphereStrength;

    finalColor = vec4(atmoColor, alpha);
}
