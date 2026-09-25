# 0015. dalmatian_physics ターゲットを追加し、Jolt を閉じ込める

- Status: Accepted
- Date: 2026-09-25

## Context

[0014](0014-physics-jolt.md) で Jolt Physics の採用を決めた。Jolt は大きなライブラリであり、
そのヘッダや型が複数のモジュールに広がると、差し替えやビルド時間の面で影響が大きくなる。

[0009](0009-module-structure.md) の6ターゲット構成には当たり判定の置き場所がなく、
ターゲットの追加・依存方向の変更は新しいADRで行うと定めている。

## Decision

ターゲット `dalmatian_physics`（`src/physics/`、名前空間 `dal::physics`）を追加し、
Jolt を直接使うのはこのターゲットだけとする。

- 公開インターフェースには Jolt の型を出さず、glm の型で受け渡す
- `app` からは「移動させる」「視線（射線）を飛ばして当たった対象を返す」などの
  小さなインターフェースだけを使う
- OpenGL に依存させず、単体テストの対象とする

依存方向は次のとおりとし、逆方向の依存を禁止する。

```
app -> core
app -> gfx -> anim
app -> platform
app -> physics
tests -> core, anim, physics
```

テストは `tests/physics/` に置く。

本ADRは [0009](0009-module-structure.md) のうち、ターゲット一覧と依存方向の部分を置き換える。
それ以外（ファイル構成の規約・`core` と `app` の責務分担など）は 0009 のまま有効である。

## Consequences

- Jolt のヘッダが `physics` 以外に広がらず、ビルド時間への影響と差し替えの範囲を限定できる
- 当たり判定を描画なしで単体テストできる
- Jolt の機能を使うたびに、`physics` のインターフェースを広げる必要がある
- ターゲットが7つになり、CMakeの記述量が増える

## Alternatives considered

- **`app` の中で Jolt を直接使う**: ターゲットは増えないが、`app` の責務が大きくなり、
  当たり判定を単体テストできない
