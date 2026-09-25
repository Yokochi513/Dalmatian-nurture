# core / 芸（tricks）

ソース：`src/core/tricks.hpp`、`src/core/tricks.cpp`　概要：[core.md](../core.md)

芸の一覧と解禁、習熟度と習得、「しつける」「芸をさせる」の判定と効果（ADR 0034・0035）。

## 芸の一覧と解禁（ADR 0034）

```cpp
enum class Trick { Sit, Paw, Down, Stay, Spin };
inline constexpr std::size_t kTrickCount = 5;
```

| 芸 | 名前 | 練習できる段階 |
|---|---|---|
| `Sit` | おすわり | 子犬から |
| `Paw` | お手 | 子犬から |
| `Down` | ふせ | 若犬から |
| `Stay` | まて | 若犬から |
| `Spin` | ぐるぐる回る | 成犬から |

- 解禁の段階はコードの表で持つ（バランスの数値ではないため `Tuning` に入れない）
- 表示する名前への変換は `app` が行う

## 習熟度と習得（ADR 0035）

- 芸ごとに習熟度（0〜100）を持つ。**100 で習得**とし、習得済みかどうかは別に持たない
- 「しつける」を受け付けると、選んだ芸の習熟度が次の量だけ上がる。**乱数は使わない**

```
上がり幅 = train_base × (0.5 + 0.5 × (機嫌 + なつき度) / 200)
```

- 係数は 0.5（機嫌・なつき度とも 0）〜 1.0（ともに 100）。機嫌となつき度を保つほど早く覚える
- 仮の数値 `train_base = 25` では、1つの芸を覚えるのに 4〜8 回の練習が要る
- 機嫌となつき度は、しつけの効果（なつき度の上昇など）を反映する **前** の値を使う
- 100 に達したら出来事 `LearnedTrick`（芸つき）を返す

## しつける・芸をさせる

`Care` に `Train`（しつける）と `PerformTrick`（芸をさせる）を加える。
クールダウン・1日の上限・記録（`CareRecord`）は、ほかの世話と同じ仕組みを使う（[care.md](care.md)）。
クールダウンと1日の上限は **世話の種類ごと** で、芸ごとには持たない。

| 世話 | 対象の欲求 | 効果 |
|---|---|---|
| `Train` | 退屈 | 退屈が `relief` だけ減り、選んだ芸の習熟度が上がる |
| `PerformTrick` | 退屈 | 退屈が `relief` だけ減り、なつき度が `affection_gain` だけ上がる（ADR 0035） |

- どちらも `min_need = 0` とし、「満たされている」では断らない
- 受け付けたら、ほかの世話と同じく成長ポイントがたまる（[growth.md](growth.md)）

### 芸ごとの判定

芸を指定した判定は、次の順に調べる。芸ごとの理由は待っても解消しないため、クールダウンなどより先に判定する。

| 順 | 理由（`CareBlock`） | 条件 |
|---|---|---|
| 1 | `TrickLocked` | 今の成長段階ではまだ練習できない（`Train`・`PerformTrick` とも） |
| 2 | `TrickLearned` | すでに習得している（`Train` のみ） |
| 2 | `TrickNotLearned` | まだ習得していない（`PerformTrick` のみ） |
| 3〜 | 世話の判定 | `DailyLimit`・`Cooldown`・`NotNeeded`（[care.md](care.md)） |

- 芸を指定しない `check_care(Train)` は、世話の判定（上限・クールダウン）だけを行う。メニューの
  「しつける」「芸をさせる」の項目を薄く表示するかどうかに使う
- `do_care(Train)` のように芸を指定せずに実行しようとした場合は `TrickNotSpecified` を返す（呼び出し側の誤り）

## メニューの組み立て（app 向け）

ADR 0012 のメニューで、`app` は次のように使う想定。

```
犬 → E
  なでる / 遊ぶ
  しつける ▶ [おすわり 60/100] [お手 20/100]        ← 解禁済みの芸（習得済みは薄く）
  芸をさせる ▶ [おすわり]                           ← 習得済みの芸
```

- `unlocked_tricks(stage)` で解禁済みの芸を得る
- 習熟度は `DogState::trick_proficiency` から読む

## 関数

```cpp
bool is_unlocked(Trick trick, GrowthStage stage);
std::vector<Trick> unlocked_tricks(GrowthStage stage);
bool is_learned(const DogState& state, Trick trick);
double& proficiency_of(DogState& state, Trick trick);
double proficiency_of(const DogState& state, Trick trick);

// 芸の練習の上がり幅
double training_gain(const DogState& state, const Tuning& tuning);

// 芸を指定した判定（芸ごとの判定 → 世話の判定）。care は Train か PerformTrick
CareAvailability check_trick(const DogState& state, Care care, Trick trick, const ClockReading& now, const Tuning& tuning);

// 芸を指定して実行する。受け付けたら世話の効果（apply_care）に加えて、Train なら習熟度を上げる。
// 習得したら LearnedTrick を返す
TrickResult apply_trick(DogState& state, Care care, Trick trick, const ClockReading& now, const Tuning& tuning);

struct TrickResult {
    CareAvailability availability;
    std::vector<Event> events;  // LearnedTrick
};
```

`Simulation` の入口は [simulation.md](simulation.md)。

## DogState に加えるもの

```cpp
std::array<double, kTrickCount> trick_proficiency{};  // Trick の順。0〜100、100 で習得
```

## 出来事に加えるもの

```cpp
enum class EventKind { Grew, LearnedTrick };

struct Event {
    EventKind kind;
    GrowthStage stage = GrowthStage::Puppy;  // Grew のとき
    Trick trick = Trick::Sit;                // LearnedTrick のとき
};
```

## Tuning

既定値は仮の値で、バランス調整の中で決める。

| 項目 | 仮の既定値 | 意味 |
|---|---|---|
| `train_base` | 25 | 練習1回の習熟度の上がり幅の基本値 |

世話ごとの数値（`CareTuning`、[care.md](care.md) の表に追加）。

| 世話 | `cooldown` | `daily_limit` | `min_need` | `relief` | `affection_gain` | `growth_points` |
|---|---|---|---|---|---|---|
| `train` | 15分 | 8 | 0 | 10 | 1 | 10 |
| `perform_trick` | 5分 | 10 | 0 | 10 | 3 | 5 |

## 検討して採らなかった案

- **しつける・芸をさせるを世話とは別の仕組みにする**: 芸の扱いは分かりやすいが、クールダウンと1日の上限の
  仕組みを二重に持つことになる
- **クールダウンを芸ごとに持つ**: 芸を替えれば続けて練習でき、1日にたまる成長ポイントや習熟度を調整しにくい
- **世話の判定を芸ごとの判定より先にする**: 解禁されていない芸に「あと15分」と表示され、待っても練習できない
- **習熟度の上がり幅を機嫌だけで決める**: 単純だが、なつき度を上げる意味が芸に表れない（ADR 0035）
