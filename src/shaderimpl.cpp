#include "shaderimpl.hpp"

namespace hw3 {
    namespace shaders {
        ShaderProgram fixed_program;
        ShaderProgram font_program;
        ShaderProgram normal_program;
        ShaderProgram phong_program;
        ShaderProgram point_program;
        ShaderProgram textured_program;

        void init() {
            auto vertex_simple = std::make_shared<Shader>(impl::vertex_simple, ShaderType::VERTEX);
            auto vertex_textured = std::make_shared<Shader>(impl::vertex_textured, ShaderType::VERTEX);
            auto vertex_textured_normal = std::make_shared<Shader>(impl::vertex_textured_normal, ShaderType::VERTEX);
            auto geometry_point = std::make_shared<Shader>(impl::geometry_point, ShaderType::GEOMETRY);
            auto fragment_fixed = std::make_shared<Shader>(impl::fragment_fixed, ShaderType::FRAGMENT);
            auto fragment_font = std::make_shared<Shader>(impl::fragment_font, ShaderType::FRAGMENT);
            auto fragment_normal = std::make_shared<Shader>(impl::fragment_normal, ShaderType::FRAGMENT);
            auto fragment_phong = std::make_shared<Shader>(impl::fragment_phong, ShaderType::FRAGMENT);
            auto fragment_textured = std::make_shared<Shader>(impl::fragment_textured, ShaderType::FRAGMENT);

            fixed_program = ShaderProgram({ vertex_simple, fragment_fixed });
            font_program = ShaderProgram({ vertex_textured, fragment_font });
            normal_program = ShaderProgram({ vertex_textured_normal, fragment_normal });
            phong_program = ShaderProgram({ vertex_textured_normal, fragment_phong });
            point_program = ShaderProgram({ vertex_simple, geometry_point, fragment_fixed });
            textured_program = ShaderProgram({ vertex_textured, fragment_textured });
        }
    }
}
