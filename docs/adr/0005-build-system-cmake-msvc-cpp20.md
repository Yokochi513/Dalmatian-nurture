# 0005. ビルドを CMake + MSVC / C++20 で構成し、層ごとにターゲットを分割する

- Status: Partially superseded by [0009](0009-module-structure.md)（ターゲット分割・依存方向・ディレクトリ構成）
- Date: 2026-09-25

## Context

エンジンを使わないため、ビルド構成と依存関係の取得方法を自分で決める必要がある。
開発者は大規模なC++コードベースの経験がないため、構成が破綻しにくいことを優先する。

## Decision

- ビルドシステムは **CMake（3.24以上）**、ジェネレータは **Ninja**
- コンパイラは **MSVC（Visual Studio 2022 Build Tools）**、規格は **C++20**
- 依存関係の取得は **CMake FetchContent**
- 警告設定は `/W4 /permissive-` とし、警告は放置せず解消する
- C++20 modules は使用しない（MSVCでも実用段階に達していない）
- ターゲットを層ごとに分割し、依存方向を一方向に固定する

```
dalmatian_core   育成ロジック・状態遷移・セーブ形式（OpenGL非依存、テスト対象）
dalmatian_gfx    レンダラ・シェーダ管理・スキニング（OpenGLに依存）
dalmatian_app    main、ウィンドウ、シーン結合（実行ファイル）
dalmatian_tests  core のみを対象とする単体テスト
```

依存方向は `app -> gfx -> (なし)`、`app -> core`、`tests -> core` とし、
**core は gfx に依存させない**。

想定ディレクトリ構成:

```
CMakeLists.txt
cmake/
src/core/        育成ロジック
src/gfx/         描画
src/platform/    ウィンドウ・入力・時間
src/app/         main とシーン結合
shaders/
assets/
tests/
third_party/     glad の生成物など
docs/adr/
```

## Consequences

- 「Visual Studio 2022 と CMake を入れて構成・ビルドする」だけで環境が揃う
- core が OpenGL に依存しないため、育成ロジックを描画なしでテストできる。
  これが大規模経験の不足を補う主な安全弁になる
- FetchContent は初回構成時に依存をダウンロードするため、初回ビルドが遅く、
  オフラインでの初回構成ができない
- 依存が増えてビルド時間が問題になった場合は vcpkg（manifest mode）への移行を検討する。
  その際は新しいADRを追加する

## Alternatives considered

- **vcpkg (manifest mode)**: 依存が多い場合は優位だが、選定したライブラリは
  ヘッダオンリか小規模のため（[0006](0006-dependency-libraries.md)）、
  vcpkgの導入手順を増やす価値が薄い
- **Visual Studio のソリューションファイルを直接管理**: GUI依存になり、
  構成差分がレビューしづらい
- **依存ライブラリをリポジトリに直接コミット（vendoring）**: バージョン更新が煩雑。
  ただし glad の生成物のみは例外として `third_party/` にコミットする
