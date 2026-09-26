# platform モジュール設計

`dalmatian_platform`（`src/platform/`、名前空間 `dal::platform`）の設計の概要。
モジュールをまたぐ決定は [docs/adr/](../adr/) にあり、ここではそれを前提に platform の内部を決める。
各部品の設計は [platform/](platform/) に、ソースファイルと対応する単位で置く（ADR 0043）。

## 責務

- OS との境界：ウィンドウ、入力、システム時計、音声、ファイルの置き場所、セーブファイルの操作（ADR 0009）
- core が定めるインターフェース（時計）を実装する（ADR 0042）
- 画面に何を表示するか（画面設計）は扱わない。画面設計は `app` の範囲

## 設計の方針

- **GLFW・miniaudio・Windows API は platform の外に出さない**。公開ヘッダでは前方宣言か自前の型を使う。
  例外は ImGui の GLFW backend に渡すウィンドウのハンドル（`Window::native_handle()`）だけ
- **初期化に失敗したときの扱いを部品ごとに決める**。ウィンドウが作れなければ続けられないので例外、
  音声が使えなくても遊べるので音なしで続ける
- **テストできる部品はテストする**。セーブファイルの操作は、保存先のディレクトリを外から渡して一時ディレクトリで
  テストする（ADR 0044）

## 部品と設計書

| ソース | 内容 | 設計書 | 状態 |
|---|---|---|---|
| `window.hpp/.cpp` | ウィンドウ、OpenGL のコンテキスト、カーソルの捕捉 | [window.md](platform/window.md) | 設計済み（一部実装済み） |
| `input.hpp/.cpp` | キー・マウスの入力 | [input.md](platform/input.md) | 設計済み |
| `system_clock.hpp/.cpp` | core の時計の本番用の実装 | [system_clock.md](platform/system_clock.md) | 実装済み |
| `audio.hpp/.cpp`、`miniaudio_impl.cpp` | 効果音の再生 | [audio.md](platform/audio.md) | 設計済み |
| `paths.hpp/.cpp` | 実行ファイル・アセット・シェーダ・セーブの置き場所 | [paths.md](platform/paths.md) | 設計済み |
| `save_storage.hpp/.cpp` | セーブファイルの書き込み・読み込み・退避 | [save_storage.md](platform/save_storage.md) | 設計済み |

## テスト方針

- テストは `tests/platform/` に置く（ADR 0044）
- ウィンドウ・入力・音声は、ウィンドウや音声デバイスが要るため単体テストしない。ゲームを起動して確かめる
- セーブファイルの操作とパスの組み立ては、一時ディレクトリを使って単体テストする
