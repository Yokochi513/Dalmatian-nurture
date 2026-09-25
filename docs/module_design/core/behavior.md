# core / 行動意図（behavior）

ソース：`src/core/behavior.hpp`、`src/core/behavior.cpp`　概要：[core.md](../core.md)

犬が次に何をしたいか（行動意図）と、どこへ行きたいか（行き先の種類）を決める（ADR 0009・0019・0037）。
行動意図をアニメーションと移動に変えるのは `app` の役目。

## 行動意図と行き先の種類（ADR 0037）

```cpp
enum class IntentKind {
    Idle, Wander, Sleep, Stretch, Beg, Bark, Greet,   // 自律・家
    Eat, Petted, Play, Train, PerformTrick,           // 世話への反応
    Follow, Sniff, Dig,                               // 散歩中
};

enum class Destination { Here, Any, Bowl, Bed, Owner };

enum class NeedKind { Hunger, Exercise, Boredom, Loneliness, Sleepiness };

struct Intent {
    IntentKind kind = IntentKind::Idle;
    Destination destination = Destination::Here;
    NeedKind need = NeedKind::Hunger;  // Beg のとき：何が欲しいか
    Trick trick = Trick::Sit;          // Train・PerformTrick のとき
    TimePoint since{};                 // この行動意図になった時刻（UTC）
    std::uint64_t id = 0;              // 行動意図が変わるたびに増える通し番号
};
```

| 意図 | 行き先 | 種類 | 始まるとき | 終わるとき |
|---|---|---|---|---|
| Idle | Here | 状態 | ほかに当てはまらない | 規則で別の意図が選ばれたとき |
| Wander | Any | 動作 | Idle が `idle_before_wander` 続き、退屈が `wander_threshold` 以上 | 終了通知 |
| Sleep | Bed | 状態 | 眠気が寝るしきい値以上（昼 `sleep_threshold_day`、夜 `sleep_threshold_night`。ADR 0032） | 眠気が `wake_threshold` 以下 → Stretch |
| Stretch | Here | 動作 | Sleep から起きたとき | 終了通知 |
| Beg | Owner | 状態 | 眠気以外で最も高い欲求が `beg_threshold` 以上。`need` にその欲求 | 規則で別の意図が選ばれたとき、または Bark へ |
| Bark | Here | 動作 | Beg が `bark_after` 続いた（応えてもらえない） | 終了通知 → 規則で決め直す（Beg なら時間を数え直す） |
| Greet | Owner | 動作 | 起動時（`resume`）、散歩から家に戻ったとき（`end_walk`） | 終了通知 |
| Eat | Bowl | 動作 | ごはんを受け付けた | 終了通知 |
| Petted | Owner | 動作 | なでるを受け付けた | 終了通知 |
| Play | Owner | 動作 | 遊ぶを受け付けた | 終了通知 |
| Train | Owner | 動作 | しつけるを受け付けた。`trick` に芸 | 終了通知 |
| PerformTrick | Owner | 動作 | 芸をさせるを受け付けた。`trick` に芸 | 終了通知 |
| Follow | Owner | 状態 | 散歩を受け付けた、散歩中に動作が終わった | 規則で Sniff か Dig へ、散歩の終了 |
| Sniff | Any | 動作 | 散歩中、Follow が `sniff_interval` 続き、退屈が `dig_threshold` 未満 | 終了通知 |
| Dig | Any | 動作 | 散歩中、Follow が `sniff_interval` 続き、退屈が `dig_threshold` 以上 | 終了通知 |

`Owner` の座標は飼い主の位置として `app` が決める（ADR 0019）。

## 状態の意図と動作の意図

- **状態の意図**（Idle・Sleep・Beg・Follow）：続いている間、`core` が1秒刻みごとに規則を見直し、
  別の意図が選ばれたら切り替える
- **動作の意図**（それ以外）：`app` が演出（移動・アニメーション）を終えたら `finish_intent(id)` で知らせ、
  `core` が規則で次の意図を決める（ADR 0016）
- 終了の通知が来ない場合の保険として、動作の意図が `max_action_duration` 続いたら終わったものとみなす（ADR 0016）
- `finish_intent(id)` の `id` が今の意図と違えば無視する。演出の途中で世話によって意図が切り替わった場合に、
  古い演出の終了で新しい意図を終わらせないため
- 状態の意図に対する `finish_intent` も無視する

## 規則（次の意図の決め方）

乱数は使わない。「ときどき」は、今の意図が続いた時間（周期）で決める（ADR 0037 の未決事項を周期に決定）。

### 散歩中（`DogState::walking`）

1. 今の意図が Follow で、`sniff_interval` 以上続いていれば、退屈が `dig_threshold` 以上なら **Dig**、未満なら **Sniff**
2. それ以外は **Follow**

散歩中は Sleep・Beg などの家の意図を選ばない（行き先 Bed・Bowl が散歩のシーンにないため。ADR 0037）。

### 家

1. 今の意図が Sleep なら、眠気が `wake_threshold` 以下になるまで **Sleep** を続け、なったら **Stretch**
2. 眠気が寝るしきい値以上なら **Sleep**（夜は `night_start`〜`night_end` の現地時刻。ADR 0024）
3. 眠気以外で最も高い欲求が `beg_threshold` 以上なら **Beg**（`need` にその欲求）。
   今の意図が同じ欲求の Beg で、`bark_after` 以上続いていれば **Bark**
4. 今の意図が Idle で `idle_before_wander` 以上続き、退屈が `wander_threshold` 以上なら **Wander**
5. それ以外は **Idle**

意図の種類（Beg なら欲求も）が今と同じなら切り替えない（`since` と `id` をそのまま）。

## 世話との関係

- 世話を受け付けたら、`Simulation` が世話に対応する動作の意図に切り替える（ADR 0016）。
  散歩の開始は Follow。今の意図は動作の途中でも上書きする
- **Sleep の間は、すべての世話を `Sleeping` で断る**（ADR 0017 の「寝ている」）。
  判定の順は `AlreadyWalking` の次（[care.md](care.md)）
- 寝ている間は、欲求の変化の活動を `Sleeping` にする（眠気が減る。[needs.md](needs.md)）

## 起動時と不在

- 閉じていた間は行動意図を進めず（ADR 0022）、`resume()` で **Greet** にする（「おかえり」）
- 起動中に `offline_gap` 以上空いて不在として扱った場合は、規則で決め直す

## 一日の中の時刻（ADR 0024）

- `core` は時計から一日の中の時刻（現地時刻の時、0〜24 の小数）を求める。`app` はそれを読んで光に使う
- 保存はせず、時計から毎回求める

```cpp
double hours_of_day(const ClockReading& reading);  // clock.hpp
bool is_night(double hours, const Tuning& tuning);  // behavior.hpp
```

## 関数

```cpp
bool is_action(IntentKind kind);  // 動作の意図か

// 規則で次の意図を決める（今の意図が続いた時間も使う）。同じなら今の意図をそのまま返す
Intent decide_intent(const DogState& state, const ClockReading& now, const Tuning& tuning);

// 意図を切り替える（id を増やし、since を now にする）
void set_intent(DogState& state, IntentKind kind, Destination destination, const ClockReading& now);

// 1秒刻みごとに呼ぶ。状態の意図を見直し、長すぎる動作の意図を終わらせる
void update_intent(DogState& state, const ClockReading& now, const Tuning& tuning);

// 動作の終了通知。id が今の意図と同じ動作の意図なら、規則で次の意図を決める
void finish_intent(DogState& state, std::uint64_t id, const ClockReading& now, const Tuning& tuning);

// 世話を受け付けたときの意図
void start_care_intent(DogState& state, Care care, Trick trick, const ClockReading& now);
```

## DogState に加えるもの

```cpp
Intent intent;  // 今の行動意図
```

## Tuning

既定値は仮の値で、バランス調整の中で決める。

| 項目 | 仮の既定値 | 意味 |
|---|---|---|
| `behavior.sleep_threshold_day` | 80 | 昼に寝始める眠気 |
| `behavior.sleep_threshold_night` | 40 | 夜に寝始める眠気 |
| `behavior.night_start` | 21 | 夜の始まり（現地時刻の時） |
| `behavior.night_end` | 6 | 夜の終わり（現地時刻の時） |
| `behavior.wake_threshold` | 5 | 起きる眠気 |
| `behavior.beg_threshold` | 60 | おねだりを始める欲求 |
| `behavior.bark_after` | 2分 | おねだりが続いたら吠えるまでの時間 |
| `behavior.wander_threshold` | 30 | うろうろし始める退屈 |
| `behavior.idle_before_wander` | 20秒 | うろうろする前にじっとしている時間 |
| `behavior.sniff_interval` | 30秒 | 散歩中に匂いを嗅ぐ（または掘る）間隔 |
| `behavior.dig_threshold` | 60 | 散歩中に匂いを嗅ぐ代わりに掘る退屈 |
| `behavior.max_action_duration` | 60秒 | 終了通知が来なくても動作の意図を終わらせる時間 |

## 検討して採らなかった案

- **「ときどき」を乱数で決める**: 動きに変化が出るが、乱数を注入してテストで固定する仕組みが要る。
  動きが単調に見える場合は、乱数の注入を後から検討する
- **終了通知に通し番号を付けない**: 単純だが、演出の途中で世話によって意図が変わると、古い演出の終了で
  新しい意図が終わってしまう
- **寝ている間もなでることはできる**: 起こして遊べるが、「寝ている」という理由（ADR 0017）が生きない
- **すべての意図を `app` の終了通知で終わらせる**: 仕組みは1つになるが、Idle や Sleep のように終わりの
  ない意図を `app` が判断することになる
