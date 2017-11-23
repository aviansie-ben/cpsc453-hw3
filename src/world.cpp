#include <glm/gtc/matrix_transform.hpp>

#include "shaderimpl.hpp"
#include "world.hpp"

namespace hw3 {
    glm::mat4 Orientation::apply(const glm::mat4& transform) const {
        return glm::rotate(
            glm::rotate(
                glm::rotate(transform, this->yaw, glm::vec3(0, 1, 0)),
                this->pitch,
                glm::vec3(1, 0, 0)
            ),
            this->roll,
            glm::vec3(0, 0, 1)
        );
    }

    glm::mat4 Object::transform_matrix() const {
        return this->m_orientation.apply(
            glm::scale(glm::translate(glm::mat4(1), this->m_pos), glm::vec3(this->m_scale))
        );
    }

    AABB Object::bounding_box() const {
        return this->m_model->bounding_box() * this->m_scale;
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

        {
            Material m = this->m_material;

            if (!render_settings.draw_textures) {
                m = m.without_maps();
            } else {
                if (!render_settings.use_ambient_occlusion) {
                    m = m.without_ao();
                }
            }

            program.set_uniform("material", m);
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

    void OrbitControls::begin_rotate(glm::vec2 pos) {
        if (this->m_state == OrbitState::NONE) {
            this->m_state = OrbitState::ROTATING;
            this->m_last_pos = pos;
        }
    }

    void OrbitControls::begin_pan(glm::vec2 pos) {
        if (this->m_state == OrbitState::NONE) {
            this->m_state = OrbitState::PANNING;
            this->m_last_pos = pos;
        }
    }

    static glm::vec3 calculate_posball(glm::vec2 pos) {
        float dist_sq = pos.x * pos.x + pos.y * pos.y;

        if (dist_sq <= 1) {
            return glm::vec3(pos, std::sqrt(1 - dist_sq));
        } else {
            return glm::vec3(glm::normalize(pos), 0);
        }
    }

    void OrbitControls::move_cursor(glm::vec2 pos) {
        switch (this->m_state) {
        case OrbitState::ROTATING: {
            auto camera_orientation = this->m_camera->orientation_matrix();
            auto camera_pos = this->m_camera->pos();

            auto last_posball = calculate_posball(this->m_last_pos);
            auto posball = calculate_posball(pos);

            float angle = std::acos(std::min(1.0f, glm::dot(last_posball, posball)));
            auto axis = glm::inverse(camera_orientation)
                * glm::cross(last_posball, posball);

            // If posball == last_posball, then glm::cross(posball, last_posball) will be the null
            // vector and thus the axis of rotation will also be the null vector. Trying to rotate
            // using the null vector as an axis causes the resulting rotation matrix to be full of
            // NaNs, which breaks everything. Instead, we should just cancel the rotation.
            if (axis == glm::vec3()) {
                break;
            }

            auto rotate_matrix = glm::mat3(glm::rotate(
                glm::mat4(1),
                angle,
                axis
            ));

            camera_orientation = camera_orientation * rotate_matrix;
            camera_pos = this->m_rotate_origin
                + glm::transpose(rotate_matrix) * (camera_pos - this->m_rotate_origin);

            this->m_camera->orientation_matrix(camera_orientation);
            this->m_camera->pos(camera_pos);

            break;
        }
        case OrbitState::PANNING: {
            auto delta3 = glm::inverse(this->m_camera->orientation_matrix())
                * glm::vec3(this->m_last_pos - pos, 0)
                * glm::distance(this->m_camera->pos(), this->m_rotate_origin);

            this->m_camera->pos(this->m_camera->pos() + delta3);
            this->m_rotate_origin += delta3;

            break;
        }
        case OrbitState::NONE:
            break; // Do nothing
        }

        this->m_last_pos = pos;
    }

    void OrbitControls::handle_zoom(float delta) {
        glm::vec3 camera_look = this->m_camera->pos() - this->m_rotate_origin;

        this->m_camera->pos(this->m_rotate_origin + camera_look * std::pow(2.0f, delta * -0.25f));
    }

    void OrbitControls::end_rotate() {
        if (this->m_state == OrbitState::ROTATING) {
            this->m_state = OrbitState::NONE;
        }
    }

    void OrbitControls::end_pan() {
        if (this->m_state == OrbitState::PANNING) {
            this->m_state = OrbitState::NONE;
        }
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
