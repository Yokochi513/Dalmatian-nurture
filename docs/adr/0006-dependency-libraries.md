# 0006. 依存ライブラリの構成

- Status: Accepted
- Date: 2026-09-25

## Context

[0003](0003-no-game-engine-cpp-with-small-libraries.md) で「小規模ライブラリは利用する」
方針を定めた。その具体的な内容を確定する。選定基準は次の3点。

1. ヘッダオンリまたは小規模で、ビルド時間を圧迫しない
2. CMake FetchContent で取得できる（[0005](0005-build-system-cmake-msvc-cpp20.md)）
3. 単一責務であり、ゲーム全体の構造を規定しない（=フレームワークではない）

## Decision

| 用途 | ライブラリ | 備考 |
|---|---|---|
| ウィンドウ・入力 | GLFW 3.4 | |
| OpenGL関数ロード | glad（GL 4.6 Core） | 生成物を `third_party/glad/` にコミット |
| 行列・ベクトル演算 | glm | ヘッダオンリ |
| モデル読込 | cgltf | ヘッダオンリ、glTF 2.0のskin/animationデータに対応 |
| 画像読込 | stb_image | ヘッダオンリ |
| デバッグUI | Dear ImGui | glfw + opengl3 backend を使用 |
| 音声再生 | miniaudio | 単一ヘッダ |
| セーブデータ | nlohmann/json | 目視で内容を確認・編集できる |
| 単体テスト | Catch2 v3 | 対象は `dalmatian_core` のみ |

Dear ImGui は公式のCMakeLists.txtを持たないため、本体とbackendのソースを列挙した
ターゲットを自前で定義する。

ログ出力は当面 `std::format` ベースの自前実装で足りるため、spdlog等は導入しない。

## Consequences

- 依存の大半がヘッダオンリか小規模のため、初回以降のビルドが軽い
- cgltf はパースのみを行うため、**ジョイント行列の構築とスキニングは自作範囲になる**。
  これは [0003](0003-no-game-engine-cpp-with-small-libraries.md) の意図どおり
- Dear ImGui のビルド定義を自分で書く必要がある（更新時に追随が必要）
- 物理演算ライブラリは選定していない。当たり判定が必要になった時点で、
  自作するか導入するかを新しいADRとして決める

## Alternatives considered

- **assimp（モデル読込）**: 多数のフォーマットに対応するがビルドが重く、
  glTFに統一する方針（[0007](0007-asset-pipeline-gltf-blender.md)）なら過剰
- **SDL2/SDL3（ウィンドウ・入力・音声）**: 機能は十分だが、GLFW + miniaudio の
  組み合わせのほうが各層の責務が明確になる
- **独自バイナリ形式でのセーブ**: 高速だが、開発中に内容を目視確認できない
