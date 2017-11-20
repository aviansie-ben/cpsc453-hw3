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

namespace hw3 {
    constexpr float default_fov = tau / 6;

    static float calculate_camera_distance(const AABB& bounding_box, float fov) {
        // Start by turning the bounding box into a bounding sphere.
        float bound_radius = glm::distance(bounding_box.min(), bounding_box.center());

        // Now calculate the distance the camera must be to show the entire bounding sphere.
        return bound_radius / std::tan(fov / 2);
    }

    extern "C" int main(int argc, char** argv) {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " [object file]" << std::endl;
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

        Model3D m;

        m.load_geometry(boost::filesystem::path(argv[1]));

        std::cout << "Loaded " << m.num_vertices() << " vertices from " << argv[1] << std::endl;

        glm::vec2 window_size = glm::vec2(window.size());
        glm::vec2 inverse_window_size = glm::vec2(1.0) / window_size;

        window.set_mouse_button_callback([&](int button, int action, int mods) {
        });
        window.set_cursor_move_callback([&](glm::dvec2 dpos) {
        });
        window.set_resize_callback([&](glm::ivec2 size) {
            window_size = glm::vec2(size);
            inverse_window_size = glm::vec2(1.0) / window_size;
        });
        window.set_scroll_callback([&](glm::dvec2 offset) {
        });

        window.set_key_callback([&](int key, int action, int mods) {
        });

        // We will be using the alpha channel, so we need to enable the correct blending mode.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float angle = 0.0;
        float distance = calculate_camera_distance(m.bounding_box(), default_fov);

        distance *= std::sqrt(2);

        window.do_main_loop([&](double delta_t) {
            angle += (tau / 10) * delta_t;

            auto projMatrix = glm::perspective(
                default_fov,
                window_size.x / window_size.y,
                0.1f,
                100.0f
            );
            auto viewMatrix = glm::lookAt(
                m.bounding_box().center() + glm::vec3(std::sin(angle) * distance, distance, std::cos(angle) * distance),
                m.bounding_box().center(),
                glm::vec3(0, 1, 0)
            );

            glClear(GL_COLOR_BUFFER_BIT);
            m.draw(projMatrix * viewMatrix);
        });

        return 0;
    }
}
