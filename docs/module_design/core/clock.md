# core / 時計（clock）

ソース：`src/core/clock.hpp`　概要：[core.md](../core.md)

## 決めたこと

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

// 現地時刻と現地の日付
std::chrono::local_time<std::chrono::milliseconds> local_now(const ClockReading&);
std::chrono::local_days local_day(const ClockReading&);
double hours_of_day(const ClockReading&);  // 一日の中の時刻（現地時刻の時、0〜24 の小数。ADR 0024）

} // namespace dal::core
```

- 経過時間の計算とセーブの最終終了時刻には `utc` を使う
- 「1日」の区切り（ADR 0023）と一日の中の時刻（ADR 0024）には現地時刻（`utc + utc_offset`）を使う
- core は OS のタイムゾーン情報を直接読まない。テストでは時差も自由に設定できる
- `Simulation` は作成時に `const Clock&` を受け取り、`step()` や世話の命令の中で自分で時刻を読む。
  すべての操作が同じ時計を使うことが保証される（[simulation.md](simulation.md)）

## 時計の実装

時計の実装は core の外に置く。

| 実装 | 置き場所 | 用途 |
|---|---|---|
| `SystemClock` | `platform/system_clock` | 本番。OS の時計と、`std::chrono::current_zone()` の時差を読む。タイムゾーンを読めなければ時差 0 |
| `DebugClock` | `app` | 倍速・不在の再現（ADR 0025）。未実装 |
| `FakeClock` | `tests/core/fake_clock.hpp` | テスト。時刻と時差を自由に設定し、進める |

## 検討して採らなかった案

- **現地時刻だけを渡す**: 単純だが、夏時間の切り替えやタイムゾーンをまたぐ移動で経過時間がずれる
- **UTC だけを渡し、時差は core が OS から読む**: 時計は単純になるが、core が OS に依存し、テストで時差を固定できない
- **呼び出しごとに時刻を引数で渡す**: core の入出力が引数だけになるが、呼び出しごとに違う時刻を渡す誤りが起きうる
