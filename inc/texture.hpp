#ifndef HW3_TEXTURE_HPP
#define HW3_TEXTURE_HPP

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace hw3 {
    enum class TextureSampleMode : GLint {
        NEAREST = GL_NEAREST,
        LINEAR = GL_LINEAR,
        NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
        LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
    };

    enum class TextureWrapMode : GLint {
        CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
        CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
        MIRROR_CLAMP_TO_EDGE = GL_MIRROR_CLAMP_TO_EDGE,
        REPEAT = GL_REPEAT,
        MIRROR_REPEAT = GL_MIRRORED_REPEAT
    };

    enum class TextureDataFormat : GLenum {
        GRAYSCALE = GL_RED,
        RGBA = GL_RGBA
    };

    class Texture2D {
        GLuint m_id;

        int m_width;
        int m_height;
    public:
        Texture2D() : m_id(0) {}
        Texture2D(const Texture2D& other) = delete;
        Texture2D(Texture2D&& other);
        Texture2D(TextureDataFormat format, int width, int height);
        ~Texture2D();

        Texture2D& operator =(const Texture2D& other) = delete;
        Texture2D& operator =(Texture2D&& other);

        Texture2D& load_data(const char* data, TextureDataFormat format, int width, int height);
        Texture2D& load_subimage_data(const char* data, TextureDataFormat format, int x, int y, int width, int height);
        Texture2D& generate_mipmap();

        glm::ivec2 size() const { return glm::ivec2(this->m_width, this->m_height); }
        float aspect_ratio() const { return (float)this->m_width / (float)this->m_height; }

        operator bool() const { return this->m_id != 0; }
        GLuint id() const { return this->m_id; }

        static Texture2D load_from_file(std::string path);
        static std::shared_ptr<Texture2D> single_pixel();
    };

    class Sampler2D {
        GLuint m_id;
        std::shared_ptr<Texture2D> m_texture;
    public:
        Sampler2D() : m_id(0) {}
        Sampler2D(const Sampler2D& other) = delete;
        Sampler2D(Sampler2D&& other);
        Sampler2D(std::shared_ptr<Texture2D> texture);
        ~Sampler2D();

        Sampler2D& operator =(const Sampler2D& other) = delete;
        Sampler2D& operator =(Sampler2D&& other);

        Sampler2D& bind_texture(std::shared_ptr<Texture2D> texture);
        Sampler2D& set_sample_mode(TextureSampleMode upsample, TextureSampleMode downsample);
        Sampler2D& set_wrap_mode(TextureWrapMode wrap_x, TextureWrapMode wrap_y);

        void bind(GLuint unit) const;

        std::shared_ptr<Texture2D> texture() const { return this->m_texture; }

        operator bool() const { return this->m_id != 0; }
        GLuint id() const { return this->m_id; }
    };
}

#endif
