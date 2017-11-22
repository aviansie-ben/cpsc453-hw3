#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "font.hpp"
#include "objmodel.hpp"
#include "shaderimpl.hpp"
#include "texture.hpp"
#include "window.hpp"
#include "world.hpp"

namespace hw3 {
    constexpr float default_fov = tau / 6;

    static float calculate_camera_distance(const AABB& bounding_box, float fov) {
        // TODO: This doesn't take into account that the AABB is not at the origin.
        // Start by turning the bounding box into a bounding sphere.
        float bound_radius = glm::distance(bounding_box.min(), bounding_box.center());

        // Now calculate the distance the camera must be to show the entire bounding sphere.
        return bound_radius / std::tan(fov / 2);
    }

    extern "C" int main(int argc, char** argv) {
        if (argc != 2 && argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <object file> [texture]" << std::endl;
            return 1;
        }

        init_windowing_system();
        init_fonts();

        Window window("HW3", 640, 480);

        // Enable v-sync to reduce CPU and GPU usage
        window.set_vsync(true);

        window.make_current_context();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        shaders::init();

        std::shared_ptr<Model3D> model = std::make_shared<Model3D>();

        model->load_geometry(boost::filesystem::path(argv[1]));

        std::cout << "Loaded " << model->num_vertices() << " vertices from " << argv[1] << std::endl;

        World world;

        glm::vec2 window_size = glm::vec2(window.size());

        world.camera().projection_matrix(glm::infinitePerspective(
            default_fov,
            window_size.x / window_size.y,
            0.1f
        ));

        world.objects().emplace_back(([&]() {
            auto tex = argc == 3
                ? std::make_shared<Texture2D>(Texture2D::load_from_file(argv[2]))
                : Texture2D::single_pixel();
            tex->generate_mipmap();

            auto obj = std::make_unique<Object>(
                model,
                Material {
                    .ambient = glm::vec3(0.5),

                    .diffuse = glm::vec3(0.5),
                    .diffuse_map = std::move(
                        Sampler2D(tex)
                            .set_sample_mode(TextureSampleMode::LINEAR, TextureSampleMode::LINEAR_MIPMAP_LINEAR)
                    ),

                    .specular = glm::vec3(0.5),
                    .specular_map = Sampler2D(Texture2D::single_pixel()),

                    .shininess = 10
                }
            );

            return obj;
        })());
        world.point_lights().push_back(std::make_unique<PointLight>(PointLight {
            .pos = glm::vec3(2),

            .ambient = glm::vec3(0.1, 0, 0),
            .diffuse = glm::vec3(0.5, 0, 0),
            .specular = glm::vec3(0.8, 0, 0),

            .a0 = 1,
            .a1 = 0,
            .a2 = 0.1
        }));

        window.set_mouse_button_callback([&](int button, int action, int mods) {
        });
        window.set_cursor_move_callback([&](glm::dvec2 dpos) {
        });
        window.set_resize_callback([&](glm::ivec2 size) {
            window_size = glm::vec2(size);

            world.camera().projection_matrix(glm::perspective(
                default_fov,
                window_size.x / window_size.y,
                0.1f,
                100.0f
            ));
        });
        window.set_scroll_callback([&](glm::dvec2 offset) {
        });

        window.set_key_callback([&](int key, int action, int mods) {
            if (key == GLFW_KEY_R && action == GLFW_PRESS) {
                switch (world.render_settings().mode) {
                case RenderMode::STANDARD:
                    world.render_settings().mode = RenderMode::FULL_BRIGHT;
                    std::cout << "Render Mode: FULL_BRIGHT" << std::endl;

                    break;
                case RenderMode::FULL_BRIGHT:
                    world.render_settings().mode = RenderMode::NORMALS;
                    std::cout << "Render Mode: NORMALS" << std::endl;

                    break;
                case RenderMode::NORMALS:
                    world.render_settings().mode = RenderMode::STANDARD;
                    std::cout << "Render Mode: STANDARD" << std::endl;

                    break;
                }
            } else if (key == GLFW_KEY_B && action == GLFW_PRESS) {
                world.render_settings().draw_bounding_boxes = !world.render_settings().draw_bounding_boxes;

                if (world.render_settings().draw_bounding_boxes) {
                    std::cout << "Bounding boxes ENABLED" << std::endl;
                } else {
                    std::cout << "Bounding boxes DISABLED" << std::endl;
                }
            } else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
                world.render_settings().draw_textures = !world.render_settings().draw_textures;

                if (world.render_settings().draw_textures) {
                    std::cout << "Texture mapping ENABLED" << std::endl;
                } else {
                    std::cout << "Texture mapping DISABLED" << std::endl;
                }
            }
        });

        // We will be using the alpha channel, so we need to enable the correct blending mode.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float angle = 0.0;
        float distance = calculate_camera_distance(model->bounding_box(), default_fov);

        distance *= std::sqrt(2);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        window.do_main_loop([&](double delta_t) {
            angle += (tau / 10) * delta_t;

            world.camera()
                .pos(glm::vec3(std::sin(angle) * distance, distance, std::cos(angle) * distance))
                .look_at(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            world.draw();
        });

        return 0;
    }
}
