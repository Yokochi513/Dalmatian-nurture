# 0009. モジュール構成を6ターゲットとし、アニメーション計算を描画から分離する

- Status: Accepted
- Date: 2026-09-25

## Context

[0005](0005-build-system-cmake-msvc-cpp20.md) で `core / gfx / app / tests` の4ターゲット構成を
定めたが、その後の決定を踏まえると次の不足がある。

1. ディレクトリ `src/platform/` を持つターゲットが定義されていない
2. [0008](0008-time-progression-hybrid.md) で育成ロジックに時計を外部から注入すると決めたため、
   時計のインターフェースと実装の置き場所が必要になった
3. [0003](0003-no-game-engine-cpp-with-small-libraries.md) で最大の山場としたジョイント行列の
   構築はOpenGLを使わないCPU計算だが、`gfx` に含まれる想定のためテスト対象外になっている
4. 音声（miniaudio）と、犬の自律行動（欲求に応じて次の行動を決める処理）の置き場所が未定

## Decision

ターゲットを次の6つとする。

```
dalmatian_core      育成ロジック（標準C++ と nlohmann/json のみに依存。テスト対象）
dalmatian_anim      glTF読込・スケルトン・アニメーション計算（glm・cgltfに依存、OpenGL非依存。テスト対象）
dalmatian_gfx       OpenGL描画（anim のデータをGPUへ転送して描画する）
dalmatian_platform  OSとの境界：ウィンドウ・入力・システム時計・音声・保存先パス
dalmatian_app       main・ゲームループ・シーン・core と anim の橋渡し・ImGuiデバッグUI（実行ファイル）
dalmatian_tests     core と anim の単体テスト
```

依存方向は次のとおりとし、逆方向の依存を禁止する。

```
app -> core
app -> gfx -> anim
app -> platform
tests -> core, anim
```

- `gfx` は GLFW に依存しない。OpenGL関数のローダは `app` から渡す
- `core` は描画・アニメーションを知らない。犬が「次に何をしたいか」（Idle / Wander / Sleep /
  Beg など）までを決め、どのクリップを再生しどこへ移動するかは `app` が決める
- 時計のインターフェースは `core` に置き、OSの時計を使う実装は `platform` に置く
- 音声は独立ターゲットにせず `platform` に含める

ファイル構成の規約:

- ヘッダ（`.hpp`）とソース（`.cpp`）は同じディレクトリに置く（`include/` を分けない）
- インクルードは `src/` を起点とする（例: `#include "core/dog.hpp"`）
- 名前空間はターゲット名に対応させる（`dal::core`、`dal::anim`、`dal::gfx`、
  `dal::platform`、`dal::app`）
- テストは `tests/core/`、`tests/anim/` に対象ディレクトリと対応させて置く

初期のファイル構成:

```
CMakeLists.txt
cmake/                    Dependencies.cmake（FetchContent）、CompilerOptions.cmake
src/
  core/
    clock.hpp             時計インターフェース
    dog.hpp/.cpp          犬の状態（欲求・なつき度・成長段階・習得済みの芸）
    needs.hpp/.cpp        欲求の時間変化、閉じている間の変化量の上限
    care.hpp/.cpp         ごはん・散歩・なでる・しつけ・遊ぶ（効果・クールダウン・1日の上限）
    growth.hpp/.cpp       世話の積み重ねによる成長
    tricks.hpp/.cpp       芸の習得
    behavior.hpp/.cpp     欲求から次の行動意図を決める
    simulation.hpp/.cpp   1ステップ進める入口
    save.hpp/.cpp         JSONとの相互変換
  anim/
    gltf_loader.hpp/.cpp  .glb をCPU側データ構造へ変換（cgltf）
    mesh_data.hpp         頂点・インデックス・スキンウェイト
    skeleton.hpp/.cpp     ジョイント階層・逆バインド行列
    clip.hpp/.cpp         アニメーションクリップのキーフレーム補間
    pose.hpp/.cpp         ポーズからジョイント行列を計算、クリップ間ブレンド
  gfx/
    gl_objects.hpp/.cpp   Buffer・VertexArray・Texture のRAIIラッパ
    shader.hpp/.cpp       シェーダの読込とuniform設定
    skinned_mesh.hpp/.cpp anim のデータをGPUへ転送
    renderer.hpp/.cpp     描画パス
    camera.hpp/.cpp
    lighting.hpp/.cpp     時刻に応じた昼夜の光
  platform/
    window.hpp/.cpp       GLFW
    input.hpp/.cpp
    system_clock.hpp/.cpp core の時計インターフェースの実装
    audio.hpp/.cpp        miniaudio
    paths.hpp/.cpp        セーブの保存先（%APPDATA%）
  app/
    main.cpp
    game.hpp/.cpp         メインループ
    scene_home.hpp/.cpp   家のシーン
    dog_actor.hpp/.cpp    core の行動意図をアニメーションと移動へ変換
    debug_ui.hpp/.cpp     ImGui：ステータス表示・早送り
shaders/
assets/
  models/                 .glb
  textures/
  audio/
  source/                 .blend などの編集元
tests/
  core/
  anim/
third_party/glad/
docs/adr/
```

初期ファイルは出発点であり、ターゲット内でのファイルの追加・分割はADRを要しない。
ターゲットの追加・削除や依存方向の変更は新しいADRで行う。

本ADRは [0005](0005-build-system-cmake-msvc-cpp20.md) のうち、ターゲット分割・依存方向・
ディレクトリ構成の部分を置き換える。CMake・MSVC・C++20・FetchContent・警告設定・
modules不使用の決定は 0005 のまま有効である。

## Consequences

- スキニングの山場であるジョイント行列の計算を、描画なしで単体テストできる
- 時計の注入（[0008](0008-time-progression-hybrid.md)）の置き場所が確定し、
  テストでは偽の時計、実行時は `platform` の時計を使える
- `core` が描画を知らないため、育成ロジックのテストは状態と行動意図の検証だけで済む
- `core` の行動意図とアニメーションクリップの対応付けを `app` で管理する必要がある
- ターゲットが4つから6つに増え、CMakeの記述量が増える
- 音声を `platform` に含めるため、音声の比重が大きくなった場合は分離を再検討する

## Alternatives considered

- **0005 の4ターゲット構成のまま**: ターゲット数は少ないが、アニメーション計算が
  テストできず、`platform` の所属も曖昧なまま残る
- **anim を gfx に含めたまま、テストだけ gfx を対象にする**: テスト実行にOpenGLコンテキストが
  必要になり、CIやヘッドレス環境で実行できない
- **音声を独立ターゲット（dalmatian_audio）にする**: 責務は明確になるが、現時点では
  ファイル1組のためターゲットを増やす価値が薄い
- **`include/` と `src/` を分ける**: ライブラリとして外部公開する場合の構成であり、
  単一の実行ファイルでは手間が増えるだけ
