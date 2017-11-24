#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shaderimpl.hpp"
#include "world.hpp"

namespace hw3 {
    glm::mat4 Orientation::apply(const glm::mat4& transform) const {
        return glm::rotate(
            glm::rotate(
                glm::rotate(transform, this->yaw, glm::vec3(0, 1, 0)),
                this->pitch,
                glm::vec3(1, 0, 0)
            ),
            this->roll,
            glm::vec3(0, 0, 1)
        );
    }

    glm::mat4 Object::transform_matrix() const {
        return this->m_orientation.apply(
            glm::scale(glm::translate(glm::mat4(1), this->m_pos), glm::vec3(this->m_scale))
        );
    }

    AABB Object::bounding_box() const {
        return this->m_model->bounding_box() * this->transform_matrix();
    }

    void Object::draw(
        ShaderProgram& program,
        const RenderSettings& render_settings,
        const glm::mat4& view_projection_matrix
    ) const {
        auto model_matrix = this->transform_matrix();

        program.set_uniform("vertex_transform", view_projection_matrix * model_matrix);
        program.set_uniform("vertex_world_transform", model_matrix);
        program.set_uniform("normal_transform", glm::transpose(glm::inverse(glm::mat3(model_matrix))));

        {
            Material m = this->m_material;

            if (!render_settings.draw_textures) {
                m = m.without_maps();
            } else {
                if (!render_settings.use_ambient_occlusion) {
                    m = m.without_ao();
                }
            }

            program.set_uniform("material", m);
        }

        program.use();

        this->m_model->draw();

        if (render_settings.draw_bounding_boxes) {
            this->m_model->bounding_box().draw(
                view_projection_matrix * model_matrix,
                glm::vec4(1, 0, 0, 1)
            );

            this->bounding_box().draw(
                view_projection_matrix,
                glm::vec4(0, 0, 1, 1)
            );
        }
    }

    void ShaderProgram::uniform_setter<PointLight>::operator ()(
        ShaderProgram& program,
        std::string name,
        const PointLight& value
    ) {
        program.set_uniform(name + ".pos", value.pos);

        program.set_uniform(name + ".ambient", value.ambient);
        program.set_uniform(name + ".diffuse", value.diffuse);
        program.set_uniform(name + ".specular", value.specular);

        program.set_uniform(name + ".a0", value.a0);
        program.set_uniform(name + ".a1", value.a1);
        program.set_uniform(name + ".a2", value.a2);
    }

    void OrbitControls::begin_rotate(glm::vec2 pos) {
        if (this->m_state == OrbitState::NONE) {
            this->m_state = OrbitState::ROTATING;
            this->m_last_pos = pos;
        }
    }

    void OrbitControls::begin_pan(glm::vec2 pos) {
        if (this->m_state == OrbitState::NONE) {
            this->m_state = OrbitState::PANNING;
            this->m_last_pos = pos;
        }
    }

    static glm::vec3 calculate_posball(glm::vec2 pos) {
        float dist_sq = pos.x * pos.x + pos.y * pos.y;

        if (dist_sq <= 1) {
            return glm::vec3(pos, std::sqrt(1 - dist_sq));
        } else {
            return glm::vec3(glm::normalize(pos), 0);
        }
    }

    void OrbitControls::move_cursor(glm::vec2 pos) {
        switch (this->m_state) {
        case OrbitState::ROTATING: {
            auto camera_orientation = this->m_camera->orientation_matrix();
            auto camera_pos = this->m_camera->pos();

            auto last_posball = calculate_posball(this->m_last_pos);
            auto posball = calculate_posball(pos);

            float angle = std::acos(std::min(1.0f, glm::dot(last_posball, posball)));
            auto axis = glm::inverse(camera_orientation)
                * glm::cross(last_posball, posball);

            // If posball == last_posball, then glm::cross(posball, last_posball) will be the null
            // vector and thus the axis of rotation will also be the null vector. Trying to rotate
            // using the null vector as an axis causes the resulting rotation matrix to be full of
            // NaNs, which breaks everything. Instead, we should just cancel the rotation.
            if (axis == glm::vec3()) {
                break;
            }

            auto rotate_matrix = glm::mat3(glm::rotate(
                glm::mat4(1),
                angle,
                axis
            ));

            camera_orientation = camera_orientation * rotate_matrix;
            camera_pos = this->m_rotate_origin
                + glm::transpose(rotate_matrix) * (camera_pos - this->m_rotate_origin);

            this->m_camera->orientation_matrix(camera_orientation);
            this->m_camera->pos(camera_pos);

            break;
        }
        case OrbitState::PANNING: {
            auto delta3 = glm::inverse(this->m_camera->orientation_matrix())
                * glm::vec3(this->m_last_pos - pos, 0)
                * glm::distance(this->m_camera->pos(), this->m_rotate_origin);

            this->m_camera->pos(this->m_camera->pos() + delta3);
            this->m_rotate_origin += delta3;

            break;
        }
        case OrbitState::NONE:
            break; // Do nothing
        }

        this->m_last_pos = pos;
    }

    void OrbitControls::handle_zoom(float delta) {
        glm::vec3 camera_look = this->m_camera->pos() - this->m_rotate_origin;

        this->m_camera->pos(this->m_rotate_origin + camera_look * std::pow(2.0f, delta * -0.25f));
    }

    void OrbitControls::end_rotate() {
        if (this->m_state == OrbitState::ROTATING) {
            this->m_state = OrbitState::NONE;
        }
    }

    void OrbitControls::end_pan() {
        if (this->m_state == OrbitState::PANNING) {
            this->m_state = OrbitState::NONE;
        }
    }

    ShaderProgram& World::select_program() const {
        switch (this->m_render_settings.mode) {
        case RenderMode::STANDARD:
        case RenderMode::FULL_BRIGHT:
            return shaders::phong_program;
        case RenderMode::NORMALS:
            return shaders::normal_program;
        default:
            throw std::runtime_error("Invalid render mode");
        }
    }

    AABB World::bounding_box() const {
        glm::vec3 min(std::numeric_limits<float>::infinity());
        glm::vec3 max(-std::numeric_limits<float>::infinity());

        for (const auto& o : this->m_objects) {
            auto aabb = o->bounding_box();

            if (aabb.min().x < min.x) min.x = aabb.min().x;
            if (aabb.max().x > max.x) max.x = aabb.max().x;

            if (aabb.min().y < min.y) min.y = aabb.min().y;
            if (aabb.max().y > max.y) max.y = aabb.max().y;

            if (aabb.min().z < min.z) min.z = aabb.min().z;
            if (aabb.max().z > max.z) max.z = aabb.max().z;
        }

        return AABB(min, max);
    }

    class SceneLoader {
        World* m_world;
        std::istream* m_stream;
        boost::filesystem::path m_dir;

        std::map<std::string, std::shared_ptr<Model3D>> m_models;
        std::map<std::string, Material> m_materials;

        size_t m_current_line_number = 0;
        std::vector<std::string> m_current_line;
        size_t m_current_indent;

        bool read_next_line();
        std::runtime_error syntax_error(std::function<void (std::ostream&)> fn);
        boost::filesystem::path resolve_path(std::string path);

        glm::vec3 read_vec3(size_t offset);

        void parse_mdl();
        void parse_mtl();
        void parse_plight();
        void parse_obj();
    public:
        SceneLoader(
            World* world,
            std::istream* stream,
            boost::filesystem::path dir
        ) : m_world(world), m_stream(stream), m_dir(dir) {}

        void load();
    };

    static size_t count_indentation(const std::string& line) {
        size_t indentation = 0;

        for (char c : line) {
            if (c == ' ') {
                indentation += 1;
            } else if (c == '\t') {
                indentation += 4;
            } else {
                break;
            }
        }

        return indentation;
    }

    bool SceneLoader::read_next_line() {
        std::string line;

        while (std::getline(*this->m_stream, line)) {
            size_t comment_pos = line.find("#");

            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            size_t indent = count_indentation(line);
            boost::trim(line);

            this->m_current_line_number++;

            if (line.size() > 0) {
                this->m_current_line.clear();
                boost::split(
                    this->m_current_line,
                    line,
                    boost::is_any_of(" \t"),
                    boost::token_compress_on
                );

                this->m_current_indent = indent;
                return true;
            }
        }

        this->m_current_line.clear();
        this->m_current_indent = 0;
        return false;
    }

    std::runtime_error SceneLoader::syntax_error(std::function<void (std::ostream&)> fn) {
        std::ostringstream ss;

        ss << "Syntax error on line " << this->m_current_line_number << ": ";
        fn(ss);

        return std::runtime_error(ss.str());
    }

    boost::filesystem::path SceneLoader::resolve_path(std::string path) {
        boost::filesystem::path p(path);

        if (p.is_absolute()) {
            return p;
        } else {
            return this->m_dir / p;
        }
    }

    glm::vec3 SceneLoader::read_vec3(size_t offset) {
        assert(this->m_current_line.size() >= offset + 2);

        return glm::vec3(
            std::stof(this->m_current_line[offset]),
            std::stof(this->m_current_line[offset + 1]),
            std::stof(this->m_current_line[offset + 2])
        );
    }

    void SceneLoader::parse_mdl() {
        if (this->m_current_line.size() != 3) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for mdl command";
            });
        }

        if (this->m_models.find(this->m_current_line[1]) != this->m_models.end()) {
            throw this->syntax_error([&](auto& ss) {
                ss << "A model \"" << this->m_current_line[1] << "\" already exists";
            });
        }

        std::shared_ptr<Model3D> m = std::make_shared<Model3D>();
        m->load_geometry(this->resolve_path(this->m_current_line[2]));

        this->m_models[this->m_current_line[1]] = std::move(m);

        this->read_next_line();
    }

    void SceneLoader::parse_mtl() {
        if (this->m_current_line.size() != 2) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for mtl command";
            });
        }

        auto name = this->m_current_line[1];
        size_t indent = this->m_current_indent;

        auto material = Material {
            .ambient = glm::vec3(1, 1, 1),
            .ambient_occlusion_map = Sampler2D::single_pixel(),

            .diffuse = glm::vec3(1, 1, 1),
            .diffuse_map = Sampler2D::single_pixel(),

            .specular = glm::vec3(1, 1, 1),
            .specular_map = Sampler2D::single_pixel(),

            .shininess = 1
        };

        auto read_vec3_attr = [&](const std::string& name) {
            if (this->m_current_line.size() != 4) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Wrong number of arguments for mtl::" << name << " attribute";
                });
            }

            try {
                return this->read_vec3(1);
            } catch (std::exception& e) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Invalid argument for mtl::" << name << " attribute";
                });
            }
        };

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "ambient") {
                    material.ambient = read_vec3_attr("ambient");
                } else if (cmd == "diffuse") {
                    material.diffuse = read_vec3_attr("diffuse");
                } else if (cmd == "specular") {
                    material.specular = read_vec3_attr("specular");
                } else if (cmd == "shininess") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for mtl::shininess attribute";
                        });
                    }

                    try {
                        material.shininess = std::stof(this->m_current_line[1]);
                    } catch (std::exception& e) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Invalid argument for mtl::shininess attribute";
                        });
                    }
                } else if (cmd == "ao_map") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for mtl::ao_map attribute";
                        });
                    }

                    material.ambient_occlusion_map = std::make_shared<Sampler2D>(
                        std::make_shared<Texture2D>(Texture2D::load_from_file(
                            this->resolve_path(this->m_current_line[1]).native()
                        ))
                    );
                } else if (cmd == "diffuse_map") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for mtl::diffuse_map attribute";
                        });
                    }

                    material.diffuse_map = std::make_shared<Sampler2D>(
                        std::make_shared<Texture2D>(Texture2D::load_from_file(
                            this->resolve_path(this->m_current_line[1]).native()
                        ))
                    );
                } else if (cmd == "specular_map") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for mtl::specular_map attribute";
                        });
                    }

                    material.specular_map = std::make_shared<Sampler2D>(
                        std::make_shared<Texture2D>(Texture2D::load_from_file(
                            this->resolve_path(this->m_current_line[1]).native()
                        ))
                    );
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid mtl attribute \"" << cmd << "\"";
                    });
                }
            } while(this->read_next_line() && this->m_current_indent == indent);
        }

        this->m_materials[name] = std::move(material);
    }

    void SceneLoader::parse_plight() {
        if (this->m_current_line.size() != 1) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for plight command";
            });
        }

        size_t indent = this->m_current_indent;

        auto plight = PointLight {
            .pos = glm::vec3(0, 0, 0),

            .ambient = glm::vec3(0, 0, 0),
            .diffuse = glm::vec3(0, 0, 0),
            .specular = glm::vec3(0, 0, 0),

            .a0 = 1.0,
            .a1 = 0.0,
            .a2 = 0.0
        };

        auto read_vec3_attr = [&](const std::string& name) {
            if (this->m_current_line.size() != 4) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Wrong number of arguments for plight::" << name << " attribute";
                });
            }

            try {
                return this->read_vec3(1);
            } catch (std::exception& e) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Invalid argument for plight::" << name << " attribute";
                });
            }
        };

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "pos") {
                    plight.pos = read_vec3_attr("pos");
                } else if (cmd == "ambient") {
                    plight.ambient = read_vec3_attr("ambient");
                } else if (cmd == "diffuse") {
                    plight.diffuse = read_vec3_attr("diffuse");
                } else if (cmd == "specular") {
                    plight.specular = read_vec3_attr("specular");
                } else if (cmd == "atten") {
                    auto atten = read_vec3_attr("atten");

                    plight.a0 = atten.x;
                    plight.a1 = atten.y;
                    plight.a2 = atten.z;
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid plight attribute \"" << cmd << "\"";
                    });
                }
            } while (this->read_next_line() && this->m_current_indent == indent);
        }

        this->m_world->point_lights().push_back(std::make_unique<PointLight>(std::move(plight)));
    }

    void SceneLoader::parse_obj() {
        if (this->m_current_line.size() != 1) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for obj command";
            });
        }

        size_t indent = this->m_current_indent;

        std::shared_ptr<Model3D> mdl;
        auto mtl = Material {
            .ambient = glm::vec3(1, 1, 1),
            .ambient_occlusion_map = Sampler2D::single_pixel(),

            .diffuse = glm::vec3(1, 1, 1),
            .diffuse_map = Sampler2D::single_pixel(),

            .specular = glm::vec3(1, 1, 1),
            .specular_map = Sampler2D::single_pixel(),

            .shininess = 1
        };

        glm::vec3 pos;
        glm::vec3 rot;
        float scale = 1.0f;

        auto read_vec3_attr = [&](const std::string& name) {
            if (this->m_current_line.size() != 4) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Wrong number of arguments for obj::" << name << " attribute";
                });
            }

            try {
                return this->read_vec3(1);
            } catch (std::exception& e) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Invalid argument for obj::" << name << " attribute";
                });
            }
        };

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "mdl") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for obj::mdl attribute";
                        });
                    }

                    auto mdl_it = this->m_models.find(this->m_current_line[1]);

                    if (mdl_it == this->m_models.end()) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "No such model: " << this->m_current_line[1];
                        });
                    }

                    mdl = mdl_it->second;
                } else if (cmd == "mtl") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for obj::mtl attribute";
                        });
                    }

                    auto mtl_it = this->m_materials.find(this->m_current_line[1]);

                    if (mtl_it == this->m_materials.end()) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "No such material: " << this->m_current_line[1];
                        });
                    }

                    mtl = mtl_it->second;
                } else if (cmd == "pos") {
                    pos = read_vec3_attr("pos");
                } else if (cmd == "rot") {
                    rot = read_vec3_attr("rot");
                } else if (cmd == "scale") {
                    if (this->m_current_line.size() != 2) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Wrong number of arguments for obj::scale attribute";
                        });
                    }

                    try {
                        scale = std::stof(this->m_current_line[1]);
                    } catch (std::exception& e) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "Invalid argument for obj::scale attribute";
                        });
                    }
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid obj attribute \"" << cmd << "\"";
                    });
                }
            } while (this->read_next_line() && this->m_current_indent == indent);
        }

        if (!mdl) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Missing obj::mdl attribute";
            });
        }

        this->m_world->objects().push_back(std::make_unique<Object>(([&]() {
            Object obj(mdl, mtl);

            obj.pos() = pos;
            obj.orientation() = Orientation(rot.x, rot.y, rot.z);
            obj.scale(scale);

            return std::move(obj);
        })()));
    }

    void SceneLoader::load() {
        if (!this->read_next_line()) {
            return;
        }

        while (this->m_current_line.size() > 0) {
            const auto& cmd = this->m_current_line[0];

            if (this->m_current_indent != 0) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Bad indentation";
                });
            }

            if (cmd == "mdl") {
                this->parse_mdl();
            } else if (cmd == "mtl") {
                this->parse_mtl();
            } else if (cmd == "plight") {
                this->parse_plight();
            } else if (cmd == "obj") {
                this->parse_obj();
            } else {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Invalid scene command \"" << cmd << "\"";
                });
            }
        }
    }

    World& World::load_scene(boost::filesystem::path path) {
        boost::filesystem::ifstream f(path);

        if (!f) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Failed to open object file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        this->m_objects.clear();
        this->m_point_lights.clear();

        SceneLoader loader(this, &f, path.parent_path());

        loader.load();

        if (f.bad()) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Error reading scene file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        return *this;
    }

    void World::draw() const {
        auto& program = this->select_program();
        auto view_projection_matrix = this->camera().view_projection_matrix();

        program.set_uniform("camera_position", this->camera().pos());

        if (this->m_point_lights.size() > 16) {
            throw std::runtime_error("Too many point lights");
        }

        if (this->m_render_settings.mode == RenderMode::STANDARD) {
            program.set_uniform("scene_ambient", glm::vec3(0));
            program.set_uniform("num_point_lights", static_cast<int>(this->m_point_lights.size()));
            for (size_t i = 0; i < this->m_point_lights.size(); i++) {
                program.set_uniform(([&]() {
                    std::ostringstream ss;

                    ss << "point_lights[" << i << "]";

                    return ss.str();
                })(), *this->m_point_lights[i]);
            }
        } else {
            program.set_uniform("scene_ambient", glm::vec3(1));
            program.set_uniform("num_point_lights", 0);
        }

        for (const auto& obj : this->m_objects) {
            obj->draw(program, this->m_render_settings, view_projection_matrix);
        }

        if (this->m_render_settings.draw_bounding_boxes && this->m_objects.size() > 1) {
            this->bounding_box().draw(view_projection_matrix, glm::vec4(0, 1, 0, 1));
        }
    }
}
