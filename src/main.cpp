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
        return bound_radius * 2 / std::tan(fov / 2);
    }

    glm::vec2 get_gl_coord(glm::dvec2 screen_coord, const Window& window) {
        glm::vec2 win_size = glm::vec2(window.size());

        return glm::vec2(screen_coord.x, win_size.y - screen_coord.y) / win_size * 2.0f - glm::vec2(1, 1);
    }

    extern "C" int main(int argc, char** argv) {
        if (argc < 2 || argc > 4) {
            std::cerr << "Usage: " << argv[0] << " <object file> [texture] [ambient map]" << std::endl;
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
        OrbitControls orbit(&world.camera());

        glm::vec2 window_size = glm::vec2(window.size());

        world.camera().projection_matrix(glm::infinitePerspective(
            default_fov,
            window_size.x / window_size.y,
            0.1f
        ));

        world.objects().emplace_back(([&]() {
            auto diffuse_tex = argc >= 3
                ? std::make_shared<Texture2D>(Texture2D::load_from_file(argv[2]))
                : Texture2D::single_pixel();
            diffuse_tex->generate_mipmap();

            auto ao_tex = argc >= 4
                ? std::make_shared<Texture2D>(Texture2D::load_from_file(argv[3]))
                : Texture2D::single_pixel();
            ao_tex->generate_mipmap();

            auto obj = std::make_unique<Object>(
                model,
                Material {
                    .ambient = glm::vec3(0.5),
                    .ambient_occlusion_map = std::move(
                        Sampler2D(ao_tex)
                            .set_sample_mode(TextureSampleMode::LINEAR, TextureSampleMode::LINEAR_MIPMAP_LINEAR)
                    ),

                    .diffuse = glm::vec3(0.5),
                    .diffuse_map = std::move(
                        Sampler2D(diffuse_tex)
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

            .ambient = glm::vec3(0.3),
            .diffuse = glm::vec3(1),
            .specular = glm::vec3(0.8),

            .a0 = 1,
            .a1 = 0,
            .a2 = 0.1
        }));

        float edit_speed = 1.0f;

        window.set_mouse_button_callback([&](int button, int action, int mods) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                if (action == GLFW_PRESS) {
                    orbit.begin_rotate(get_gl_coord(window.cursor_pos(), window));
                } else if (action == GLFW_RELEASE) {
                    orbit.end_rotate();
                }
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                if (action == GLFW_PRESS) {
                    orbit.begin_pan(get_gl_coord(window.cursor_pos(), window));
                } else {
                    orbit.end_pan();
                }
            }
        });
        window.set_cursor_move_callback([&](glm::dvec2 dpos) {
            orbit.move_cursor(get_gl_coord(dpos, window));
        });
        window.set_resize_callback([&](glm::ivec2 size) {
            window_size = glm::vec2(size);

            world.camera().projection_matrix(glm::infinitePerspective(
                default_fov,
                window_size.x / window_size.y,
                0.1f
            ));
        });
        window.set_scroll_callback([&](glm::dvec2 offset) {
            orbit.handle_zoom(offset.y);
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
            } else if (key == GLFW_KEY_O && action == GLFW_PRESS) {
                world.render_settings().use_ambient_occlusion = !world.render_settings().use_ambient_occlusion;

                if (world.render_settings().use_ambient_occlusion) {
                    std::cout << "Ambient occlusion ENABLED" << std::endl;
                } else {
                    std::cout << "Ambient occlusion DISABLED" << std::endl;
                }
            } else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
                auto obj_aabb = world.objects()[0]->bounding_box();
                auto obj_center = world.objects()[0]->pos() + obj_aabb.center();
                float distance = calculate_camera_distance(model->bounding_box(), default_fov);

                world.camera()
                    .pos(obj_center + glm::vec3(0, 0, distance))
                    .look_at(obj_center, glm::vec3(0, 1, 0));
                orbit.rotate_origin(obj_center);
            } else if (key == GLFW_KEY_I && action == GLFW_PRESS) {
                edit_speed *= 1.1f;
            } else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
                edit_speed /= 1.1f;
            }
        });

        // We will be using the alpha channel, so we need to enable the correct blending mode.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        {
            auto obj_aabb = world.objects()[0]->bounding_box();
            auto obj_center = world.objects()[0]->pos() + obj_aabb.center();
            float distance = calculate_camera_distance(obj_aabb, default_fov);

            world.camera()
                .pos(obj_center + glm::vec3(0, 0, distance))
                .look_at(obj_center, glm::vec3(0, 1, 0));
            orbit.rotate_origin(obj_center);
        }

        window.do_main_loop([&](double delta_t) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (window.is_key_pressed(GLFW_KEY_LEFT_SHIFT) || window.is_key_pressed(GLFW_KEY_RIGHT_SHIFT)) {
                if (window.is_key_pressed(GLFW_KEY_A))
                    world.objects()[0]->orientation().yaw -= delta_t / 10 * tau * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_D))
                    world.objects()[0]->orientation().yaw += delta_t / 10 * tau * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_W))
                    world.objects()[0]->orientation().pitch -= delta_t / 10 * tau * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_S))
                    world.objects()[0]->orientation().pitch += delta_t / 10 * tau * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_Q))
                    world.objects()[0]->orientation().roll += delta_t / 10 * tau * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_E))
                    world.objects()[0]->orientation().roll -= delta_t / 10 * tau * edit_speed;
            } else {
                if (window.is_key_pressed(GLFW_KEY_A))
                    world.objects()[0]->pos().x -= delta_t * 1.5f * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_D))
                    world.objects()[0]->pos().x += delta_t * 1.5f * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_W))
                    world.objects()[0]->pos().z -= delta_t * 1.5f * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_S))
                    world.objects()[0]->pos().z += delta_t * 1.5f * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_Q))
                    world.objects()[0]->pos().y -= delta_t * 1.5f * edit_speed;
                if (window.is_key_pressed(GLFW_KEY_E))
                    world.objects()[0]->pos().y += delta_t * 1.5f * edit_speed;
            }

            if (window.is_key_pressed(GLFW_KEY_Z))
                world.objects()[0]->scale(
                    world.objects()[0]->scale() * std::pow(1.5f, delta_t * edit_speed)
                );
            if (window.is_key_pressed(GLFW_KEY_X))
                world.objects()[0]->scale(
                    world.objects()[0]->scale() / std::pow(1.5f, delta_t * edit_speed)
                );

            world.draw();
        });

        return 0;
    }
}
