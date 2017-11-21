#ifndef HW3_OBJMODEL_HPP
#define HW3_OBJMODEL_HPP

#include <iostream>
#include <vector>

#include <boost/filesystem.hpp>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "texture.hpp"
#include "vertex.hpp"

namespace hw3 {
    class AABB {
        glm::vec3 m_min;
        glm::vec3 m_max;
    public:
        AABB() {}
        AABB(glm::vec3 min, glm::vec3 max) : m_min(min), m_max(max) {}

        const glm::vec3& min() const { return this->m_min; }
        const glm::vec3& max() const { return this->m_max; }

        glm::vec3 size() const { return this->m_max - this->m_min; }
        glm::vec3 center() const { return (this->m_min + this->m_max) / 2.0f; }
    };

    struct Material {
        glm::vec3 ambient;

        glm::vec3 diffuse;
        Sampler2D diffuse_map;

        glm::vec3 specular;
        Sampler2D specular_map;

        float shininess;
    };

    template <>
    struct ShaderProgram::uniform_setter<Material> {
        void operator ()(ShaderProgram& program, std::string name, const Material& value);
    };

    struct ModelSubObject3D {
        GlBuffer index_buffer;
        size_t num_indices;
    };

    class Model3DLoader;
    class Model3D {
        GlVertexArray m_vertices;
        std::vector<ModelSubObject3D> m_sub_objects;
        AABB m_bounding_box;
    public:
        Model3D();

        Model3D& load_geometry(boost::filesystem::path path);

        size_t num_vertices() const { return this->m_vertices.size(); }

        size_t num_sub_objects() const { return this->m_sub_objects.size(); }
        ModelSubObject3D& sub_object(size_t i) {
            assert(i < this->m_sub_objects.size());
            return this->m_sub_objects[i];
        }
        const ModelSubObject3D& sub_object(size_t i) const {
            assert(i < this->m_sub_objects.size());
            return this->m_sub_objects[i];
        }

        const AABB& bounding_box() const { return this->m_bounding_box; }

        void draw() const;

        friend class Model3DLoader;
    };
}

#endif
