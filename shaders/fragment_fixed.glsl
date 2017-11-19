#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) out vec4 frag_color;

uniform vec4 fixed_color;

void main() {
    frag_color = fixed_color;
}
