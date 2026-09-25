# 0038. 行動意図とクリップの対応表は app のコードに書き、起動時にクリップの存在を検査する

- Status: Accepted
- Date: 2026-09-25

## Context

[0009](0009-module-structure.md) により、行動意図をアニメーションと移動に変換するのは `app` の
`dog_actor` である。[0037](0037-behavior-intents.md) の行動意図には、「歩く → 伏せる → 寝ている」
のように複数のクリップの流れになるものがある。対応表をどこで持ち、.glb の中のクリップ名と
どうそろえるかを決める必要がある。

## Decision

- 対応表は `app` の `dog_actor` に **コードとして** 書く
- 起動時に、対応表にあるクリップ名がすべて犬の .glb にあるかを検査し、足りなければエラーで止める
- .glb の中のクリップ名は次のとおりとし、Blender 側でそろえる

```
idle, walk, run, sit, lie_down, sleep, stand_up,
stretch, eat, wag, beg, play_bow, sniff, dig, bark,
trick_sit, trick_paw, trick_down, trick_stay, trick_spin
```

対応表の例：

```
Sleep   → walk → lie_down → sleep（ループ）
Stretch → stand_up → stretch
Eat     → walk → eat（ループ）
Greet   → run → wag
```

## Consequences

- 対応表に型の検査が効き、ファイルも増えない
- クリップ名の食い違いは起動直後に分かる
- 対応表を変えるたびにビルドし直す必要がある
- クリップ名が .glb の仕様になり、既成アセットを使う場合も Blender でこの名前に付け直す
  （[0007](0007-asset-pipeline-gltf-blender.md)）
- クリップは20本になる。優先度を付け、最初は idle・walk・eat などから作る

## Alternatives considered

- **データファイル（JSON）に書く**: ビルドし直さずに調整できるが、読み込みと検査のコードが増え、
  書き間違いは実行するまで分からない
