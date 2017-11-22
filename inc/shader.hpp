#ifndef HW3_SHADER_HPP
#define HW3_SHADER_HPP

#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace hw3 {
    enum class ShaderType : GLenum {
        COMPUTE = GL_COMPUTE_SHADER,
        VERTEX = GL_VERTEX_SHADER,
        TESS_CONTROL = GL_TESS_CONTROL_SHADER,
        TESS_EVALUATION = GL_TESS_EVALUATION_SHADER,
        GEOMETRY = GL_GEOMETRY_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER
    };

    class Shader {
        GLuint m_id;

        void compile(const std::string& impl);
    public:
        Shader() : m_id(0) {}
        Shader(const Shader& other) = delete;
        Shader(Shader&& other);
        Shader(const std::string& impl, ShaderType type);

        ~Shader();

        Shader& operator =(const Shader& other) = delete;
        Shader& operator =(Shader&& other);

        operator bool() const { return this->m_id != 0; }
        GLuint id() const { return this->m_id; }
    };

    class Sampler2D;
    class ShaderProgram {
        struct TextureBinding {
            GLuint unit;
            const Sampler2D* sampler;
        };

        GLuint m_id;
        std::vector<std::shared_ptr<Shader>> m_shaders;

        GLuint m_next_texture;
        std::map<GLint, TextureBinding> m_texture_bindings;

        int m_patch_size;

        void link();
    public:
        ShaderProgram() : m_id(0), m_patch_size(0) {}
        ShaderProgram(const ShaderProgram& other) = delete;
        ShaderProgram(ShaderProgram&& other);
        ShaderProgram(std::initializer_list<std::shared_ptr<Shader>> shaders);
        ShaderProgram(std::vector<std::shared_ptr<Shader>>&& shaders);

        ~ShaderProgram();

        ShaderProgram& operator =(const ShaderProgram& other) = delete;
        ShaderProgram& operator =(ShaderProgram&& other);

        void set_uniform(std::string name, float value);
        void set_uniform(std::string name, int value);
        void set_uniform(std::string name, glm::vec2 value);
        void set_uniform(std::string name, glm::vec3 value);
        void set_uniform(std::string name, glm::vec4 value);
        void set_uniform(std::string name, const glm::mat3& value);
        void set_uniform(std::string name, const glm::mat4& value);
        void set_uniform(std::string name, Sampler2D* value) { this->set_uniform(name, static_cast<const Sampler2D*>(value)); }
        void set_uniform(std::string name, const Sampler2D* value);

        template <typename T>
        struct uniform_setter {
            void operator()(ShaderProgram& program, std::string name, const T& value) const {
                static_assert(sizeof(T) == 0, "Unsupported type for uniform_setter");
            }
        };

        template <typename T>
        void set_uniform(std::string name, const T& value) {
            uniform_setter<T>()(*this, name, value);
        }

        int patch_size() const { return this->m_patch_size; }
        void patch_size(int size) { this->m_patch_size = size; }

        void use() const;

        operator bool() const { return this->m_id != 0; }

        GLuint id() const { return this->m_id; }
    };
}

#endif
