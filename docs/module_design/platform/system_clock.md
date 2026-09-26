# platform / システム時計（system_clock）

ソース：`src/platform/system_clock.hpp`、`src/platform/system_clock.cpp`　概要：[platform.md](../platform.md)

core の時計（[core/clock.md](../core/clock.md)）の本番用の実装（ADR 0042）。

## 決めたこと

- UTC の時刻は `std::chrono::system_clock::now()` をミリ秒に切り捨てたもの
- 現地との時差は `std::chrono::current_zone()->get_info(utc).offset`（夏時間も含む）
- タイムゾーン情報を読めない環境では、時差 0（UTC）として扱う。例外は外に出さない

```cpp
class SystemClock final : public core::Clock {
public:
    core::ClockReading now() const override;
};
```

## 検討して採らなかった案

- **Windows API（`GetTimeZoneInformation`）で時差を得る**: 標準ライブラリに頼らないが、C++20 の
  タイムゾーン機能で足りる
