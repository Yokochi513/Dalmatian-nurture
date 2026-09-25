# core / 欲求（needs）

ソース：`src/core/needs.hpp`、`src/core/needs.cpp`　概要：[core.md](../core.md)

欲求の時間変化、なつき度の低下、閉じていた間の一括計算、機嫌の計算。

## 関数

```cpp
enum class Activity { Awake, Sleeping, Walking };
using Duration = std::chrono::duration<double>;

void advance_needs(Needs&, Activity, Duration elapsed, const Tuning&);                 // 起動中の変化
void apply_neglect(double& affection, const Needs&, Duration elapsed, const Tuning&);  // 起動中のなつき度の低下
void apply_absence(DogState&, Duration elapsed, const Tuning&);                        // 不在中の一括計算
double max_need(const Needs&);
double mood(const Needs&);
```

1秒刻みごとに `advance_needs` → `apply_neglect` の順に呼ぶ（なつき度の判定には更新後の欲求を使う）。
`Activity` は `Simulation` が決める。散歩中（`DogState::walking`）なら `Walking`、行動意図が Sleep なら
`Sleeping`（[behavior.md](behavior.md)）、それ以外は `Awake`。

## 起動中の変化（1秒刻みごと）

| 欲求 | 起きている間 | 寝ている間（Sleep） | 散歩中 |
|---|---|---|---|
| 空腹 | 増える | 増える | 増える |
| 運動 | 増える | 増える | **減る**（ADR 0018） |
| 退屈 | 増える | 増える | 増える |
| 寂しさ | 増える | 増える | 増える |
| 眠気 | 増える | **減る** | 増える |

- 変化率は欲求ごとに `Tuning` に持つ（1時間あたりの量）。変化率は昼夜を問わず一定（ADR 0032）
- 起動中は 0〜100 の範囲で変化する
- ごはん・遊ぶ・なでるによる回復は [care.md](care.md)

## なつき度の低下

- いずれかの欲求が `neglect_threshold` 以上の間、一定の割合で下がる（ADR 0030）
- 起動中も不在中も同じ規則とする

## 不在中の一括計算（ADR 0022）

閉じていた間の経過 `elapsed` から、次を一度に計算する。

- **眠気以外の4つ**：`min(need + rate × elapsed, max(need, offline_need_cap))`。
  閉じている間は `offline_need_cap` を超えて増えない。すでに超えていればそのまま
- **眠気**：留守中は寝ていたものとして、`max(sleepiness − sleep_recovery × elapsed, 0)` で減る
- **なつき度**：各欲求がしきい値に達する時刻は一定の変化率から計算できるため、「いずれかの欲求が
  しきい値以上だった時間」を求め、その時間に応じて下げる。1回の不在での低下は
  `offline_affection_loss_max` まで。不在前の欲求から計算するため、欲求より先に更新する
  - 増える4つの欲求：最初にしきい値に達した時刻 `b` から後はずっとしきい値以上（上限がしきい値未満なら達しない）
  - 減る眠気：最初からしきい値以上なら、下回る時刻 `a` までの間
  - しきい値以上だった時間 ＝ `[0, a)` と `[b, elapsed)` の和集合の長さ（重なりは二重に数えない）
- 行動意図は進めず、復帰後に決め直す

1秒刻みを同じ時間だけ繰り返した結果と、不在の一括計算の結果は一般には一致しない（不在中は上限と眠気の
扱いが異なるため）。単体テストでは、上限に届かない範囲で眠気以外の欲求が一致することを確かめる。

## 機嫌

```
mood = 100 − （5つの欲求の平均）
```

計算式は調整する可能性があるため、`mood()` の中に閉じ込める。

## Tuning

既定値は仮の値で、バランス調整の中で決める。

| 項目 | 仮の既定値 | 意味 |
|---|---|---|
| `hunger_per_hour` | 12.5 | 空腹の増え方（8時間で 0 → 100） |
| `exercise_per_hour` | 8.0 | 運動の増え方 |
| `boredom_per_hour` | 16.0 | 退屈の増え方 |
| `loneliness_per_hour` | 10.0 | 寂しさの増え方 |
| `sleepiness_per_hour` | 6.0 | 眠気の増え方 |
| `sleep_recovery_per_hour` | 12.5 | Sleep 中・不在中の眠気の減り |
| `walk_exercise_per_hour` | 180.0 | 散歩中の運動の減り（20分で 60） |
| `neglect_threshold` | 80.0 | なつき度が下がり始める欲求の値 |
| `affection_loss_per_hour` | 1.0 | なつき度の下がり方 |
| `offline_need_cap` | 80.0 | 不在中に欲求が届く上限 |
| `offline_affection_loss_max` | 10.0 | 1回の不在でのなつき度の低下の上限 |

## 検討して採らなかった案

- **不在中の上限を「増える量の上限」にする**: 閉じたときの値が高いと 100 に届いてしまう
- **なつき度を不在の時間に比例して下げる**: 単純だが、起動したまま世話をしなくても下がらない
- **不在中も眠気を増やす**: 規則はそろうが、長く閉じた後はいつも眠い状態で始まる
