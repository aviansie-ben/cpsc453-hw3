#ifndef HW3_FONT_HPP
#define HW3_FONT_HPP

#include <string>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "texture.hpp"
#include "vertex.hpp"

namespace hw3 {
    class Font;
    class Text {
        GlVertexArray m_vertex_array;
        const Font* m_font;

        glm::vec2 m_size;
    public:
        Text() : m_font(nullptr) {}
        Text(GlVertexArray&& vertex_array, const Font* font, glm::vec2 size);

        glm::vec2 size() const { return this->m_size; }

        void draw(const glm::mat4& transform, glm::vec3 color) const;

        operator bool() const { return this->m_font != nullptr; }
    };

    struct FontGlyph {
        glm::vec2 advance;
        glm::vec2 bitmap_size;
        glm::vec2 bitmap_offset;

        float texture_x;
    };

    class Font {
        Sampler2D m_sampler;
        std::vector<FontGlyph> m_glyphs;

        float m_line_first;
        float m_line_height;
    public:
        Font() {}
        Font(Sampler2D&& sampler, std::vector<FontGlyph>&& glyphs, float line_first, float line_height)
            : m_sampler(std::move(sampler)), m_glyphs(std::move(glyphs)), m_line_first(line_first),
              m_line_height(line_height) {}

        Sampler2D& sampler() { return this->m_sampler; }
        const Sampler2D& sampler() const { return this->m_sampler; }

        float line_first() const { return this->m_line_first; }
        float line_height() const { return this->m_line_height; }

        const FontGlyph& glyph(int c) const;

        Text create_text(std::string text) const;

        operator bool() const { return this->m_sampler; }

        static Font load_from_file(std::string path, int size);
        static Font load_font(std::string name, int size);
    };

    void init_fonts();
}

#endif
