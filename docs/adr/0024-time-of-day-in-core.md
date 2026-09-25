# 0024. 一日の中の時刻は core が状態として持ち、app が光に変換する

- Status: Accepted
- Date: 2026-09-25

## Context

[0008](0008-time-progression-hybrid.md) により、昼夜は現実時間と同期する。昼夜は描画の光
（`gfx` の `lighting`）に使うほか、犬が夜に眠くなるなど育成ロジックにも使いたい。
時刻を誰が計算するかを決める必要がある。

## Decision

- `core` は注入された時計から **一日の中の時刻** を求め、状態として持つ
- `app` はそれを [0020](0020-core-state-read-events-returned.md) に従って読み取り、
  `gfx` の `lighting` に渡す

```
時計（注入）→ core : time_of_day を状態に持つ（夜は眠くなる などにも使う）
app  : state().time_of_day を読む
gfx  : lighting.set_time_of_day(t)
```

## Consequences

- 時刻の出どころが時計の注入の1か所になり、描画と育成ロジックで時刻がずれない
- デバッグ用の時計で早送りすると（[0025](0025-debug-time-controls.md)）、昼夜も犬の行動も
  そろって進む
- `gfx` は時刻から光への変換だけを担い、時計を知らない

## Alternatives considered

- **`app` が時計から直接計算する**: 単純だが、犬が夜に眠くなるような処理を入れると時刻の
  計算が2か所になり、早送りしたときにずれる
