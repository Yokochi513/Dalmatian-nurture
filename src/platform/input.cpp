#include "platform/input.hpp"

#include <GLFW/glfw3.h>

#include <optional>

namespace dal::platform {

namespace {

std::optional<Key> key_from_glfw(int glfw_key)
{
    switch (glfw_key) {
    case GLFW_KEY_W: return Key::W;
    case GLFW_KEY_A: return Key::A;
    case GLFW_KEY_S: return Key::S;
    case GLFW_KEY_D: return Key::D;
    case GLFW_KEY_E: return Key::E;
    case GLFW_KEY_ESCAPE: return Key::Escape;
    case GLFW_KEY_SPACE: return Key::Space;
    case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
    case GLFW_KEY_ENTER: return Key::Enter;
    case GLFW_KEY_F1: return Key::F1;
    default: return std::nullopt;
    }
}

std::optional<MouseButton> button_from_glfw(int glfw_button)
{
    switch (glfw_button) {
    case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
    case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
    default: return std::nullopt;
    }
}

std::size_t index(Key key)
{
    return static_cast<std::size_t>(key);
}

std::size_t index(MouseButton button)
{
    return static_cast<std::size_t>(button);
}

} // namespace

bool Input::down(Key key) const
{
    return keys_[index(key)].down;
}

bool Input::pressed(Key key) const
{
    return keys_[index(key)].pressed;
}

bool Input::released(Key key) const
{
    return keys_[index(key)].released;
}

bool Input::down(MouseButton button) const
{
    return mouse_buttons_[index(button)].down;
}

bool Input::pressed(MouseButton button) const
{
    return mouse_buttons_[index(button)].pressed;
}

void Input::begin_frame()
{
    for (ButtonState& key : keys_) {
        key.pressed = false;
        key.released = false;
    }
    for (ButtonState& button : mouse_buttons_) {
        button.pressed = false;
        button.released = false;
    }
    mouse_delta_ = {};
    scroll_ = 0.0;
}

void Input::apply(ButtonState& state, int action)
{
    if (action == GLFW_PRESS) {
        state.pressed = !state.down;
        state.down = true;
    } else if (action == GLFW_RELEASE) {
        state.released = state.down;
        state.down = false;
    }
    // GLFW_REPEAT は押されたままとして扱う
}

void Input::on_key(int glfw_key, int action)
{
    if (const auto key = key_from_glfw(glfw_key)) {
        apply(keys_[index(*key)], action);
    }
}

void Input::on_mouse_button(int glfw_button, int action)
{
    if (const auto button = button_from_glfw(glfw_button)) {
        apply(mouse_buttons_[index(*button)], action);
    }
}

void Input::on_cursor_pos(double x, double y)
{
    if (has_last_cursor_) {
        mouse_delta_.x += x - last_cursor_x_;
        mouse_delta_.y += y - last_cursor_y_;
    }
    has_last_cursor_ = true;
    last_cursor_x_ = x;
    last_cursor_y_ = y;
}

void Input::on_scroll(double y)
{
    scroll_ += y;
}

void Input::on_focus_lost()
{
    // フォーカスを失うと離した通知が届かないため、押されているものをすべて離したものとする
    for (ButtonState& key : keys_) {
        apply(key, GLFW_RELEASE);
    }
    for (ButtonState& button : mouse_buttons_) {
        apply(button, GLFW_RELEASE);
    }
}

void Input::discard_next_mouse_delta()
{
    has_last_cursor_ = false;
}

} // namespace dal::platform
