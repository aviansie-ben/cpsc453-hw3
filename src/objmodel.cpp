#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "objmodel.hpp"
#include "shaderimpl.hpp"

namespace hw3 {
    static GlVertexArray box_va;

    const GlVertexArray& AABB::box_geometry() {
        if (!box_va) {
            box_va = GlVertexArray(2, 8);

            box_va.buffer(0).load_data<glm::vec3>({
                glm::vec3(0, 0, 0),
                glm::vec3(0, 0, 1),
                glm::vec3(0, 1, 0),
                glm::vec3(0, 1, 1),
                glm::vec3(1, 0, 0),
                glm::vec3(1, 0, 1),
                glm::vec3(1, 1, 0),
                glm::vec3(1, 1, 1)
            }, GL_STATIC_DRAW);

            box_va.buffer(1).load_data<unsigned int>({
                0, 1,
                1, 3,
                3, 2,
                2, 0,
                4, 5,
                5, 7,
                7, 6,
                6, 4,
                0, 4,
                1, 5,
                2, 6,
                3, 7
            }, GL_STATIC_DRAW);

            box_va.bind_attribute(0, 3, DataType::FLOAT, 0, 0, 0);
        }

        return box_va;
    }

    void AABB::draw(const glm::mat4& transform, glm::vec4 colour) const {
        shaders::fixed_program.set_uniform(
            "vertex_transform",
            transform * glm::scale(glm::translate(glm::mat4(1), this->m_min), this->size())
        );
        shaders::fixed_program.set_uniform("fixed_color", colour);
        shaders::fixed_program.use();

        const auto& geometry = AABB::box_geometry();

        geometry.draw_indexed(
            geometry.buffer(1),
            0,
            geometry.buffer(1).size() / sizeof(unsigned int),
            PrimitiveType::LINES
        );
    }

    AABB operator*(float scale, const AABB& aabb) {
        if (scale >= 0) {
            return AABB(aabb.min() * scale, aabb.max() * scale);
        } else {
            return AABB(aabb.max() * scale, aabb.min() * scale);
        }
    }

    Material Material::without_maps() const {
        return Material {
            .ambient = this->ambient,
            .ambient_occlusion_map = Sampler2D(Texture2D::single_pixel()),

            .diffuse = this->diffuse,
            .diffuse_map = Sampler2D(Texture2D::single_pixel()),

            .specular = this->specular,
            .specular_map = Sampler2D(Texture2D::single_pixel()),

            .shininess = this->shininess
        };
    }

    void ShaderProgram::uniform_setter<Material>::operator ()(
        ShaderProgram& program, std::string name, const Material& value
    ) {
        program.set_uniform(name + ".ambient", value.ambient);
        program.set_uniform(name + ".ambient_occlusion_map", &value.ambient_occlusion_map);

        program.set_uniform(name + ".diffuse", value.diffuse);
        program.set_uniform(name + ".diffuse_map", &value.diffuse_map);

        program.set_uniform(name + ".specular", value.specular);
        program.set_uniform(name + ".specular_map", &value.specular_map);

        program.set_uniform(name + ".shininess", value.shininess);
    }

    struct Model3DVertex {
        glm::vec3 pos;
        glm::vec2 tex;
        glm::vec3 norm;
    };

    class Model3DLoader {
        Model3D* m_model;

        std::vector<glm::vec3> m_pos;
        std::vector<glm::vec2> m_tex;
        std::vector<glm::vec3> m_norm;

        std::vector<Model3DVertex> m_vertices;
        std::map<std::tuple<unsigned int, unsigned int, unsigned int>, unsigned int> m_vertex_indices;

        std::vector<unsigned int> m_current_vertices;

        void emit_subobject();
        unsigned int add_vertex(unsigned int pos, unsigned int tex, unsigned int norm);
        unsigned int add_vertex(std::string spec);
    public:
        Model3DLoader(Model3D* model) : m_model(model) {}
        Model3DLoader(const Model3DLoader& other) = delete;

        Model3DLoader& operator =(const Model3DLoader& other) = delete;

        void handle_line(std::string line);
        void finish();
    };

    void Model3DLoader::emit_subobject() {
        if (this->m_current_vertices.size() > 0) {
            ModelSubObject3D subobj;

            subobj.index_buffer.load_data(this->m_current_vertices, GL_STATIC_DRAW);
            subobj.num_indices = this->m_current_vertices.size();
            this->m_current_vertices.clear();

            this->m_model->m_sub_objects.push_back(std::move(subobj));
        }
    }

    unsigned int Model3DLoader::add_vertex(unsigned int pos, unsigned int tex, unsigned int norm) {
        auto t = std::make_tuple(pos, tex, norm);
        auto existing_entry = this->m_vertex_indices.find(t);

        if (existing_entry != this->m_vertex_indices.end()) {
            return existing_entry->second;
        }

        if (pos >= this->m_pos.size() || tex >= this->m_tex.size() || norm >= this->m_norm.size()) {
            throw std::runtime_error("Face vertex index out of range");
        }

        this->m_vertices.push_back(Model3DVertex {
            .pos = this->m_pos[pos],
            .tex = this->m_tex[tex],
            .norm = this->m_norm[norm]
        });
        this->m_vertex_indices.emplace(t, this->m_vertices.size() - 1);

        return this->m_vertices.size() - 1;
    }

    unsigned int Model3DLoader::add_vertex(std::string spec) {
        std::vector<std::string> parts;

        boost::split(parts, spec, boost::is_any_of("/"));

        if (parts.size() != 3) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Invalid face vertex specification \"" << spec << "\"";

                return ss.str();
            })());
        }

        int pos = std::stoi(parts[0]);
        if (pos < 0) {
            pos += this->m_pos.size() + 1;

            if (pos <= 0) {
                throw std::out_of_range("Negative index out of range");
            }
        } else if (pos == 0) {
            throw std::out_of_range("0 is not a valid index");
        }

        int tex = std::stoi(parts[1]);
        if (tex < 0) {
            tex += this->m_tex.size() + 1;

            if (tex <= 0) {
                throw std::out_of_range("Negative index out of range");
            }
        } else if (tex == 0) {
            throw std::out_of_range("0 is not a valid index");
        }

        int norm = std::stoi(parts[2]);
        if (norm < 0) {
            norm += this->m_norm.size() + 1;

            if (norm <= 0) {
                throw std::out_of_range("Negative index out of range");
            }
        } else if (norm == 0) {
            throw std::out_of_range("0 is not a valid index");
        }

        return this->add_vertex(
            static_cast<unsigned int>(pos - 1),
            static_cast<unsigned int>(tex - 1),
            static_cast<unsigned int>(norm - 1)
        );
    }

    static glm::vec3 parse_vec3(const std::vector<std::string>& parts, int start) {
        return glm::vec3(
            std::stof(parts[start]),
            std::stof(parts[start + 1]),
            std::stof(parts[start + 2])
        );
    }

    static glm::vec2 parse_vec2(const std::vector<std::string>& parts, int start) {
        return glm::vec2(
            std::stof(parts[start]),
            std::stof(parts[start + 1])
        );
    }

    void Model3DLoader::handle_line(std::string line) {
        size_t comment_pos = line.find("#");

        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        boost::trim(line);

        if (line.size() == 0) {
            return;
        }

        std::vector<std::string> parts;

        boost::split(parts, line, boost::is_any_of(" \t"));

        if (parts[0] == "v") {
            if (parts.size() != 4) {
                throw std::runtime_error("Wrong number of arguments for \"v\"");
            }

            this->m_pos.push_back(parse_vec3(parts, 1));
        } else if (parts[0] == "vt") {
            if (parts.size() != 3) {
                throw std::runtime_error("Wrong number of arguments for \"vt\"");
            }

            auto v = parse_vec2(parts, 1);

            this->m_tex.push_back(glm::vec2(v.x, 1 - v.y));
        } else if (parts[0] == "vn") {
            if (parts.size() != 4) {
                throw std::runtime_error("Wrong number of arguments for \"vn\"");
            }

            this->m_norm.push_back(parse_vec3(parts, 1));
        } else if (parts[0] == "f") {
            if (parts.size() != 4) {
                throw std::runtime_error("Wrong number of arguments for \"f\"");
            }

            this->m_current_vertices.push_back(this->add_vertex(parts[1]));
            this->m_current_vertices.push_back(this->add_vertex(parts[2]));
            this->m_current_vertices.push_back(this->add_vertex(parts[3]));
        }
    }

    void Model3DLoader::finish() {
        this->emit_subobject();

        this->m_model->m_vertices.buffer(0).load_data(this->m_vertices, GL_STATIC_DRAW);
        this->m_model->m_vertices.size(this->m_vertices.size());

        if (this->m_vertices.size() > 0) {
            glm::vec3 min(std::numeric_limits<float>::infinity());
            glm::vec3 max(-std::numeric_limits<float>::infinity());

            for (const auto& v : this->m_vertices) {
                if (v.pos.x < min.x) min.x = v.pos.x;
                if (v.pos.x > max.x) max.x = v.pos.x;

                if (v.pos.y < min.y) min.y = v.pos.y;
                if (v.pos.y > max.y) max.y = v.pos.y;

                if (v.pos.z < min.z) min.z = v.pos.z;
                if (v.pos.z > max.z) max.z = v.pos.z;
            }

            this->m_model->m_bounding_box = AABB(min, max);
        } else {
            this->m_model->m_bounding_box = AABB();
        }
    }

    Model3D::Model3D() : m_vertices(1, 0) {}

    Model3D& Model3D::load_geometry(boost::filesystem::path path) {
        boost::filesystem::ifstream f(path);

        if (!f) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Failed to open object file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        this->m_sub_objects.clear();

        Model3DLoader loader(this);
        std::string line;

        while (std::getline(f, line)) {
            loader.handle_line(line);
        }

        if (f.bad()) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Error reading object file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        loader.finish();

        this->m_vertices.bind_attribute(0, 3, DataType::FLOAT, sizeof(Model3DVertex), offsetof(Model3DVertex, pos), 0);
        this->m_vertices.bind_attribute(1, 2, DataType::FLOAT, sizeof(Model3DVertex), offsetof(Model3DVertex, tex), 0);
        this->m_vertices.bind_attribute(2, 3, DataType::FLOAT, sizeof(Model3DVertex), offsetof(Model3DVertex, norm), 0);

        return *this;
    }

    void Model3D::draw() const {
        for (const ModelSubObject3D& so : this->m_sub_objects) {
            this->m_vertices.draw_indexed(so.index_buffer, 0, so.num_indices, PrimitiveType::TRIANGLES);
        }
    }
}
