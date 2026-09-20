#version 450 core

layout(location = 0) out vec3 vDirection;

uniform mat4 inverseViewProjection;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 p =
        positions[gl_VertexID];

    vec4 world =
        inverseViewProjection *
        vec4(
            p,
            1.0,
            1.0);

    world.xyz /=
        world.w;

    vDirection =
        world.xyz;

    gl_Position =
        vec4(
            p,
            1.0,
            1.0);
}