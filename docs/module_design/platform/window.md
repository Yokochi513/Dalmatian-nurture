# platform / ウィンドウ（window）

ソース：`src/platform/window.hpp`、`src/platform/window.cpp`　概要：[platform.md](../platform.md)

## 決めたこと

- GLFW の初期化と後始末、OpenGL 4.6 Core のコンテキストを持つウィンドウの作成を担う（実装済み）
- ウィンドウを作れなければ例外を投げる（続けられないため）
- 大きさは 1280×720、大きさの変更を許す。垂直同期を有効にする
- **入力（[input.md](input.md)）はウィンドウが持つ**。ウィンドウの作成時に GLFW のコールバックを登録するため、
  その後に初期化する ImGui の GLFW backend が私たちのコールバックを呼び続けてくれる（ImGui は既存の
  コールバックを連鎖して呼ぶ）
- `poll_events()` は、入力のフレームの区切り（`Input::begin_frame()`）を行ってから GLFW のイベントを処理する

## カーソルの捕捉（ADR 0012）

一人称で歩いている間はマウスで視点を動かし、メニューを開いている間はカーソルを表示する。

```cpp
void set_cursor_captured(bool captured);  // true：カーソルを隠して画面内に固定、マウスの移動量だけを使う
bool cursor_captured() const;
```

- 捕捉中は `GLFW_CURSOR_DISABLED` にし、使える環境なら生のマウスの移動量（raw mouse motion）を使う
- 捕捉を切り替えた直後の最初の移動量は捨てる（カーソルの位置が飛ぶため。[input.md](input.md)）
- いつ捕捉するか（歩いている間・メニュー表示中など）は `app` が決める

## インターフェース

```cpp
class Window {
public:
    Window(int width, int height, const char* title);
    bool should_close() const;
    void poll_events();           // input().begin_frame() → glfwPollEvents()
    void swap_buffers();
    void framebuffer_size(int& width, int& height) const;
    void set_cursor_captured(bool captured);
    bool cursor_captured() const;
    Input& input();
    const Input& input() const;
    GLFWwindow* native_handle() const;  // ImGui の GLFW backend 用
};

using GlProc = void (*)();
GlProc gl_proc_address(const char* name);  // gfx::load_gl に渡す
```

## 検討して採らなかった案

- **入力を独立したクラスにし、`app` で作る**: 分離はされるが、ImGui より先にコールバックを登録する順番を
  `app` が守る必要があり、間違えると ImGui か私たちのどちらかが入力を受け取れなくなる
