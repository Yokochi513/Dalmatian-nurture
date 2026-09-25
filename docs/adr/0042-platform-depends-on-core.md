# 0042. platform は core に依存し、core のインターフェースの実装（時計など）を提供する

- Status: Accepted
- Date: 2026-09-26

## Context

[0009](0009-module-structure.md) の本文で「時計のインターフェースは `core` に置き、OSの時計を使う実装は
`platform` に置く」と決めた。しかし 0009 とそれを置き換えた [0015](0015-physics-target.md) の依存方向には
`platform -> core` がなく、本文と依存関係の図が食い違っていた。`core` の時計を実装する際に、この食い違いが
問題になった。

## Decision

依存方向に **`platform -> core`** を加える。`platform` は `core` が定めるインターフェース（時計など）を実装する。

```
app -> core
app -> gfx -> anim
app -> platform -> core
app -> physics
tests -> core, anim, physics
```

本ADRは [0015](0015-physics-target.md) の依存方向の部分を置き換える。

## Consequences

- 0009 の本文どおり、本番用の時計（`platform::SystemClock`）を `platform` に置ける
- `core` は OS を知らないまま、OS に依存する実装を外から受け取れる
- `platform` の変更で `core` を再ビルドする必要はないが、`core` のインターフェースを変えると `platform` も
  再ビルドされる
- `core` から `platform` への依存は引き続き禁止する

## Alternatives considered

- **時計の実装を `app` に置く**: 依存方向は変わらないが、0009 の本文と異なり、OS に依存するコードが
  `platform` と `app` に分散する
- **`platform` は OS の時刻を返す関数だけを持ち、`app` で `core` の時計に合わせる**: `platform` は `core` を
  知らずに済むが、時計1つのために変換だけの型が `app` に増える
