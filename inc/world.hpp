#ifndef HW3_WORLD_HPP
#define HW3_WORLD_HPP

#include <memory>
#include <sstream>
#include <vector>

#include <boost/filesystem.hpp>
#include <glm/glm.hpp>

#include "objmodel.hpp"
#include "shader.hpp"

namespace hw3 {
    struct Orientation {
        float yaw;
        float pitch;
        float roll;

        Orientation() : yaw(0), pitch(0), roll(0) {}
        Orientation(float yaw, float pitch, float roll) : yaw(yaw), pitch(pitch), roll(roll) {}

        glm::mat4 apply(const glm::mat4& transform) const;
    };

    enum class RenderMode {
        STANDARD,
        FULL_BRIGHT,
        NORMALS
    };

    struct RenderSettings {
        RenderMode mode = RenderMode::STANDARD;
        bool use_ambient_occlusion = true;
        bool draw_textures = true;
        bool draw_bounding_boxes = false;
    };

    class Object {
        std::shared_ptr<Model3D> m_model;
        Material m_material;

        glm::vec3 m_pos;
        Orientation m_orientation;
        float m_scale = 1.0f;

        glm::mat4 transform_matrix() const;
    public:
        Object(std::shared_ptr<Model3D> model, Material material)
            : m_model(std::move(model)), m_material(std::move(material)) {}

        const std::shared_ptr<Model3D>& model() const { return this->m_model; }
        const Material& material() const { return this->m_material; }
        Object& material(Material material) {
            this->m_material = std::move(material);
            return *this;
        }

        glm::vec3& pos() { return this->m_pos; }
        const glm::vec3& pos() const { return this->m_pos; }
        Orientation& orientation() { return this->m_orientation; }
        const Orientation& orientation() const { return this->m_orientation; }
        float scale() const { return this->m_scale; }
        Object& scale(float scale) {
            this->m_scale = scale;
            return *this;
        }

        AABB bounding_box() const;

        void draw(
            ShaderProgram& program,
            const RenderSettings& render_settings,
            const glm::mat4& view_projection_matrix
        ) const;
    };

    struct PointLight {
        glm::vec3 pos;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        float a0;
        float a1;
        float a2;
    };

    template <>
    struct ShaderProgram::uniform_setter<PointLight> {
        void operator ()(ShaderProgram& program, std::string name, const PointLight& value);
    };

    class Camera {
        glm::mat4 m_view_matrix;
        glm::mat4 m_projection_matrix;
    public:
        Camera() {}
        Camera(const glm::mat4& view_matrix) : m_view_matrix(view_matrix) {}

        const glm::mat4& view_matrix() const { return this->m_view_matrix; }

        const glm::mat4& projection_matrix() const { return this->m_projection_matrix; }
        Camera& projection_matrix(const glm::mat4& projection_matrix) {
            this->m_projection_matrix = projection_matrix;
            return *this;
        }

        glm::mat4 view_projection_matrix() const {
            return this->m_projection_matrix * this->m_view_matrix;
        }

        glm::mat3 orientation_matrix() const { return glm::mat3(this->m_view_matrix); }
        Camera& orientation_matrix(const glm::mat3& orientation_matrix) {
            this->m_view_matrix[0] = glm::vec4(orientation_matrix[0], this->m_view_matrix[0].w);
            this->m_view_matrix[1] = glm::vec4(orientation_matrix[1], this->m_view_matrix[1].w);
            this->m_view_matrix[2] = glm::vec4(orientation_matrix[2], this->m_view_matrix[2].w);

            return *this;
        }

        Camera& look_at(glm::vec3 pos, glm::vec3 up) {
            this->m_view_matrix = glm::lookAt(this->pos(), pos, up);
            return *this;
        }

        glm::vec3 pos() const { return glm::vec3(glm::inverse(this->m_view_matrix)[3]); }
        Camera& pos(glm::vec3 pos) {
            pos = this->orientation_matrix() * -pos;
            this->m_view_matrix[3] = glm::vec4(pos, this->m_view_matrix[3].w);
            return *this;
        }
    };

    class OrbitControls {
        enum class OrbitState {
            NONE,
            ROTATING,
            PANNING
        };

        Camera* m_camera = nullptr;
        glm::vec3 m_rotate_origin;

        glm::vec2 m_last_pos;
        OrbitState m_state = OrbitState::NONE;
    public:
        OrbitControls() {}
        OrbitControls(Camera* camera) : m_camera(camera) {}

        glm::vec3 rotate_origin() const { return this->m_rotate_origin; }
        OrbitControls& rotate_origin(glm::vec3 rotate_origin) {
            this->m_rotate_origin = rotate_origin;
            return *this;
        }

        void begin_rotate(glm::vec2 pos);
        void begin_pan(glm::vec2 pos);

        void move_cursor(glm::vec2 pos);
        void handle_zoom(float delta);

        void end_rotate();
        void end_pan();
    };

    class World {
        std::vector<std::unique_ptr<Object>> m_objects;
        std::vector<std::unique_ptr<PointLight>> m_point_lights;

        RenderSettings m_render_settings;

        Camera m_camera;

        ShaderProgram& select_program() const;
    public:
        World() {};

        std::vector<std::unique_ptr<Object>>& objects() { return this->m_objects; }
        const std::vector<std::unique_ptr<Object>>& objects() const { return this->m_objects; }

        std::vector<std::unique_ptr<PointLight>>& point_lights() { return this->m_point_lights; }
        const std::vector<std::unique_ptr<PointLight>>& point_lights() const {
            return this->m_point_lights;
        }

        RenderSettings& render_settings() { return this->m_render_settings; }
        const RenderSettings& render_settings() const { return this->m_render_settings; }

        Camera& camera() { return this->m_camera; }
        const Camera& camera() const { return this->m_camera; }

        AABB bounding_box() const;

        World& load_scene(boost::filesystem::path path);

        void draw() const;
    };
}

#endif
