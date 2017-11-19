#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "opengl.hpp"
#include "vertex.hpp"

namespace hw3 {
    GlBuffer::GlBuffer(GlBuffer&& other) : m_id(other.m_id), m_size(other.m_size) {
        other.m_id = 0;
        other.m_size = 0;
    }

    GlBuffer::~GlBuffer() {
        if (this->m_id) {
            glDeleteBuffers(1, &this->m_id);
        }
    }

    GlBuffer& GlBuffer::operator =(GlBuffer&& other) {
        if (this->m_id) {
            glDeleteBuffers(1, &this->m_id);
        }

        this->m_id = other.m_id;
        this->m_size = other.m_size;

        other.m_id = 0;
        other.m_size = 0;

        return *this;
    }

    void GlBuffer::load_data(const void* data, std::size_t length, GLenum usage) {
        if (!this->m_id) {
            glGenBuffers(1, &this->m_id);

            if (!this->m_id) {
                throw std::runtime_error("Failed to allocate OpenGL buffer");
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
        glBufferData(GL_ARRAY_BUFFER, length, data, usage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        handle_errors();

        this->m_size = length;
    }

    GlVertexArray::GlVertexArray(GlVertexArray&& other): m_id(other.m_id), m_size(other.m_size), m_buffers(std::move(other.m_buffers)) {
        other.m_id = 0;
        other.m_size = 0;
    }

    GlVertexArray::GlVertexArray(int num_buffers, int size) : m_size(size), m_buffers(num_buffers) {
        glGenVertexArrays(1, &this->m_id);

        if (!this->m_id) {
            throw std::runtime_error("Failed to allocate vertex array");
        }
    }

    GlVertexArray::~GlVertexArray() {
        if (this->m_id) {
            glDeleteVertexArrays(1, &this->m_id);
        }
    }

    GlVertexArray& GlVertexArray::operator =(GlVertexArray&& other) {
        if (this->m_id) {
            glDeleteVertexArrays(1, &this->m_id);
        }

        this->m_id = other.m_id;
        this->m_size = other.m_size;
        this->m_buffers = std::move(other.m_buffers);

        other.m_id = 0;
        other.m_size = 0;

        return *this;
    }

    void GlVertexArray::bind_attribute(
        int attribute_index,
        size_t num_components,
        DataType data_type,
        size_t stride,
        size_t start,
        int buffer_index
    ) {
        glBindVertexArray(this->m_id);
        glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[buffer_index].id());

        switch (data_type) {
        case DataType::FLOAT:
            glVertexAttribPointer(
                attribute_index,
                num_components,
                (GLenum)data_type,
                GL_FALSE,
                stride,
                (void*)start
            );
        }

        glEnableVertexAttribArray(attribute_index);
        handle_errors();
    }

    void GlVertexArray::draw(PrimitiveType type) const {
        this->draw(0, this->size(), type);
    }

    void GlVertexArray::draw(int first, size_t n, PrimitiveType type) const {
        glBindVertexArray(this->m_id);
        glDrawArrays((GLenum)type, first, n);
        handle_errors();
    }

    void GlVertexArray::draw_indexed(int buffer_index, int first, size_t n, PrimitiveType type) const {
        glBindVertexArray(this->m_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->buffer(buffer_index).id());
        glDrawElements((GLenum)type, n, GL_UNSIGNED_SHORT, (void*)(intptr_t)first);
        handle_errors();
    }
}
