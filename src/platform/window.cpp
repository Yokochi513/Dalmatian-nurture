#include "platform/window.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace dal::platform {

namespace {

Input& input_of(GLFWwindow* window)
{
    return static_cast<Window*>(glfwGetWindowUserPointer(window))->input();
}

void on_key(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    input_of(window).on_key(key, action);
}

void on_mouse_button(GLFWwindow* window, int button, int action, int /*mods*/)
{
    input_of(window).on_mouse_button(button, action);
}

void on_cursor_pos(GLFWwindow* window, double x, double y)
{
    input_of(window).on_cursor_pos(x, y);
}

void on_scroll(GLFWwindow* window, double /*x*/, double y)
{
    input_of(window).on_scroll(y);
}

void on_focus(GLFWwindow* window, int focused)
{
    if (focused == GLFW_FALSE) {
        input_of(window).on_focus_lost();
    }
}

} // namespace

Window::Window(int width, int height, const char* title)
{
    if (!glfwInit()) {
        throw std::runtime_error("GLFW の初期化に失敗しました");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("OpenGL 4.6 Core のウィンドウを作成できませんでした");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    // ImGui より先に登録する（ImGui は既存のコールバックを連鎖して呼ぶ）
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, on_key);
    glfwSetMouseButtonCallback(window_, on_mouse_button);
    glfwSetCursorPosCallback(window_, on_cursor_pos);
    glfwSetScrollCallback(window_, on_scroll);
    glfwSetWindowFocusCallback(window_, on_focus);
}

Window::~Window()
{
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Window::should_close() const
{
    return glfwWindowShouldClose(window_) != 0;
}

void Window::poll_events()
{
    input_.begin_frame();
    glfwPollEvents();
}

void Window::swap_buffers()
{
    glfwSwapBuffers(window_);
}

void Window::framebuffer_size(int& width, int& height) const
{
    glfwGetFramebufferSize(window_, &width, &height);
}

void Window::set_cursor_captured(bool captured)
{
    if (captured == cursor_captured_) {
        return;
    }
    cursor_captured_ = captured;
    glfwSetInputMode(window_, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
    }
    // 切り替えた直後はカーソルの位置が飛ぶため、最初の移動量を捨てる
    input_.discard_next_mouse_delta();
}

GlProc gl_proc_address(const char* name)
{
    return glfwGetProcAddress(name);
}

} // namespace dal::platform
