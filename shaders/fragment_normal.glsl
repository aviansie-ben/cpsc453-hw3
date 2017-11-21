#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 unnormalized_normal;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = vec4((normalize(unnormalized_normal) + 1.0) / 2.0, 1.0);
}
