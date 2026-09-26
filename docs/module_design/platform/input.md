# platform / 入力（input）

ソース：`src/platform/input.hpp`、`src/platform/input.cpp`　概要：[platform.md](../platform.md)

キーとマウスの入力を、フレームごとの状態として `app` に渡す。

## 決めたこと

- GLFW のコールバック（キー・マウスボタン・カーソル位置・スクロール）で受け取り、フレームごとの状態にまとめる
- `app` が GLFW を知らずに済むよう、**キーは自前の列挙** で表す。使うキーだけを並べる
- 状態は「押されている（down）」「このフレームで押された（pressed）」「このフレームで離された（released）」
- マウスは、このフレームの移動量とスクロール量を返す
- **文字入力（名前入力）は扱わない**。名前入力は ImGui の入力欄が ImGui 自身のコールバックで受け取る（ADR 0013）
- ImGui が入力を使っているか（`WantCaptureKeyboard` など）で、ゲームの操作を止めるかどうかは `app` が決める
- キーの割り当ての変更は扱わない（固定）

## キー

| 列挙 | キー | 用途（ADR 0012） |
|---|---|---|
| `W`・`A`・`S`・`D` | W・A・S・D | 移動 |
| `E` | E | 視線の先の対象へのメニュー |
| `Escape` | Esc | メニューを閉じる・カーソルの捕捉を外す |
| `Space` | スペース | 予備 |
| `LeftShift` | 左シフト | 予備（走るなど） |
| `Enter` | Enter | 決定 |
| `F1` | F1 | デバッグUIの表示の切り替え |

マウスボタンは `Left`・`Right`。

## フレームの区切り

```
Window::poll_events()
  ├─ Input::begin_frame()   // pressed・released・移動量・スクロール量を 0 に戻す
  └─ glfwPollEvents()       // コールバックで今フレームの入力がたまる
app がこのフレームの入力を読む
```

- カーソルの捕捉を切り替えた直後の最初の移動量は捨てる（[window.md](window.md)）
- ウィンドウがフォーカスを失ったら、押されているキーをすべて離したものとする（キーが押されたままになるのを防ぐ）

## インターフェース

```cpp
enum class Key { W, A, S, D, E, Escape, Space, LeftShift, Enter, F1 };
enum class MouseButton { Left, Right };

struct MouseDelta {
    double x = 0.0;
    double y = 0.0;
};

class Input {
public:
    bool down(Key key) const;
    bool pressed(Key key) const;
    bool released(Key key) const;
    bool down(MouseButton button) const;
    bool pressed(MouseButton button) const;
    MouseDelta mouse_delta() const;  // このフレームのカーソルの移動量（ピクセル）
    double scroll() const;           // このフレームの縦スクロール量

    // 以下は Window が呼ぶ
    void begin_frame();
    void on_key(int glfw_key, int action);
    void on_mouse_button(int glfw_button, int action);
    void on_cursor_pos(double x, double y);
    void on_scroll(double y);
    void on_focus_lost();
    void discard_next_mouse_delta();
};
```

`on_*` は GLFW のキー番号を受け取るが、ヘッダでは `int` とし GLFW のヘッダを含めない。

## 検討して採らなかった案

- **毎フレーム `glfwGetKey` で問い合わせる**: コールバックの登録順（ImGui との連鎖）を気にせずに済むが、
  フレームの間に押して離した短い入力を取りこぼす
- **GLFW のキー番号をそのまま `app` に渡す**: 列挙を作る手間はないが、`app` が GLFW に依存する
