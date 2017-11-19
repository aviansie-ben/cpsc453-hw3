#ifndef HW3_VERTEX_HPP
#define HW3_VERTEX_HPP

#include <array>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

namespace hw3 {
    constexpr double tau = 6.28318530717958647692;

    enum class PrimitiveType {
        POINTS = GL_POINTS,
        LINE_STRIP = GL_LINE_STRIP,
        LINE_STRIP_ADJACENCY = GL_LINE_STRIP_ADJACENCY,
        LINE_LOOP = GL_LINE_LOOP,
        TRIANGLES = GL_TRIANGLES,
        TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        PATCHES = GL_PATCHES
    };

    enum class DataType {
        FLOAT = GL_FLOAT
    };

    class GlBuffer {
        GLuint m_id;
        std::size_t m_size;
    public:
        GlBuffer() : m_id(0), m_size(0) {};
        GlBuffer(const GlBuffer& other) = delete;
        GlBuffer(GlBuffer&& other);
        ~GlBuffer();

        GlBuffer& operator =(const GlBuffer& other) = delete;
        GlBuffer& operator =(GlBuffer&& other);

        GLuint id() const { return this->m_id; }
        std::size_t size() const { return this->m_size; }

        void load_data(const void* data, std::size_t length, GLenum usage);

        template<class T>
        void load_data(std::vector<T> data, GLenum usage) {
            this->load_data(data.data(), data.size() * sizeof(T), usage);
        }

        template<class T>
        void load_data(std::initializer_list<T> data, GLenum usage) {
            this->load_data(data.begin(), data.size() * sizeof(T), usage);
        }
    };

    class GlVertexArray {
        GLuint m_id;
        int m_size;

        std::vector<GlBuffer> m_buffers;
    public:
        GlVertexArray() : m_id(0) {}
        GlVertexArray(const GlVertexArray& other) = delete;
        GlVertexArray(GlVertexArray&& other);
        GlVertexArray(int num_buffers, int size);

        ~GlVertexArray();

        GlVertexArray& operator =(const GlVertexArray& other) = delete;
        GlVertexArray& operator =(GlVertexArray&& other);

        operator bool() const { return this->m_id != 0; }
        GLuint id() const { return this->m_id; }

        GlBuffer& buffer(int i) { return this->m_buffers[i]; }
        const GlBuffer& buffer(int i) const { return this->m_buffers[i]; }

        int size() const { return this->m_size; }
        void size(int size) { this->m_size = size; }

        void bind_attribute(
            int attribute_index,
            size_t num_components,
            DataType data_type,
            size_t stride,
            size_t start,
            int buffer_index
        );

        void draw(PrimitiveType type) const;
        void draw(int first, size_t n, PrimitiveType type) const;
        void draw_indexed(int buffer_index, int first, size_t n, PrimitiveType type) const;
    };
}

#endif
