#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 position_out;
layout(location = 1) out vec2 tex_coord_out;
layout(location = 2) out vec3 normal_out;

uniform mat4 vertex_transform = mat4(1.0);
uniform mat4 vertex_world_transform = mat4(1.0);
uniform mat3 normal_transform = mat3(1.0);

void main() {
    gl_Position = vec4(position, 1.0) * vertex_transform;
    position_out = vec3(vec4(position, 1.0) * vertex_world_transform);
    tex_coord_out = tex_coord;
    normal_out = normal * normal_transform;
}
