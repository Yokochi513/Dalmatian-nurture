# ダルメシアン育成ゲーム

## 目的
ダルメシアンの育成3Dゲームを作成し、


## 技術スタック

| 領域 | 選定 |
|---|---|
| 対象環境 | Windows デスクトップ（実行ファイル配布） |
| 言語 | C++20 / MSVC (Visual Studio 2022) |
| 描画API | OpenGL 4.6 Core |
| ゲームエンジン | 使用しない（レンダラ・アニメーション再生・育成ロジックは自作） |
| ビルド | CMake + Ninja、依存取得は FetchContent |
| 主な依存 | GLFW / glad / glm / cgltf / stb_image / Dear ImGui / miniaudio / nlohmann-json / Catch2 |
| 3Dアセット | Blender 経由で glTF 2.0 (.glb) に統一 |

選定理由は [docs/adr/](docs/adr/) の各ADRを参照。

## ドキュメント
- 決定事項: [docs/adr/](docs/adr/) — 1決定につき1ファイル（[運用ルール](docs/adr/0001-record-decisions-in-adr.md)）
- エージェント向けルール: [AGENT.md](AGENT.md)

##
