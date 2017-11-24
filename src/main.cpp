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
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <scene file>" << std::endl;
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

        World world;
        OrbitControls orbit(&world.camera());

        glm::vec2 window_size = glm::vec2(window.size());

        world.load_scene(boost::filesystem::path(argv[1]));
        world.camera().projection_matrix(glm::infinitePerspective(
            default_fov,
            window_size.x / window_size.y,
            0.1f
        ));

        std::cout << "Scene loaded" << std::endl;

        float edit_speed = 1.0f;
        int edit_object = -1;

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
            } else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
                world.render_settings().draw_lights = !world.render_settings().draw_lights;

                if (world.render_settings().draw_lights) {
                    std::cout << "Light display ENABLED" << std::endl;
                } else {
                    std::cout << "Light display DISABLED" << std::endl;
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
                auto aabb = world.bounding_box();
                auto center = aabb.center();
                float distance = calculate_camera_distance(aabb, default_fov);

                world.camera()
                    .pos(center + glm::vec3(0, 0, distance))
                    .look_at(center, glm::vec3(0, 1, 0));
                orbit.rotate_origin(center);
            } else if (key == GLFW_KEY_I && action == GLFW_PRESS) {
                edit_speed *= 1.1f;
            } else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
                edit_speed /= 1.1f;
            } else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
                if ((mods & GLFW_MOD_SHIFT) == 0) {
                    edit_object++;

                    if (static_cast<size_t>(edit_object) == world.objects().size())
                        edit_object = -1;
                } else {
                    edit_object--;

                    if (edit_object == -2)
                        edit_object = static_cast<int>(world.objects().size()) - 1;
                }
            } else if (key == GLFW_KEY_P && action == GLFW_PRESS && edit_object != -1) {
                auto wrap_angle = [](float angle) {
                    angle = std::remainder(angle, tau);

                    return angle < 0 ? angle + tau : angle;
                };

                Object* o = world.objects()[edit_object].get();
                auto pos = o->pos();
                auto rot = o->orientation();
                auto scale = o->scale();

                std::cout << std::endl;
                std::cout << "  pos " << pos.x << " " << pos.y << " " << pos.z << std::endl;
                std::cout << "  rot " << wrap_angle(rot.yaw) << " "
                                    << wrap_angle(rot.pitch) << " "
                                    << wrap_angle(rot.roll) << std::endl;
                std::cout << "  scale " << scale << std::endl;
            }
        });

        // We will be using the alpha channel, so we need to enable the correct blending mode.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        {
            auto aabb = world.bounding_box();
            auto center = aabb.center();
            float distance = calculate_camera_distance(aabb, default_fov);

            world.camera()
                .pos(center + glm::vec3(0, 0, distance))
                .look_at(center, glm::vec3(0, 1, 0));
            orbit.rotate_origin(center);
        }

        window.do_main_loop([&](double delta_t) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shaders::point_program.set_uniform("point_half_size", glm::vec2(3.0) / window_size);

            if (edit_object >= 0) {
                if (window.is_key_pressed(GLFW_KEY_LEFT_SHIFT) || window.is_key_pressed(GLFW_KEY_RIGHT_SHIFT)) {
                    if (window.is_key_pressed(GLFW_KEY_A))
                        world.objects()[edit_object]->orientation().yaw -= delta_t / 10 * tau * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_D))
                        world.objects()[edit_object]->orientation().yaw += delta_t / 10 * tau * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_W))
                        world.objects()[edit_object]->orientation().pitch -= delta_t / 10 * tau * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_S))
                        world.objects()[edit_object]->orientation().pitch += delta_t / 10 * tau * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_Q))
                        world.objects()[edit_object]->orientation().roll += delta_t / 10 * tau * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_E))
                        world.objects()[edit_object]->orientation().roll -= delta_t / 10 * tau * edit_speed;
                } else {
                    if (window.is_key_pressed(GLFW_KEY_A))
                        world.objects()[edit_object]->pos().x -= delta_t * 1.5f * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_D))
                        world.objects()[edit_object]->pos().x += delta_t * 1.5f * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_W))
                        world.objects()[edit_object]->pos().z -= delta_t * 1.5f * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_S))
                        world.objects()[edit_object]->pos().z += delta_t * 1.5f * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_Q))
                        world.objects()[edit_object]->pos().y -= delta_t * 1.5f * edit_speed;
                    if (window.is_key_pressed(GLFW_KEY_E))
                        world.objects()[edit_object]->pos().y += delta_t * 1.5f * edit_speed;
                }

                if (window.is_key_pressed(GLFW_KEY_Z))
                    world.objects()[edit_object]->scale(
                        world.objects()[edit_object]->scale() * std::pow(1.5f, delta_t * edit_speed)
                    );
                if (window.is_key_pressed(GLFW_KEY_X))
                    world.objects()[edit_object]->scale(
                        world.objects()[edit_object]->scale() / std::pow(1.5f, delta_t * edit_speed)
                    );
            }

            world.draw();

            if (edit_object >= 0) {
                world.objects()[edit_object]->model()->bounding_box().draw(
                    world.camera().view_projection_matrix() * world.objects()[edit_object]->transform_matrix(),
                    glm::vec4(1)
                );
            }
        });

        return 0;
    }
}
