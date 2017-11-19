#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "font.hpp"
#include "shaderimpl.hpp"
#include "texture.hpp"
#include "window.hpp"

namespace hw3 {
    extern "C" int main(int argc, char** argv) {
        init_windowing_system();
        init_fonts();

        Window window("HW3", 640, 480);

        // Enable v-sync to reduce CPU and GPU usage
        window.set_vsync(true);

        window.make_current_context();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        shaders::init();

        glm::vec2 inverse_window_size = glm::vec2(1.0) / glm::vec2(window.size());

        window.set_mouse_button_callback([&](int button, int action, int mods) {
        });
        window.set_cursor_move_callback([&](glm::dvec2 dpos) {
        });
        window.set_resize_callback([&](glm::ivec2 size) {
            inverse_window_size = glm::vec2(1.0) / glm::vec2(size);
        });
        window.set_scroll_callback([&](glm::dvec2 offset) {
        });

        window.set_key_callback([&](int key, int action, int mods) {
        });

        // We will be using the alpha channel, so we need to enable the correct blending mode.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        window.do_main_loop([&]() {
            glClear(GL_COLOR_BUFFER_BIT);
        });

        return 0;
    }
}
