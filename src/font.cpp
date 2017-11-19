#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "font.hpp"
#include "opengl.hpp"
#include "shaderimpl.hpp"

namespace hw3 {
    static FT_Library ft_lib;
    static FcConfig* fc_config;

    Text::Text(GlVertexArray&& vertex_array, const Font* font, glm::vec2 size)
        : m_vertex_array(std::move(vertex_array)), m_font(font), m_size(size) {
    }

    void Text::draw(const glm::mat4& transform, glm::vec3 color) const {
        shaders::font_program.set_uniform("color", color);
        shaders::font_program.set_uniform("tex", &this->m_font->sampler());
        shaders::font_program.set_uniform("vertex_transform", transform);
        shaders::font_program.use();

        this->m_vertex_array.draw_indexed(1, 0, this->m_vertex_array.size(), PrimitiveType::TRIANGLES);
    }

    const FontGlyph& Font::glyph(int c) const {
        if (c >= 0x20 && c <= 0x7f) {
            return this->m_glyphs[c - 0x20];
        } else {
            return this->m_glyphs['?' - 0x20];
        }
    }

    Text Font::create_text(std::string text) const {
        struct VertData {
            glm::vec2 pos;
            glm::vec2 texcoord;
        };

        std::vector<VertData> vertices;
        std::vector<unsigned short> indices;

        vertices.reserve(text.length() * 4);
        indices.reserve(text.length() * 6);

        glm::vec2 tex_size = this->m_sampler.texture()->size();
        float x = 0.0;
        float y = this->m_line_first;
        unsigned short i = 0;

        float max_x = 0.0;

        for (char c : text) {
            if (c == '\n') {
                x = 0.0;
                y += this->m_line_height;
                continue;
            }

            const FontGlyph& glyph = this->glyph(c);

            float xo = x + glyph.bitmap_offset.x;
            float yo = y - glyph.bitmap_offset.y;
            float w = glyph.bitmap_size.x;
            float h = glyph.bitmap_size.y;

            x += glyph.advance.x;
            y += glyph.advance.y;

            if (w == 0.0 || h == 0.0) {
                continue;
            }

            max_x = std::max(max_x, xo + w);

            vertices.push_back({
                glm::vec2(xo, yo),
                glm::vec2(glyph.texture_x, 0)
            });
            vertices.push_back({
                glm::vec2(xo + w, yo),
                glm::vec2(glyph.texture_x + w / tex_size.x, 0)
            });
            vertices.push_back({
                glm::vec2(xo + w, yo + h),
                glm::vec2(glyph.texture_x + w / tex_size.x, h / tex_size.y)
            });
            vertices.push_back({
                glm::vec2(xo, yo + h),
                glm::vec2(glyph.texture_x, h / tex_size.y)
            });

            indices.insert(indices.end(), {
                i,
                static_cast<unsigned short>(i + 1),
                static_cast<unsigned short>(i + 3),
                static_cast<unsigned short>(i + 1),
                static_cast<unsigned short>(i + 3),
                static_cast<unsigned short>(i + 2)
            });

            i += 4;
        }

        GlVertexArray va(2, (i / 4) * 6);

        va.buffer(0).load_data(vertices, GL_STATIC_DRAW);
        va.buffer(1).load_data(indices, GL_STATIC_DRAW);
        va.bind_attribute(0, 2, DataType::FLOAT, sizeof(float) * 4, 0, 0);
        va.bind_attribute(1, 2, DataType::FLOAT, sizeof(float) * 4, sizeof(float) * 2, 0);

        return Text(std::move(va), this, glm::vec2(max_x, y + this->m_line_height - this->m_line_first));
    }

    Font Font::load_from_file(std::string path, int size) {
        FT_Face face;
        FT_Error error;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if ((error = FT_New_Face(ft_lib, path.c_str(), 0, &face)) != 0) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Error " << error << " while loading font " << path;

                return ss.str();
            })());
        }

        FT_Set_Pixel_Sizes(face, 0, size);

        FT_GlyphSlot g = face->glyph;
        unsigned int width = 0;
        unsigned int height = 0;

        for (int c = 0x20; c <= 0x7f; c++) {
            if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER)) != 0) {
                throw std::runtime_error(([&]() {
                    std::ostringstream ss;

                    ss << "Error " << error << " while loading character 0x" << std::hex << c << " from font " << path;

                    return ss.str();
                })());
            }

            width += g->bitmap.width + 1;
            height = std::max(height, g->bitmap.rows);
        }

        std::shared_ptr<Texture2D> tex = std::make_shared<Texture2D>(TextureDataFormat::GRAYSCALE, width, height);
        std::vector<FontGlyph> glyphs;
        unsigned int x = 0;

        glyphs.reserve(0x80 - 0x20);

        for (int c = 0x20; c <= 0x7f; c++) {
            if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER)) != 0) {
                throw std::runtime_error(([&]() {
                    std::ostringstream ss;

                    ss << "Error " << error << " while loading font " << path;

                    return ss.str();
                })());
            }

            tex->load_subimage_data(
                (char*)g->bitmap.buffer,
                TextureDataFormat::GRAYSCALE,
                x,
                0,
                g->bitmap.width,
                g->bitmap.rows
            );

            glyphs.push_back(FontGlyph {
                .advance = glm::vec2(g->advance.x / 64.0, g->advance.y / 64.0),
                .bitmap_size = glm::vec2(g->bitmap.width, g->bitmap.rows),
                .bitmap_offset = glm::vec2(g->bitmap_left, g->bitmap_top),
                .texture_x = (float)x / width
            });

            x += g->bitmap.width + 1;
        }

        tex->generate_mipmap();

        float line_first = face->size->metrics.ascender / 64.0;
        float line_height = face->size->metrics.height / 64.0;

        FT_Done_Face(face);

        return Font(
            std::move(Sampler2D(tex)
                .set_sample_mode(TextureSampleMode::LINEAR, TextureSampleMode::LINEAR)
                .set_wrap_mode(TextureWrapMode::CLAMP_TO_BORDER, TextureWrapMode::CLAMP_TO_BORDER)),
            std::move(glyphs),
            line_first,
            line_height
        );
    }

    Font Font::load_font(std::string name, int size) {
        FcResult result;
        FcChar8* file_c;
        std::string file;
        FcPattern* pat = FcNameParse(
            const_cast<FcChar8*>(reinterpret_cast<const FcChar8*>(name.c_str()))
        );

        FcConfigSubstitute(fc_config, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        FcPattern* font = FcFontMatch(fc_config, pat, &result);

        if (!font) {
            FcPatternDestroy(pat);
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Failed to find font " << name;

                return ss.str();
            })());
        }

        if ((result = FcPatternGetString(font, FC_FILE, 0, &file_c)) != FcResultMatch) {
            FcPatternDestroy(pat);
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Fontconfig error " << result << " while loading font " << name;

                return ss.str();
            })());
        }

        file = reinterpret_cast<const char*>(file_c);
        FcPatternDestroy(pat);

        return Font::load_from_file(file, size);
    }

    void init_fonts() {
        FT_Error error;

        if ((error = FT_Init_FreeType(&ft_lib)) != 0) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Error " << error << " while loading FreeType";

                return ss.str();
            })());
        }

        fc_config = FcInitLoadConfigAndFonts();
    }
}
