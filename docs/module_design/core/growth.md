# core / 成長（growth）

ソース：`src/core/growth.hpp`、`src/core/growth.cpp`、`src/core/event.hpp`　概要：[core.md](../core.md)

世話でたまる成長ポイントと、成長段階の進み方（ADR 0033・0036）。成長したことを知らせる出来事も扱う。

## 成長ポイント

- 世話を受け付けるたびに、世話ごとの `growth_points`（`CareTuning`）だけ成長ポイントがたまる（ADR 0036）
- 成長ポイントは累計で持ち、減らない
- **1日にたまる成長ポイントには上限 `daily_cap` を設ける**。世話ごとのクールダウンと1日の回数の上限
  （[care.md](care.md)）だけでは、1日にたまる量が世話の組み合わせで大きく変わり、若犬から成犬までの日数を
  調整しにくいため。上限を超えた分は捨てる
- 1日の区切りは現地時刻の0時（ADR 0023）。今日たまった量を数えている日付を `growth_day` に持ち、
  日付が変わっていれば今日の量を 0 に戻してから足す（[care.md](care.md) の回数の数え方と同じ）
- 成長ポイントを足すのは `Simulation` の役目とする。`care` は成長を知らない（[simulation.md](simulation.md)）

## 成長段階

累計の成長ポイントがしきい値に達すると、次の段階になる。

| 段階 | しきい値（累計） |
|---|---|
| 子犬（Puppy） | 0 |
| 若犬（Young） | `young_at` |
| 成犬（Adult） | `adult_at` |

- 成長したときは出来事 `Grew`（新しい段階つき）を返す（ADR 0036）
- 1回で2段階を超えた場合も、段階ごとに `Grew` を返す（仮の数値では起きない）
- 段階が上がっても成長ポイントは 0 に戻さない（累計のまま）

### ペースの目標と仮の数値（ADR 0036）

- **子犬 → 若犬：初回のプレイの10分以内**。最初の10分でできる世話は、ごはん・なでる・遊ぶ・散歩の各1回程度
  （[care.md](care.md)）。1回 10 ポイントとし、`young_at = 30`（3回の世話）にする
- **若犬 → 成犬：数日**。`daily_cap = 150`、`adult_at = 450` とすると、1日目の終わりに最大 150、
  3日目に成犬になる

## 出来事

出来事は `core` を進める関数の戻り値のリストとして返す（ADR 0020）。本書で最初の出来事を定義する。

```cpp
enum class EventKind {
    Grew,          // 成長した。stage に新しい段階
    LearnedTrick,  // 芸を習得した。trick に芸（tricks.md）
};

struct Event {
    EventKind kind;
    GrowthStage stage = GrowthStage::Puppy;  // Grew のとき
    Trick trick = Trick::Sit;                // LearnedTrick のとき
};
```

## 関数

```cpp
// 成長ポイントを足す（1日の上限つき）。成長したら Grew を返す
std::vector<Event> add_growth(DogState& state, double points, std::chrono::local_days today, const Tuning& tuning);

// 累計の成長ポイントに対応する段階
GrowthStage stage_for(double growth_points, const Tuning& tuning);

// 次の段階までの進み具合（0〜1）。成犬なら 1。デバッグUIやゲージの表示用
double growth_progress(const DogState& state, const Tuning& tuning);
```

## DogState に加えるもの

```cpp
double growth_points = 0.0;            // 累計
double growth_today = 0.0;             // growth_day の日にたまった量
std::chrono::local_days growth_day{};  // growth_today を数えている現地の日付
```

## Tuning

既定値は仮の値で、バランス調整の中で決める。

| 項目 | 仮の既定値 | 意味 |
|---|---|---|
| `growth.young_at` | 30 | 若犬になる累計の成長ポイント |
| `growth.adult_at` | 450 | 成犬になる累計の成長ポイント |
| `growth.daily_cap` | 150 | 1日にたまる成長ポイントの上限 |
| `CareTuning::growth_points` | 各世話 10 | 世話を受け付けたときにたまる量（[care.md](care.md)） |

## 検討して採らなかった案

- **1日の上限を設けず、世話ごとの回数の上限だけで抑える**: 仕組みは少ないが、1日にたまる量が世話の
  組み合わせで変わり、成犬までの日数を調整しにくい
- **段階が上がるたびに成長ポイントを 0 に戻す**: 段階ごとの必要量が分かりやすいが、累計で見たときの
  進み具合が分からなくなり、セーブの値の意味も段階に依存する
- **`care` の中で成長ポイントを足す**: 呼び出しは1回で済むが、`care` が成長を知ることになり、部品の
  テストが分けにくくなる
