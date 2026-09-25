#include "platform/window.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace dal::platform {

Window::Window(int width, int height, const char* title)
{
    if (!glfwInit()) {
        throw std::runtime_error("GLFW の初期化に失敗しました");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("OpenGL 4.6 Core のウィンドウを作成できませんでした");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
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

GlProc gl_proc_address(const char* name)
{
    return glfwGetProcAddress(name);
}

} // namespace dal::platform
