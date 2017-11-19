#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec2 tex_coord;
uniform sampler2D tex;
uniform vec3 color;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = vec4(color, texture(tex, tex_coord).r);
}
