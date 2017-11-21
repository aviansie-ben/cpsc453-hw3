#include <stb_image.h>
#include <stdexcept>
#include <vector>

#include "opengl.hpp"
#include "texture.hpp"

namespace hw3 {
    static std::shared_ptr<Texture2D> single_pixel_texture;

    Texture2D::Texture2D(Texture2D&& other)
        : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height) {
        other.m_id = 0;
    }

    Texture2D::Texture2D(TextureDataFormat format, int width, int height) : m_id(0), m_width(width), m_height(height) {
        this->load_data(nullptr, format, width, height);
    }

    Texture2D::~Texture2D() {
        if (this->m_id != 0) {
            glDeleteTextures(1, &this->m_id);
            clear_errors();
        }
    }

    Texture2D& Texture2D::operator =(Texture2D&& other) {
        if (this->m_id != 0) {
            glDeleteTextures(1, &this->m_id);
            clear_errors();
        }

        this->m_id = other.m_id;
        this->m_width = other.m_width;
        this->m_height = other.m_height;

        other.m_id = 0;

        return *this;
    }

    Texture2D& Texture2D::load_data(const char* data, TextureDataFormat format, int width, int height) {
        if (this->m_id == 0) {
            glGenTextures(1, &this->m_id);

            if (this->m_id == 0) {
                clear_errors();
                throw std::runtime_error("Failed to allocate Texture2D");
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->m_id);
        handle_errors();

        if (data == nullptr) {
            std::vector<glm::vec4> clear_data(width * height, glm::vec4(0));

            glTexImage2D(GL_TEXTURE_2D, 0, (GLenum)format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, clear_data.data());
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, (GLenum)format, width, height, 0, (GLenum)format, GL_UNSIGNED_BYTE, data);
        }

        handle_errors();

        this->m_width = width;
        this->m_height = height;

        return *this;
    }

    Texture2D& Texture2D::load_subimage_data(const char* data, TextureDataFormat format, int x, int y, int width, int height) {
        assert(this->m_id != 0);
        assert((x + width) <= this->m_width);
        assert((y + height) <= this->m_height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->m_id);
        handle_errors();

        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, (GLenum)format, GL_UNSIGNED_BYTE, data);
        handle_errors();

        return *this;
    }

    Texture2D& Texture2D::generate_mipmap() {
        assert(this->m_id != 0);

        glGenerateMipmap(GL_TEXTURE_2D);
        handle_errors();

        return *this;
    }

    Texture2D Texture2D::load_from_file(std::string path) {
        int width, height, channels;
        char* data = reinterpret_cast<char*>(stbi_load(path.c_str(), &width, &height, &channels, 4));

        if (data == nullptr) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;
                ss << "Failed to read texture from file " << path;

                return ss.str();
            })());
        }

        Texture2D tex = std::move(
            Texture2D()
                .load_data(data, TextureDataFormat::RGBA, width, height)
                .generate_mipmap()
        );

        stbi_image_free(data);

        return std::move(tex);
    }

    std::shared_ptr<Texture2D> Texture2D::single_pixel() {
        if (!single_pixel_texture) {
            char data[4] = {
                static_cast<char>(0xff),
                static_cast<char>(0xff),
                static_cast<char>(0xff),
                static_cast<char>(0xff)
            };

            single_pixel_texture = std::make_shared<Texture2D>();
            single_pixel_texture->load_data(
                data,
                TextureDataFormat::RGBA,
                1,
                1
            );
        }

        return single_pixel_texture;
    }

    Sampler2D::Sampler2D(Sampler2D&& other)
        : m_id(other.m_id), m_texture(std::move(other.m_texture)) {
        other.m_id = 0;
    }

    Sampler2D::Sampler2D(std::shared_ptr<Texture2D> texture) : m_id(0) {
        this->bind_texture(texture);
    }

    Sampler2D::~Sampler2D() {
        if (this->m_id != 0) {
            glDeleteTextures(1, &this->m_id);
            clear_errors();
        }
    }

    Sampler2D& Sampler2D::operator =(Sampler2D&& other) {
        if (this->m_id != 0) {
            glDeleteTextures(1, &this->m_id);
            clear_errors();
        }

        this->m_id = other.m_id;
        this->m_texture = std::move(other.m_texture);

        other.m_id = 0;

        return *this;
    }

    Sampler2D& Sampler2D::bind_texture(std::shared_ptr<Texture2D> texture) {
        if (this->m_id == 0) {
            glGenSamplers(1, &this->m_id);

            if (this->m_id == 0) {
                clear_errors();
                throw std::runtime_error("Failed to allocate Sampler2D");
            }
        }

        this->m_texture = std::move(texture);

        return *this;
    }

    Sampler2D& Sampler2D::set_sample_mode(TextureSampleMode upsample, TextureSampleMode downsample) {
        assert(this->m_id != 0);

        glSamplerParameteri(this->m_id, GL_TEXTURE_MAG_FILTER, (GLint)upsample);
        glSamplerParameteri(this->m_id, GL_TEXTURE_MIN_FILTER, (GLint)downsample);
        handle_errors();

        return *this;
    }

    Sampler2D& Sampler2D::set_wrap_mode(TextureWrapMode wrap_x, TextureWrapMode wrap_y) {
        assert(this->m_id != 0);

        glSamplerParameteri(this->m_id, GL_TEXTURE_WRAP_S, (GLint)wrap_x);
        glSamplerParameteri(this->m_id, GL_TEXTURE_WRAP_T, (GLint)wrap_y);
        handle_errors();

        return *this;
    }

    void Sampler2D::bind(GLuint unit) const {
        assert(this->m_id != 0);

        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, this->m_texture->id());
        glBindSampler(unit, this->m_id);
        handle_errors();
    }
}
