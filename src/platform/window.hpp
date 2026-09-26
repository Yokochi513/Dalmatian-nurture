#pragma once

#include "platform/input.hpp"

struct GLFWwindow;

namespace dal::platform {

// OpenGL 4.6 Core のコンテキストを持つウィンドウ。GLFW の初期化と後始末も担う。
// 入力（Input）を持ち、作成時に GLFW のコールバックを登録する。ImGui の GLFW backend はこの後に初期化すれば、
// 登録済みのコールバックを連鎖して呼ぶ
class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const;
    void poll_events();  // input().begin_frame() → GLFW のイベント処理
    void swap_buffers();
    void framebuffer_size(int& width, int& height) const;

    // true：カーソルを隠して画面内に固定し、マウスの移動量だけを使う（一人称の視点操作。ADR 0012）
    void set_cursor_captured(bool captured);
    bool cursor_captured() const { return cursor_captured_; }

    Input& input() { return input_; }
    const Input& input() const { return input_; }

    // ImGui の GLFW backend に渡すためのハンドル
    GLFWwindow* native_handle() const { return window_; }

private:
    GLFWwindow* window_ = nullptr;
    Input input_;
    bool cursor_captured_ = false;
};

using GlProc = void (*)();

// 現在のコンテキストから OpenGL 関数のアドレスを得る（gfx::load_gl に渡す）
GlProc gl_proc_address(const char* name);

} // namespace dal::platform
