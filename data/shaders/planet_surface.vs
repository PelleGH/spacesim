#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragWorldPos;
out vec3 fragNormal;
out vec3 fragLocalDir;

void main()
{
    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);

    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(matModel) * vertexNormal);
    fragLocalDir = normalize(vertexPosition);

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}