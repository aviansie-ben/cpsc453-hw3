#ifndef HW3_WINDOW_HPP
#define HW3_WINDOW_HPP

#include <functional>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace hw3 {
    class Cursor;

    class Window {
        GLFWwindow* m_ptr;
        std::function<void(glm::ivec2)> m_resize_callback;
        std::function<void(glm::dvec2)> m_cursor_move_callback;
        std::function<void(int, int, int)> m_mouse_button_callback;
        std::function<void(glm::dvec2)> m_scroll_callback;
        std::function<void(int, int, int)> m_key_callback;

    public:
        Window(const Window& other) = delete;
        Window(std::string title, int width, int height);
        ~Window();

        Window& operator =(const Window& other) = delete;

        glm::ivec2 size() const;
        glm::dvec2 cursor_pos() const;

        bool is_key_pressed(int key) const;

        void make_current_context() { glfwMakeContextCurrent(this->m_ptr); }

        void set_vsync(bool enabled);
        void set_cursor(const Cursor& cursor);
        void close();

        void handle_resize(int width, int height);
        void set_resize_callback(std::function<void(glm::ivec2)> callback) {
            this->m_resize_callback = std::move(callback);
        }

        void handle_cursor_move(double x, double y);
        void set_cursor_move_callback(std::function<void(glm::dvec2)> callback) {
            this->m_cursor_move_callback = std::move(callback);
        }

        void handle_mouse_button(int button, int action, int mods);
        void set_mouse_button_callback(std::function<void(int, int, int)> callback) {
            this->m_mouse_button_callback = std::move(callback);
        }

        void handle_scroll(double x, double y);
        void set_scroll_callback(std::function<void (glm::dvec2)> callback) {
            this->m_scroll_callback = std::move(callback);
        }

        void handle_key(int key, int scancode, int action, int mods);
        void set_key_callback(std::function<void(int, int, int)> callback) {
            this->m_key_callback = std::move(callback);
        }

        void do_main_loop(std::function<void(double)> callback);
    };

    class Cursor {
        GLFWcursor* m_ptr;

    public:
        Cursor() : m_ptr(nullptr) {}
        Cursor(Cursor&& other) : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
        Cursor(const Cursor& other) = delete;
        Cursor(GLFWcursor* ptr) : m_ptr(ptr) {}
        ~Cursor();

        Cursor& operator =(Cursor&& other) {
            this->m_ptr = other.m_ptr;
            other.m_ptr = nullptr;

            return *this;
        }
        Cursor& operator =(const Cursor& other) = delete;

        static Cursor standard_arrow;
        static Cursor standard_ibeam;
        static Cursor standard_crosshair;
        static Cursor standard_hand;
        static Cursor standard_hresize;
        static Cursor standard_vresize;

        friend void Window::set_cursor(const Cursor&);
    };

    void init_windowing_system();
}

#endif
