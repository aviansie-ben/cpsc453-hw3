#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 unnormalized_normal;

layout(location = 0) out vec4 frag_color;

struct PointLight {
    vec3 pos;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float a0;
    float a1;
    float a2;
};

struct Material {
    vec3 ambient;

    vec3 diffuse;
    sampler2D diffuse_map;

    vec3 specular;
    sampler2D specular_map;

    float shininess;
};

uniform vec3 camera_position;
uniform vec3 scene_ambient;

#define MAX_POINT_LIGHTS 16
uniform int num_point_lights;
uniform PointLight point_lights[MAX_POINT_LIGHTS];

uniform Material material;

float calc_attenuation(PointLight light, vec3 pos) {
    float distance = length(light.pos - pos);

    return 1.0 / (light.a0 + light.a1 * distance + light.a2 * distance * distance);
}

vec3 calc_point_light(PointLight light, vec3 view_dir, vec3 normal) {
    vec3 light_dir = normalize(light.pos - position);

    // Calculate ambient light
    vec3 ambient = light.ambient
        * vec3(texture(material.diffuse_map, tex_coord))
        * material.ambient;

    // Calculate diffuse light
    vec3 diffuse = max(dot(normal, light_dir), 0.0)
        * light.diffuse
        * vec3(texture(material.diffuse_map, tex_coord))
        * material.diffuse;

    // Calculate specular light
    vec3 specular = pow(max(dot(view_dir, reflect(-light_dir, normal)), 0.0), material.shininess)
        * light.specular
        * vec3(texture(material.specular_map, tex_coord))
        * material.specular;

    // Add effects together and apply attenuation
    return (ambient + diffuse + specular) * calc_attenuation(light, position);
}

vec3 calc_scene_ambient() {
    return scene_ambient
        * vec3(texture(material.diffuse_map, tex_coord))
        * material.ambient;
}

void main() {
    vec3 normal = normalize(unnormalized_normal);
    vec3 view_dir = normalize(camera_position - position);

    // Apply initial scene ambient lighting
    vec3 result = calc_scene_ambient();

    // Apply light from each point light
    for (int i = 0; i < num_point_lights; i++) {
        result += calc_point_light(point_lights[i], view_dir, normal);
    }

    frag_color = vec4(result, 1.0);
}
