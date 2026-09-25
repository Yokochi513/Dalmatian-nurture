# core / 世話（care）

ソース：`src/core/care.hpp`、`src/core/care.cpp`　概要：[core.md](../core.md)

世話を実行できるかの判定と、受け付けたときの効果。クールダウン、1日の上限、散歩の開始を扱う。

## 範囲

本書で扱う世話は **ごはん（Feed）・なでる（Pet）・遊ぶ（Play）・散歩（Walk）**。
次は各部品の設計で追加する。

| 追加するもの | 設計書 |
|---|---|
| しつける（Train）・芸をさせる（PerformTrick） | tricks.md |
| 受け付けたときの成長ポイントを足す処理（`Simulation` が行う） | [growth.md](growth.md) |
| 「寝ているので不可」、受け付けたときの行動意図の切り替え（ADR 0016） | behavior.md |

## 世話と欲求

| 世話 | 対応する欲求 | 効果 |
|---|---|---|
| Feed | 空腹 | 受け付けた時点で `relief` だけ減る（ADR 0016） |
| Pet | 寂しさ | 同上 |
| Play | 退屈 | 同上 |
| Walk | 運動 | 受け付けると散歩中になり、散歩中の時間に応じて減る（ADR 0018） |

受け付けたときは、どの世話でもなつき度が `affection_gain` だけ上がる。
物と世話の対応（餌皿 → ごはん など）は `app` が持つ（ADR 0017）。

## 実行できるかの判定

`check_care` は次の順に調べ、最初に当てはまった理由を返す（ADR 0017）。

| 順 | 理由（`CareBlock`） | 条件 | 残り時間 |
|---|---|---|---|
| 1 | `AlreadyWalking` | 散歩中に散歩を始めようとした | なし |
| 2 | `DailyLimit` | 今日受け付けた回数が `daily_limit` に達した | 次の現地の0時まで |
| 3 | `Cooldown` | 最後に受け付けてから `cooldown` が経っていない | クールダウンの残り |
| 4 | `NotNeeded` | 対応する欲求が `min_need` 未満（満たされている） | なし |

- 表示する文言（「あと15分」「今日はもう満足」「おなかいっぱい」など）への変換は `app` が行う
- 1日の上限は、クールダウンより長く待つ必要があるため先に判定する
- 時計が戻って最終時刻が未来になっても、クールダウンの残り時間は `cooldown` を超えない
- `NotNeeded` は、欲求が低いのに世話を繰り返して成長ポイントを稼ぐことも防ぐ
- 実行時（`apply_care`）も同じ判定を行い、実行できなければ状態を変えずに理由を返す

## 1日の回数の数え方

- 世話ごとに `CareRecord`（最後に受け付けた時刻、今日の回数）を持つ（[dog.md](dog.md)）
- 回数を数えている日付を `DogState::care_day`（現地の日付）に持つ。判定時は、今日と `care_day` が
  違えば回数を 0 とみなす。受け付けたときに日付が変わっていれば、すべての世話の回数を 0 に戻してから数える
- 「1日」の区切りは現地時刻の0時（ADR 0023、[clock.md](clock.md) の `local_day`）

## 散歩

- 散歩の開始は世話の1つ（`Care::Walk`）として判定・記録する。受け付けると `walking = true`
- 散歩中は運動の欲求が `walk_exercise_per_hour` で減る（[needs.md](needs.md)）。0 を下回らないことが効果の上限になる
- 家に戻ったら `app` が `Simulation::end_walk()` を呼ぶ
- 散歩中に終了していた場合は、`Simulation::resume()` で散歩を終える（ADR 0029）

## 関数

```cpp
enum class CareBlock { None, AlreadyWalking, DailyLimit, Cooldown, NotNeeded };

struct CareAvailability {
    CareBlock block = CareBlock::None;
    std::chrono::seconds remaining{0};  // DailyLimit・Cooldown のみ
    bool available() const;
};

const CareTuning& tuning_of(const Tuning&, Care);
CareAvailability check_care(const DogState&, Care, const ClockReading& now, const Tuning&);
CareAvailability apply_care(DogState&, Care, const ClockReading& now, const Tuning&);
```

`Simulation` からの呼び出し方は [simulation.md](simulation.md)。

## Tuning

世話ごとの数値（`CareTuning`）。既定値は仮の値で、バランス調整の中で決める。

| 世話 | `cooldown` | `daily_limit` | `min_need` | `relief` | `affection_gain` | `growth_points` |
|---|---|---|---|---|---|---|
| `feed` | 3時間 | 3 | 30 | 70 | 1 | 10 |
| `pet` | 10分 | 10 | 10 | 40 | 2 | 10 |
| `play` | 30分 | 6 | 20 | 50 | 2 | 10 |
| `walk` | 2時間 | 3 | 30 | 0（時間で減る） | 3 | 10 |

仮の数値では、最初の10分でできる世話はごはん・なでる・遊ぶ・散歩の各1回程度になる。
`growth_points` の配分は [growth.md](growth.md)（子犬 → 若犬は初回の10分以内、ADR 0036）。

## 検討して採らなかった案

- **欲求が満たされていても世話を受け付ける**: 本物の犬らしいが、世話を繰り返して成長ポイントを稼げる
- **1日の上限に達しても受け付け、効果だけなくす**: メニューで選べるのに何も起きず、ADR 0017 の
  「押しても何も起きない状態を避ける」に反する
