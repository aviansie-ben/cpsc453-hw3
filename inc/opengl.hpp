#ifndef HW3_OPENGL_HPP
#define HW3_OPENGL_HPP

#include <sstream>
#include <stdexcept>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace hw3 {
    inline void clear_errors() {
        while (glGetError() != GL_NO_ERROR) {}
    }

    inline void handle_errors() {
        GLenum error = glGetError();

        if (error != GL_NO_ERROR) {
            clear_errors();
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Unhandled OpenGL error 0x" << std::hex << error;

                return ss.str();
            })());
        }
    }
}

#endif
