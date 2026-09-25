# 0039. 当たり判定の形状と地点は Blender で背景の .glb に配置し、名前で区別する

- Status: Accepted
- Date: 2026-09-25

## Context

[0014](0014-physics-jolt.md) で当たり判定用の形状の定義場所を、[0019](0019-destination-kind-resolved-by-app.md)
で地点表の定義場所を未決とした。[0029](0029-save-dog-state-only.md) により、起動時は家の決まった場所から
始めるため、開始地点も必要になる。

## Decision

背景の .glb（例：`assets/models/home.glb`）の中に、次を Blender で配置し、**名前の先頭** で区別する。

- `col_` で始まるもの（`col_box_*`、`col_mesh_*` など）：当たり判定用の簡単な形状。描画しない
- `anchor_` で始まる空のオブジェクト：地点。名前が行き先の種類（[0037](0037-behavior-intents.md)）や
  用途に対応する

```
assets/models/home.glb
  sofa            ← 描画する
  col_box_sofa    ← 当たり判定（描画しない）
  col_mesh_floor
  anchor_bowl     ← 地点（空のオブジェクト）
  anchor_bed
  anchor_spawn    ← 飼い主の開始地点
  anchor_door     ← 散歩への出入口
```

## Consequences

- 見た目・当たり判定・地点が1つのファイルにそろい、家具を動かしてもずれない
- 当たり判定用の形状は描画用のメッシュより単純にでき、判定が軽く挙動も安定する
- .glb からメッシュだけでなく、ノードの名前と位置も読む必要がある。`anim` の glTF の読み込み
  （[0009](0009-module-structure.md)）をスキンのない背景にも使えるよう広げるか、別の読み込みを
  用意するかは実装時に決める
- 名前の付け方が Blender での作業の約束事になる。起動時に必要な地点（`anchor_spawn` など）が
  あるかを検査する

## Alternatives considered

- **別のデータファイル（JSON）に座標で書く**: Blender を開かずに直せるが、家具を動かすたびに
  座標を手で合わせる必要がある
- **描画用のメッシュをそのまま当たり判定に使う**: 形状を作る手間はないが、細かいメッシュで判定が
  重くなり、椅子の足の間を通れるなど挙動も不安定になる
