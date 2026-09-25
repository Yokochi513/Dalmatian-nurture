# 0037. 行動意図を15個、行き先の種類を5個とする

- Status: Accepted
- Date: 2026-09-25

## Context

[0009](0009-module-structure.md) により、`core` は犬の行動意図を決める。
[0019](0019-destination-kind-resolved-by-app.md) により行動意図は行き先の種類を持つ。
ここまでの決定（世話・散歩・芸・眠気）から、必要な行動意図と行き先の種類を決める必要がある。

## Decision

行き先の種類を次の5つとする。`Owner` の座標は飼い主の位置として `app` が決める。

- `Here`（その場）、`Any`（どこか）、`Bowl`（餌皿）、`Bed`（寝床）、`Owner`（飼い主）

行動意図を次の15個とする。

| 意図 | 行き先 | きっかけ |
|---|---|---|
| Idle | Here | 欲求が低い |
| Wander | Any | 退屈が中くらい |
| Sleep | Bed | 眠気がしきい値以上（夜は低い。[0032](0032-sleepiness-constant-rate.md)） |
| Stretch | Here | Sleep から起きた直後 |
| Beg | Owner | いずれかの欲求が高い。何が欲しいか（最も高い欲求）を持つ |
| Bark | Here | Beg を続けても一定時間応えてもらえない |
| Greet | Owner | 起動時の「おかえり」、散歩から家に戻ったとき |
| Eat | Bowl | ごはん |
| Petted | Owner | なでる |
| Play | Owner | 遊ぶ |
| Train | Owner | しつける（練習する芸を持つ） |
| PerformTrick | Owner | 芸をさせる（披露する芸を持つ） |
| Follow | Owner | 散歩中 |
| Sniff | Any | 散歩中、ときどき |
| Dig | Any | 散歩中、退屈が高いとき |

## Consequences

- 犬が自分から欲求を訴える（Beg・Bark）、飼い主を迎える（Greet）といった生き物らしい振る舞いを表せる
- きっかけはすべて欲求・時間・`core` の状態で決まり、`core` は空間を知らないままで済む
  （Bark は「飼い主が遠い」ではなく「応えてもらえない時間」で判定する）
- 散歩中（Follow・Sniff・Dig）と家（Sleep・Eat など）で使える行動意図が分かれる。
  散歩中に家の行き先（Bed・Bowl）が選ばれないことを単体テストで確かめる
- 「ときどき」を乱数で決めるか、周期で決めるかは `behavior` の設計で決める。
  乱数を使う場合は外から注入してテストで固定する
- Bark は鳴き声の音声、Greet・Petted などは尻尾を振るアニメーションが必要になる
  （[0038](0038-clip-table-in-app.md)）

## Alternatives considered

- **最小限に絞る（Idle・Wander・Sleep・Care・Follow）**: 単純だが、犬が自分から何かを訴える場面がなくなる
- **Bark・Dig・Greet・Stretch を加えない11個**: アニメーションは少ないが、生き物らしさが弱い
