#include <sstream>
#include <stdexcept>
#include <string>

#include "opengl.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace hw3 {
    void Shader::compile(const std::string& impl) {
        GLint status;
        const char* buf_array[] = { impl.c_str() };

        glShaderSource(this->m_id, 1, buf_array, 0);
        glCompileShader(this->m_id);
        glGetShaderiv(this->m_id, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE) {
            GLsizei length;

            glGetShaderiv(this->m_id, GL_INFO_LOG_LENGTH, &length);

            std::string info(length, ' ');
            std::ostringstream ss;

            glGetShaderInfoLog(this->m_id, length, &length, &info[0]);

            ss << "Error compiling shader: " << info;

            throw std::runtime_error(ss.str());
        }
    }

    Shader::Shader(Shader&& other) : m_id(other.m_id) {
        other.m_id = 0;
    }

    Shader::Shader(const std::string& impl, ShaderType type)
        : m_id(glCreateShader((GLenum)type)) {
        if (!this->m_id) {
            throw std::runtime_error("Failed to create shader");
        }

        this->compile(impl);
    }

    Shader::~Shader() {
        if (this->m_id) {
            glDeleteShader(this->m_id);
        }
    }

    Shader& Shader::operator =(Shader&& other) {
        if (this->m_id) {
            glDeleteShader(this->m_id);
        }

        this->m_id = other.m_id;
        other.m_id = 0;

        return *this;
    }

    void ShaderProgram::link() {
        GLint status;

        for (auto s : this->m_shaders) {
            glAttachShader(this->m_id, s->id());
        }

        glLinkProgram(this->m_id);
        glGetProgramiv(this->m_id, GL_LINK_STATUS, &status);

        if (status == GL_FALSE) {
            GLsizei length;

            glGetProgramiv(this->m_id, GL_INFO_LOG_LENGTH, &length);

            std::string info(length, ' ');
            std::ostringstream ss;

            glGetProgramInfoLog(this->m_id, length, &length, &info[0]);

            ss << "Error linking program: " << info;

            throw std::runtime_error(ss.str());
        }
    }

    ShaderProgram::ShaderProgram(ShaderProgram&& other)
        : m_id(other.m_id), m_shaders(std::move(other.m_shaders)),
          m_next_texture(other.m_next_texture),
          m_texture_bindings(std::move(other.m_texture_bindings)),
          m_patch_size(other.m_patch_size) {
        other.m_id = 0;
        other.m_next_texture = 1;
        other.m_patch_size = 0;
    }

    ShaderProgram::ShaderProgram(std::initializer_list<std::shared_ptr<Shader>> shaders)
        : m_id(glCreateProgram()), m_shaders(std::move(shaders)), m_next_texture(1),
          m_patch_size(0) {
        if (!this->m_id) {
            throw std::runtime_error("Failed to create program");
        }

        this->link();
    }

    ShaderProgram::ShaderProgram(std::vector<std::shared_ptr<Shader>>&& shaders)
        : m_id(glCreateProgram()), m_shaders(std::move(shaders)), m_next_texture(1),
          m_patch_size(0) {
        if (!this->m_id) {
            throw std::runtime_error("Failed to create program");
        }

        this->link();
    }

    ShaderProgram::~ShaderProgram() {
        if (this->m_id) {
            glDeleteProgram(this->m_id);
        }
    }

    ShaderProgram& ShaderProgram::operator =(ShaderProgram&& other) {
        if (this->m_id) {
            glDeleteProgram(this->m_id);
        }

        this->m_id = other.m_id;
        this->m_shaders = std::move(other.m_shaders);
        this->m_next_texture = other.m_next_texture;
        this->m_texture_bindings = std::move(other.m_texture_bindings);
        this->m_patch_size = other.m_patch_size;

        other.m_id = 0;
        other.m_next_texture = 1;
        other.m_patch_size = 0;

        return *this;
    }

    void ShaderProgram::set_uniform(std::string name, float value) {
        GLint loc = glGetUniformLocation(this->m_id, name.c_str());

        if (loc != -1) {
           glProgramUniform1f(this->m_id, loc, value);
        }

        handle_errors();
    }

    void ShaderProgram::set_uniform(std::string name, glm::vec2 value) {
        GLint loc = glGetUniformLocation(this->m_id, name.c_str());

        if (loc != -1) {
           glProgramUniform2f(this->m_id, loc, value.x, value.y);
        }

        handle_errors();
    }

    void ShaderProgram::set_uniform(std::string name, glm::vec3 value) {
        GLint loc = glGetUniformLocation(this->m_id, name.c_str());

        if (loc != -1) {
           glProgramUniform3f(this->m_id, loc, value.x, value.y, value.z);
        }

        handle_errors();
    }

    void ShaderProgram::set_uniform(std::string name, glm::vec4 value) {
        GLint loc = glGetUniformLocation(this->m_id, name.c_str());

        if (loc != -1) {
           glProgramUniform4f(this->m_id, loc, value.x, value.y, value.z, value.w);
        }

        handle_errors();
    }

    void ShaderProgram::set_uniform(std::string name, const glm::mat4& value) {
        GLint loc = glGetUniformLocation(this->m_id, name.c_str());

        if (loc != -1) {
           glProgramUniformMatrix4fv(this->m_id, loc, 1, GL_TRUE, glm::value_ptr(value));
        }

        handle_errors();
    }

    void ShaderProgram::set_uniform(std::string name, const Sampler2D* value) {
        GLint loc = glGetUniformLocation(this->m_id, name.c_str());

        if (loc != -1) {
            auto it = this->m_texture_bindings.find(loc);

            if (it == this->m_texture_bindings.end()) {
                GLuint unit = this->m_next_texture++;

                this->m_texture_bindings.emplace(loc, TextureBinding {
                    .unit = unit,
                    .sampler = value
                });

                glProgramUniform1i(this->m_id, loc, unit);
            } else {
                it->second.sampler = value;
            }
        }

        handle_errors();
    }

    void ShaderProgram::use() const {
        glUseProgram(this->m_id);
        handle_errors();

        for (const auto& tex_binding : this->m_texture_bindings) {
            if (tex_binding.second.sampler != nullptr)
                tex_binding.second.sampler->bind(tex_binding.second.unit);
        }

        if (this->m_patch_size != 0) {
            glPatchParameteri(GL_PATCH_VERTICES, this->m_patch_size);
            handle_errors();
        }
    }
}
