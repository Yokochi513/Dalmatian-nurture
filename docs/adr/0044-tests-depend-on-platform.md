# 0044. 単体テストの対象に platform を加える

- Status: Accepted
- Date: 2026-09-26

## Context

[0015](0015-physics-target.md) と [0042](0042-platform-depends-on-core.md) の依存方向では、単体テスト
（`dalmatian_tests`）の対象は `core`・`anim`・`physics` だった。`platform` はウィンドウや音声デバイスが
必要なためテストの対象外としていた。

一方、セーブファイルの操作（一時ファイル・`.bak`・`*.broken` の扱い。[0027](0027-save-atomic-write-and-backup.md)）を
`platform` に置くことにした。これは誤ると犬を失う重要な手順であり、一時ディレクトリを使えばウィンドウなしで
テストできる。

## Decision

単体テストの対象に `platform` を加える。

```
app -> core
app -> gfx -> anim
app -> platform -> core
app -> physics
tests -> core, anim, physics, platform
```

- テストは `tests/platform/` に置く
- ウィンドウ・入力・音声は単体テストせず、ゲームを起動して確かめる。テストするのはセーブファイルの操作や
  パスの組み立てなど、OS の資源を一時ディレクトリなどで用意できる部分に限る

本ADRは [0042](0042-platform-depends-on-core.md) の依存方向の部分を置き換える。

## Consequences

- セーブファイルの操作を単体テストで確かめられる
- テストの実行ファイルが GLFW と miniaudio をリンクするようになる（テストの中でウィンドウや音声は作らない）
- CI などウィンドウのない環境でも、テストするのはウィンドウを使わない部分なので実行できる

## Alternatives considered

- **セーブファイルの操作を `core` に置く**: テストの対象のままにできるが、`core` がファイルシステムを扱うことになり、
  [0027](0027-save-atomic-write-and-backup.md) の「ファイル操作は `core` の外」に反する
- **テストしない**: 依存は増えないが、犬を失う手順を起動して手で確かめることになる
