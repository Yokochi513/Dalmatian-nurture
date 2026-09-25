# core / 犬の状態（dog）

ソース：`src/core/dog.hpp`、`src/core/dog.cpp`　概要：[core.md](../core.md)

犬の状態を表す構造体と、関連する列挙。部品の設計に合わせてメンバーを追加する。
状態を書き換えてよいのは `Simulation` だけ（[core.md](../core.md) の設計の方針）。

## 決めたこと

```cpp
namespace dal::core {

// 欲求。0〜100 で、大きいほど強く欲しがっている（ADR 0031）
struct Needs {
    double hunger = 0.0;      // 空腹   ← ごはん
    double exercise = 0.0;    // 運動   ← 散歩
    double boredom = 0.0;     // 退屈   ← 遊ぶ
    double loneliness = 0.0;  // 寂しさ ← なでる
    double sleepiness = 0.0;  // 眠気   ← 犬が自分で寝る
};

enum class GrowthStage { Puppy, Young, Adult };

enum class Care { Feed, Pet, Play, Walk, Train, PerformTrick };  // care.md・tricks.md
enum class Trick { Sit, Paw, Down, Stay, Spin };                 // tricks.md

struct CareRecord {
    std::optional<TimePoint> last_done;  // 最後に受け付けた時刻（UTC）
    int count_today = 0;                 // care_day の日に受け付けた回数
};

struct DogState {
    std::string name;
    Needs needs;
    double affection = 0.0;  // なつき度（0〜100）
    GrowthStage stage = GrowthStage::Puppy;

    std::array<CareRecord, kCareCount> care_records{};  // Care の順（care.md）
    std::chrono::local_days care_day{};  // count_today を数えている現地の日付（care.md）
    bool walking = false;                // 散歩中か（ADR 0018、care.md）

    double growth_points = 0.0;            // 累計の成長ポイント（growth.md）
    double growth_today = 0.0;             // growth_day の日にたまった量（growth.md）
    std::chrono::local_days growth_day{};  // growth_today を数えている現地の日付（growth.md）

    std::array<double, kTrickCount> trick_proficiency{};  // Trick の順。100 で習得（tricks.md）

    Intent intent;  // 今の行動意図（behavior.md。IntentKind・Destination・NeedKind も behavior.md）
};

DogState new_dog(std::string name);  // 初回に迎えた子犬の状態
CareRecord& record_of(DogState&, Care);
const CareRecord& record_of(const DogState&, Care);

} // namespace dal::core
```

- 欲求となつき度は小数の 0〜100。欲求は大きいほど強く欲しがっている（ADR 0031）
- 値を更新したら必ず 0〜100 に切り詰める
- **機嫌は保存しない**。`mood(const Needs&)` で毎回計算する（ADR 0030、[needs.md](needs.md)）

## 初回の子犬（new_dog）

初回に名前を付けて迎えた子犬（ADR 0011）の状態。

| 項目 | 値 |
|---|---|
| 空腹・運動・退屈・寂しさ・眠気 | 60・50・60・60・20 |
| なつき度 | 20 |
| 成長段階 | 子犬 |

欲求をある程度高くしておくのは、欲求が低いと「満たされている」（[care.md](care.md)）で世話を断られ、
初回のプレイで何もできないため。

## 検討して採らなかった案

- **処理ごとにクラスで状態を隠す**（`Needs` クラス、`CareTracker` クラスなど）: 不正な更新を型で防げるが、
  セーブとの変換や読み取りにアクセサが多く必要になる
- **初回の子犬の欲求をすべて 0 にする**: 自然だが、初回のプレイで世話がすべて断られる
