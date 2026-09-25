#pragma once

struct GLFWwindow;

namespace dal::platform {

// OpenGL 4.6 Core のコンテキストを持つウィンドウ。GLFW の初期化と後始末も担う
class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const;
    void poll_events();
    void swap_buffers();
    void framebuffer_size(int& width, int& height) const;

    // ImGui の GLFW backend に渡すためのハンドル
    GLFWwindow* native_handle() const { return window_; }

private:
    GLFWwindow* window_ = nullptr;
};

using GlProc = void (*)();

// 現在のコンテキストから OpenGL 関数のアドレスを得る（gfx::load_gl に渡す）
GlProc gl_proc_address(const char* name);

} // namespace dal::platform
