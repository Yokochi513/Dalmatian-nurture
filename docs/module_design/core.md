# core モジュール設計

`dalmatian_core`（`src/core/`、名前空間 `dal::core`）の設計。
モジュールをまたぐ決定は [docs/adr/](../adr/) にあり、ここではそれを前提に core の内部を決める。

- 状態: 設計中（時計・犬の状態・欲求・時間の進め方まで）
- 未設計: 世話（care）、成長（growth）、芸（tricks）、行動意図（behavior）、セーブ（save）

## 責務

- 犬の状態を持ち、時間の経過と世話の命令によって更新する
- 犬の行動意図（何をしたいか・どこへ行きたいか）を決める
- セーブデータ（JSON）との相互変換を行う
- 描画・アニメーション・空間・OS を知らない。依存は標準C++ と nlohmann/json のみ（ADR 0009）

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
| `dog.hpp` | 犬の状態（`DogState`）と関連する列挙 | 本書（一部） |
| `tuning.hpp` | バランスの数値（`Tuning`） | 本書（一部） |
| `needs.hpp/.cpp` | 欲求の時間変化、不在中の一括計算、機嫌の計算 | 本書 |
| `simulation.hpp/.cpp` | 1秒刻みで進める入口、不在からの復帰 | 本書（一部） |
| `care.hpp/.cpp` | 世話の可否と効果、クールダウン、1日の上限 | 未設計 |
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

struct DogState {
    std::string name;
    Needs needs;
    double affection = 0.0;  // なつき度
    GrowthStage stage = GrowthStage::Puppy;
    // 以下は各節の設計で追加する：成長ポイント、芸の習熟度、世話の記録、行動意図、散歩中かどうか
};

} // namespace dal::core
```

- 欲求となつき度は小数の 0〜100。欲求は大きいほど強く欲しがっている（ADR 0031）
- 値を更新したら必ず 0〜100 に切り詰める
- **機嫌は保存しない**。`mood(const Needs&)` で毎回計算する（ADR 0030）

## 欲求（needs）

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
  `offline_affection_loss_max`（例：10）まで
- 行動意図は進めず、復帰後に決め直す

1秒刻みを同じ時間だけ繰り返した結果と、不在の一括計算の結果は一般には一致しない（不在中は上限と眠気の
扱いが異なるため）。単体テストでは、上限に届かない範囲で眠気以外の欲求が一致することを確かめる。

### 機嫌

```
mood = 100 − （5つの欲求の平均）
```

計算式は調整する可能性があるため、`mood()` の中に閉じ込める。

## 時間の進め方（simulation の一部）

- `Simulation` は前回処理した時刻と、1秒に満たない端数を持つ（ADR 0021）
- `step()` は時計を読み、前回からの経過を1秒刻みで処理し、端数を持ち越す
- 経過がマイナス（時計が戻った）なら 0 として扱う（ADR 0022）
- **起動中でも、前回から `Tuning::offline_gap`（例：60秒）以上空いた場合は不在として扱う**。
  起動したまま PC がスリープした場合などに、1秒刻みで何時間分も処理しないため
- 起動時は `resume()` を1回呼び、セーブの最終終了時刻からの経過を不在の一括計算で進める。
  不在中の出来事を返し、「おかえり」の表示に使う（ADR 0011・0020）

```cpp
class Simulation {
public:
    Simulation(const Clock& clock, DogState state, TimePoint last_saved, Tuning tuning = {});

    std::vector<Event> resume();  // 起動時に1回
    std::vector<Event> step();    // 毎フレーム
    const DogState& state() const;
    // 世話の命令・可否の問い合わせ・行動の終了通知は care と behavior の設計で追加する
};
```

`Event` の種類は、成長・芸・世話の設計で決める。

## Tuning（本書で決めた範囲）

```cpp
struct Tuning {
    // 欲求の変化率（1時間あたり）
    double hunger_per_hour;
    double exercise_per_hour;
    double boredom_per_hour;
    double loneliness_per_hour;
    double sleepiness_per_hour;
    double sleep_recovery_per_hour;     // Sleep 中・不在中の眠気の減り
    double walk_exercise_per_hour;      // 散歩中の運動の減り

    // 放置
    double neglect_threshold;           // なつき度が下がり始める欲求の値
    double affection_loss_per_hour;
    double offline_need_cap;            // 不在中に欲求が届く上限
    double offline_affection_loss_max;  // 1回の不在でのなつき度の低下の上限

    // 時間
    std::chrono::seconds offline_gap;   // これ以上空いたら不在として扱う
};
```

既定値はバランス調整の中で決める。

## テスト方針

- `FakeClock` で時刻と時差を操作し、`Simulation` の結果を確かめる
- `needs` の関数は `Simulation` を通さずに直接テストする
- テストでは `Tuning` の数値を明示する（既定値に依存しない）
- 小数の比較は誤差を許容する（ADR 0031）
