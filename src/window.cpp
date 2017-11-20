#include <stdexcept>

#include "window.hpp"

namespace hw3 {
    // The callbacks from GLFW are done using function pointers, meaning that the relevant member
    // functions cannot be used directly. Instead, we use these global functions, which find the
    // Window object from the GLFWwindow user pointer and call the relevant member function on it.
    static Window& get_owner(GLFWwindow* window) {
        return *static_cast<Window*>(glfwGetWindowUserPointer(window));
    }

    static void handle_global_resize(GLFWwindow* window, int width, int height) {
        get_owner(window).handle_resize(width, height);
    }

    static void handle_global_cursor_move(GLFWwindow* window, double x, double y) {
        get_owner(window).handle_cursor_move(x, y);
    }

    static void handle_global_mouse_button(GLFWwindow* window, int button, int action, int mods) {
        get_owner(window).handle_mouse_button(button, action, mods);
    }

    static void handle_global_scroll(GLFWwindow* window, double x, double y) {
        get_owner(window).handle_scroll(x, y);
    }

    static void handle_global_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
        get_owner(window).handle_key(key, scancode, action, mods);
    }

    Window::Window(std::string title, int width, int height) {
        this->m_ptr = glfwCreateWindow(width, height, title.c_str(), 0, 0);

        if (!this->m_ptr) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetWindowUserPointer(this->m_ptr, this);
        glfwSetWindowSizeCallback(this->m_ptr, handle_global_resize);
        glfwSetCursorPosCallback(this->m_ptr, handle_global_cursor_move);
        glfwSetMouseButtonCallback(this->m_ptr, handle_global_mouse_button);
        glfwSetScrollCallback(this->m_ptr, handle_global_scroll);
        glfwSetKeyCallback(this->m_ptr, handle_global_key);
    }

    Window::~Window() {
        if (this->m_ptr) {
            glfwDestroyWindow(this->m_ptr);
        }
    }

    glm::ivec2 Window::size() const {
        glm::ivec2 size;

        glfwGetFramebufferSize(this->m_ptr, &size.x, &size.y);

        return size;
    }

    glm::dvec2 Window::cursor_pos() const {
        glm::dvec2 cursor_pos;

        glfwGetCursorPos(this->m_ptr, &cursor_pos.x, &cursor_pos.y);

        return cursor_pos;
    }

    void Window::set_vsync(bool enabled) {
        this->make_current_context();
        glfwSwapInterval(enabled ? 1 : 0);
    }

    void Window::set_cursor(const Cursor& cursor) {
        glfwSetCursor(this->m_ptr, cursor.m_ptr);
    }

    void Window::close() {
        glfwSetWindowShouldClose(this->m_ptr, GLFW_TRUE);
    }

    void Window::handle_resize(int width, int height) {
        glViewport(0, 0, width, height);

        if (this->m_resize_callback) {
            this->m_resize_callback(glm::ivec2(width, height));
        }
    }

    void Window::handle_cursor_move(double x, double y) {
        if (this->m_cursor_move_callback) {
            this->m_cursor_move_callback(glm::dvec2(x, y));
        }
    }

    void Window::handle_mouse_button(int button, int action, int mods) {
        if (this->m_mouse_button_callback) {
            this->m_mouse_button_callback(button, action, mods);
        }
    }

    void Window::handle_scroll(double x, double y) {
        if (this->m_scroll_callback) {
            this->m_scroll_callback(glm::dvec2(x, y));
        }
    }

    void Window::handle_key(int key, int scancode, int action, int mods) {
        if (this->m_key_callback) {
            this->m_key_callback(key, action, mods);
        }
    }

    void Window::do_main_loop(std::function<void(double)> callback) {
        double prevTime = glfwGetTime();
        double nextTime = prevTime;

        while (!glfwWindowShouldClose(this->m_ptr)) {
            callback(nextTime - prevTime);

            prevTime = nextTime;

            glfwSwapBuffers(this->m_ptr);
            glfwPollEvents();

            nextTime = glfwGetTime();
        }
    }

    Cursor::~Cursor() {
        if (this->m_ptr != nullptr) {
            glfwDestroyCursor(this->m_ptr);
        }
    }

    Cursor Cursor::standard_arrow;
    Cursor Cursor::standard_ibeam;
    Cursor Cursor::standard_crosshair;
    Cursor Cursor::standard_hand;
    Cursor Cursor::standard_hresize;
    Cursor Cursor::standard_vresize;

    void init_windowing_system() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        Cursor::standard_arrow = Cursor(glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
        Cursor::standard_ibeam = Cursor(glfwCreateStandardCursor(GLFW_IBEAM_CURSOR));
        Cursor::standard_crosshair = Cursor(glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR));
        Cursor::standard_hand = Cursor(glfwCreateStandardCursor(GLFW_HAND_CURSOR));
        Cursor::standard_hresize = Cursor(glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR));
        Cursor::standard_vresize = Cursor(glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
    }
}
