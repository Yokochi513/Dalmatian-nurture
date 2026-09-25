# core モジュール設計

`dalmatian_core`（`src/core/`、名前空間 `dal::core`）の設計。
モジュールをまたぐ決定は [docs/adr/](../adr/) にあり、ここではそれを前提に core の内部を決める。

- 状態: 時計・犬の状態・欲求・時間の進め方・世話（ごはん・なでる・遊ぶ・散歩）は設計・実装済み
- 未設計: 成長（growth）、芸（tricks）、行動意図（behavior）、セーブ（save）

## 責務

- 犬の状態を持ち、時間の経過と世話の命令によって更新する
- 犬の行動意図（何をしたいか・どこへ行きたいか）を決める
- セーブデータ（JSON）との相互変換を行う
- 描画・アニメーション・空間・OS を知らない。依存は標準C++ と nlohmann/json のみ（ADR 0009）
- OS に依存する実装（時計など）は `platform` が core のインターフェースを実装して提供する（ADR 0042）

## 設計の方針

- **状態は単純な構造体、処理は関数** とする。`DogState` は公開メンバーだけを持ち、`needs` などは
  状態を受け取って更新する関数として書く。関数ごとに単体テストできる
- **状態を書き換えてよいのは `Simulation` だけ**。`app` には `const DogState&` を渡す（ADR 0020）
- **バランスの数値は `Tuning` 構造体** にまとめ、既定値をメンバーに書く。`Simulation` の作成時に渡す。
  テストは計算しやすい数値を自分で指定するため、バランスを調整してもテストは壊れない
- **乱数を使わない**（ADR 0035）。使う必要が出た場合は外から注入する

## ファイル構成

| ファイル | 内容 | 設計 |
|---|---|---|
| `clock.hpp` | 時計のインターフェース | 本書 |
| `dog.hpp/.cpp` | 犬の状態（`DogState`）と関連する列挙、初回の子犬の状態 | 本書（一部） |
| `tuning.hpp` | バランスの数値（`Tuning`） | 本書（一部） |
| `needs.hpp/.cpp` | 欲求の時間変化、なつき度の低下、不在中の一括計算、機嫌の計算 | 本書 |
| `simulation.hpp/.cpp` | 1秒刻みで進める入口、不在からの復帰、世話の入口 | 本書（一部） |
| `care.hpp/.cpp` | 世話の可否と効果、クールダウン、1日の上限 | 本書（一部） |
| `growth.hpp/.cpp` | 成長ポイントと成長段階 | 未設計 |
| `tricks.hpp/.cpp` | 芸の習熟度と習得 | 未設計 |
| `behavior.hpp/.cpp` | 行動意図の決定 | 未設計 |
| `save.hpp/.cpp` | JSON との相互変換、形式の番号と変換 | 未設計 |

時計の実装は core の外に置く。

| 実装 | 置き場所 | 用途 |
|---|---|---|
| `SystemClock` | `platform/system_clock` | 本番。OS の時計とタイムゾーンを読む |
| `DebugClock` | `app` | 倍速・不在の再現（ADR 0025） |
| `FakeClock` | `tests/core` | テスト。時刻と時差を自由に設定し、進める |

## 時計（clock）

時計は **UTC の時刻** と **その時点の現地との時差** を組にして返す。

```cpp
namespace dal::core {

using TimePoint = std::chrono::sys_time<std::chrono::milliseconds>;

struct ClockReading {
    TimePoint utc;
    std::chrono::minutes utc_offset;  // 日本なら +540
};

class Clock {
public:
    virtual ~Clock() = default;
    virtual ClockReading now() const = 0;
};

} // namespace dal::core
```

- 経過時間の計算とセーブの最終終了時刻には `utc` を使う
- 現地時刻は `local_now(reading)`、現地の日付は `local_day(reading)` で求める
- 「1日」の区切り（ADR 0023）と一日の中の時刻（ADR 0024）には `utc + utc_offset`（現地時刻）を使う
- core は OS のタイムゾーン情報を直接読まない。テストでは時差も自由に設定できる
- `Simulation` は作成時に `const Clock&` を受け取り、`step()` や世話の命令の中で自分で時刻を読む。
  すべての操作が同じ時計を使うことが保証される

```cpp
FakeClock clock{/* utc */, std::chrono::hours{9}};
Simulation sim{clock, state, Tuning{}};
clock.advance(std::chrono::hours{1});
auto events = sim.step();
```

## 犬の状態（dog）

本書で決めた範囲の `DogState`。世話・成長・芸・行動意図の設計に合わせてメンバーを追加する。

```cpp
namespace dal::core {

struct Needs {
    double hunger = 0.0;      // 空腹   ← ごはん
    double exercise = 0.0;    // 運動   ← 散歩
    double boredom = 0.0;     // 退屈   ← 遊ぶ
    double loneliness = 0.0;  // 寂しさ ← なでる
    double sleepiness = 0.0;  // 眠気   ← 犬が自分で寝る
};

enum class GrowthStage { Puppy, Young, Adult };

enum class Care { Feed, Pet, Play, Walk };

struct CareRecord {
    std::optional<TimePoint> last_done;  // 最後に受け付けた時刻（UTC）
    int count_today = 0;                 // care_day の日に受け付けた回数
};

struct DogState {
    std::string name;
    Needs needs;
    double affection = 0.0;  // なつき度
    GrowthStage stage = GrowthStage::Puppy;

    std::array<CareRecord, kCareCount> care_records{};  // Care の順
    std::chrono::local_days care_day{};  // count_today を数えている現地の日付
    bool walking = false;                // 散歩中か（ADR 0018）
    // 以下は各節の設計で追加する：成長ポイント、芸の習熟度、行動意図
};

DogState new_dog(std::string name);  // 初回に迎えた子犬の状態

} // namespace dal::core
```

- 欲求となつき度は小数の 0〜100。欲求は大きいほど強く欲しがっている（ADR 0031）
- 値を更新したら必ず 0〜100 に切り詰める
- **機嫌は保存しない**。`mood(const Needs&)` で毎回計算する（ADR 0030）
- 初回の子犬（`new_dog`）は、最初からいくつかの世話ができるよう欲求をある程度高くしておく
  （空腹 60・運動 50・退屈 60・寂しさ 60・眠気 20、なつき度 20）。欲求が低いと「満たされている」で
  世話を断られ、初回のプレイで何もできないため

## 欲求（needs）

### 関数

```cpp
enum class Activity { Awake, Sleeping, Walking };
using Duration = std::chrono::duration<double>;

void advance_needs(Needs&, Activity, Duration elapsed, const Tuning&);          // 起動中の変化
void apply_neglect(double& affection, const Needs&, Duration elapsed, const Tuning&); // 起動中のなつき度の低下
void apply_absence(DogState&, Duration elapsed, const Tuning&);                 // 不在中の一括計算
double max_need(const Needs&);
double mood(const Needs&);
```

1秒刻みごとに `advance_needs` → `apply_neglect` の順に呼ぶ（なつき度の判定には更新後の欲求を使う）。
`Activity` は、散歩中（`DogState::walking`）なら `Walking`、それ以外は `Awake`。
`Sleeping` は行動意図（behavior）の設計で反映する。

### 起動中の変化（1秒刻みごと）

| 欲求 | 起きている間 | 寝ている間（Sleep） | 散歩中 |
|---|---|---|---|
| 空腹 | 増える | 増える | 増える |
| 運動 | 増える | 増える | **減る**（ADR 0018） |
| 退屈 | 増える | 増える | 増える |
| 寂しさ | 増える | 増える | 増える |
| 眠気 | 増える | **減る** | 増える |

- 変化率は欲求ごとに `Tuning` に持つ（1時間あたりの量）。変化率は昼夜を問わず一定（ADR 0032）
- 起動中は 0〜100 の範囲で変化する
- ごはん・遊ぶ・なでるによる回復は、世話（care）の設計で決める

### なつき度の低下

- いずれかの欲求が `Tuning::neglect_threshold`（例：80）以上の間、一定の割合で下がる（ADR 0030）
- 起動中も不在中も同じ規則とする

### 不在中の一括計算（ADR 0022）

閉じていた間の経過 `elapsed` から、次を一度に計算する。

- **眠気以外の4つ**：`min(need + rate × elapsed, max(need, offline_need_cap))`。
  閉じている間は `offline_need_cap`（例：80）を超えて増えない。すでに超えていればそのまま
- **眠気**：留守中は寝ていたものとして、`max(sleepiness − sleep_recovery × elapsed, 0)` で減る
- **なつき度**：各欲求がしきい値に達する時刻は一定の変化率から計算できるため、「いずれかの欲求が
  しきい値以上だった時間」を求め、その時間に応じて下げる。1回の不在での低下は
  `offline_affection_loss_max`（例：10）まで。不在前の欲求から計算するため、欲求より先に更新する
  - 増える4つの欲求：最初にしきい値に達した時刻 `b` から後はずっとしきい値以上（上限がしきい値未満なら達しない）
  - 減る眠気：最初からしきい値以上なら、下回る時刻 `a` までの間
  - しきい値以上だった時間 ＝ `[0, a)` と `[b, elapsed)` の和集合の長さ（重なりは二重に数えない）
- 行動意図は進めず、復帰後に決め直す

1秒刻みを同じ時間だけ繰り返した結果と、不在の一括計算の結果は一般には一致しない（不在中は上限と眠気の
扱いが異なるため）。単体テストでは、上限に届かない範囲で眠気以外の欲求が一致することを確かめる。

### 機嫌

```
mood = 100 − （5つの欲求の平均）
```

計算式は調整する可能性があるため、`mood()` の中に閉じ込める。

## 世話（care）

本書で扱う世話は **ごはん（Feed）・なでる（Pet）・遊ぶ（Play）・散歩（Walk）**。
しつける・芸をさせるは芸（tricks）、成長ポイントは成長（growth）、「寝ているので不可」は
行動意図（behavior）の設計で追加する。

### 世話と欲求

| 世話 | 対応する欲求 | 効果 |
|---|---|---|
| Feed | 空腹 | 受け付けた時点で `relief` だけ減る（ADR 0016） |
| Pet | 寂しさ | 同上 |
| Play | 退屈 | 同上 |
| Walk | 運動 | 受け付けると散歩中になり、散歩中の時間に応じて減る（ADR 0018） |

受け付けたときは、どの世話でもなつき度が `affection_gain` だけ上がる。

### 実行できるかの判定

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

### 1日の回数の数え方

- 世話ごとに `CareRecord`（最後に受け付けた時刻、今日の回数）を持つ
- 回数を数えている日付を `DogState::care_day`（現地の日付）に持つ。判定時は、今日と `care_day` が
  違えば回数を 0 とみなす。受け付けたときに日付が変わっていれば、すべての世話の回数を 0 に戻してから数える

### 散歩

- 散歩の開始は世話の1つ（`Care::Walk`）として判定・記録する。受け付けると `walking = true`
- 散歩中は運動の欲求が `walk_exercise_per_hour` で減る。0 を下回らないことが効果の上限になる
- 家に戻ったら `app` が `end_walk()` を呼ぶ
- 散歩中に終了していた場合は、`resume()` で散歩を終える（ADR 0029）

### 関数

```cpp
enum class CareBlock { None, AlreadyWalking, DailyLimit, Cooldown, NotNeeded };

struct CareAvailability {
    CareBlock block = CareBlock::None;
    std::chrono::seconds remaining{0};
    bool available() const;
};

CareAvailability check_care(const DogState&, Care, const ClockReading& now, const Tuning&);
CareAvailability apply_care(DogState&, Care, const ClockReading& now, const Tuning&);  // 断ったら状態を変えない
```

## 時間の進め方（simulation の一部）

- `Simulation` は前回処理した時刻と、1秒に満たない端数を持つ（ADR 0021）
- `step()` は時計を読み、前回からの経過を1秒刻みで処理し、端数を持ち越す
- 経過がマイナス（時計が戻った）なら 0 として扱う（ADR 0022）
- **起動中でも、前回から `Tuning::offline_gap`（10分）以上空いた場合は不在として扱う**。
  起動したまま PC がスリープした場合などに、1秒刻みで何時間分も処理しないため。
  デバッグ時計の ×3600（ADR 0025）では1フレームで約60秒進むため、それが不在扱いにならない長さにする
- 起動時は `resume()` を1回呼び、セーブの最終終了時刻からの経過を不在の一括計算で進める。
  最終終了時刻が未来（時計が戻った）なら何もしない

```cpp
class Simulation {
public:
    Simulation(const Clock& clock, DogState state, TimePoint last_saved, Tuning tuning = {});

    void resume();  // 起動時に1回。散歩中だったら散歩を終える
    void step();    // 毎フレーム
    CareAvailability availability(Care care) const;  // メニューの表示用
    CareAvailability do_care(Care care);  // 先に step() で時間を進めてから判定・実行する
    void end_walk();                      // 散歩から家に戻ったとき
    const DogState& state() const;
    // 行動の終了通知は behavior の設計で追加する
};
```

`resume()` と `step()` は、最初の出来事（成長・芸の習得など）を設計する時点で、出来事のリストを返す形に
変える（ADR 0020）。不在中の出来事は「おかえり」の表示に使う（ADR 0011）。

## Tuning（本書で決めた範囲）

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
| `offline_gap` | 10分 | 前回の `step()` からこれ以上空いたら不在として扱う |

世話ごとの数値（`CareTuning`）。

| 世話 | `cooldown` | `daily_limit` | `min_need` | `relief` | `affection_gain` |
|---|---|---|---|---|---|
| `feed` | 3時間 | 3 | 30 | 70 | 1 |
| `pet` | 10分 | 10 | 10 | 40 | 2 |
| `play` | 30分 | 6 | 20 | 50 | 2 |
| `walk` | 2時間 | 3 | 30 | 0（時間で減る） | 3 |

## テスト方針

- `FakeClock` で時刻と時差を操作し、`Simulation` の結果を確かめる
- `needs` の関数は `Simulation` を通さずに直接テストする
- テストでは `Tuning` の数値を明示する（既定値に依存しない）
- 小数の比較は誤差を許容する（ADR 0031）
