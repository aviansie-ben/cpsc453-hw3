#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec3 position;

uniform mat4 vertex_transform = mat4(1.0);

void main() {
    gl_Position = vec4(position, 1.0) * vertex_transform;
}
