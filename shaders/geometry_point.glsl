#version 330
#extension GL_ARB_separate_shader_objects : require

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 point_half_size;

void main() {
    gl_Position = gl_in[0].gl_Position + gl_in[0].gl_Position.w * vec4(-point_half_size.x, -point_half_size.y, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + gl_in[0].gl_Position.w * vec4(-point_half_size.x, point_half_size.y, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + gl_in[0].gl_Position.w * vec4(point_half_size.x, -point_half_size.y, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + gl_in[0].gl_Position.w * vec4(point_half_size.x, point_half_size.y, 0.0, 0.0);
    EmitVertex();
}
