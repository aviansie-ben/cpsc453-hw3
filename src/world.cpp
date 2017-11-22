#include <glm/gtc/matrix_transform.hpp>

#include "shaderimpl.hpp"
#include "world.hpp"

namespace hw3 {
    glm::mat4 Orientation::apply(const glm::mat4& transform) const {
        // TODO Implement this
        return transform;
    }

    glm::mat4 Object::transform_matrix() const {
        return this->m_orientation.apply(glm::translate(glm::mat4(1), this->m_pos));
    }

    void Object::draw(
        ShaderProgram& program,
        const RenderSettings& render_settings,
        const glm::mat4& view_projection_matrix
    ) const {
        auto model_matrix = this->transform_matrix();

        program.set_uniform("vertex_transform", view_projection_matrix * model_matrix);
        program.set_uniform("vertex_world_transform", model_matrix);
        program.set_uniform("normal_transform", glm::transpose(glm::inverse(glm::mat3(model_matrix))));

        if (render_settings.draw_textures) {
            program.set_uniform("material", this->m_material);

            if (!render_settings.use_ambient_occlusion) {
                Sampler2D blank(Texture2D::single_pixel());

                program.set_uniform("material.ambient_occlusion_map", &blank);
            }
        } else {
            program.set_uniform("material", this->m_material.without_maps());
        }

        program.use();

        this->m_model->draw();

        if (render_settings.draw_bounding_boxes) {
            this->m_model->bounding_box().draw(
                view_projection_matrix * model_matrix,
                glm::vec4(1, 0, 0, 1)
            );
        }
    }

    void ShaderProgram::uniform_setter<PointLight>::operator ()(
        ShaderProgram& program,
        std::string name,
        const PointLight& value
    ) {
        program.set_uniform(name + ".pos", value.pos);

        program.set_uniform(name + ".ambient", value.ambient);
        program.set_uniform(name + ".diffuse", value.diffuse);
        program.set_uniform(name + ".specular", value.specular);

        program.set_uniform(name + ".a0", value.a0);
        program.set_uniform(name + ".a1", value.a1);
        program.set_uniform(name + ".a2", value.a2);
    }

    ShaderProgram& World::select_program() const {
        switch (this->m_render_settings.mode) {
        case RenderMode::STANDARD:
        case RenderMode::FULL_BRIGHT:
            return shaders::phong_program;
        case RenderMode::NORMALS:
            return shaders::normal_program;
        default:
            throw std::runtime_error("Invalid render mode");
        }
    }

    void World::draw() const {
        auto& program = this->select_program();
        auto view_projection_matrix = this->camera().view_projection_matrix();

        program.set_uniform("camera_position", this->camera().pos());

        if (this->m_point_lights.size() > 16) {
            throw std::runtime_error("Too many point lights");
        }

        if (this->m_render_settings.mode == RenderMode::STANDARD) {
            program.set_uniform("scene_ambient", glm::vec3(0));
            program.set_uniform("num_point_lights", static_cast<int>(this->m_point_lights.size()));
            for (size_t i = 0; i < this->m_point_lights.size(); i++) {
                program.set_uniform(([&]() {
                    std::ostringstream ss;

                    ss << "point_lights[" << i << "]";

                    return ss.str();
                })(), *this->m_point_lights[i]);
            }
        } else {
            program.set_uniform("scene_ambient", glm::vec3(1));
            program.set_uniform("num_point_lights", 0);
        }

        for (const auto& obj : this->m_objects) {
            obj->draw(program, this->m_render_settings, view_projection_matrix);
        }
    }
}
