#pragma once

#include <array>
#include <cstddef>

namespace dal::platform {

// ゲームで使うキー。app が GLFW を知らずに済むよう自前の列挙で表す
enum class Key { W, A, S, D, E, Escape, Space, LeftShift, Enter, F1 };
inline constexpr std::size_t kKeyCount = 10;

enum class MouseButton { Left, Right };
inline constexpr std::size_t kMouseButtonCount = 2;

struct MouseDelta {
    double x = 0.0;
    double y = 0.0;
};

// キーとマウスの入力をフレームごとの状態にまとめる。Window が GLFW のコールバックから on_* を呼ぶ
class Input {
public:
    bool down(Key key) const;
    bool pressed(Key key) const;   // このフレームで押された
    bool released(Key key) const;  // このフレームで離された
    bool down(MouseButton button) const;
    bool pressed(MouseButton button) const;
    MouseDelta mouse_delta() const { return mouse_delta_; }  // このフレームのカーソルの移動量（ピクセル）
    double scroll() const { return scroll_; }                // このフレームの縦スクロール量

    // 以下は Window が呼ぶ。GLFW のキー番号・状態は int で受け取る
    void begin_frame();
    void on_key(int glfw_key, int action);
    void on_mouse_button(int glfw_button, int action);
    void on_cursor_pos(double x, double y);
    void on_scroll(double y);
    void on_focus_lost();
    void discard_next_mouse_delta();

private:
    struct ButtonState {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    static void apply(ButtonState& state, int action);

    std::array<ButtonState, kKeyCount> keys_{};
    std::array<ButtonState, kMouseButtonCount> mouse_buttons_{};
    MouseDelta mouse_delta_;
    double scroll_ = 0.0;
    bool has_last_cursor_ = false;
    double last_cursor_x_ = 0.0;
    double last_cursor_y_ = 0.0;
};

} // namespace dal::platform
