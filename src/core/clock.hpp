#pragma once

#include <chrono>

namespace dal::core {

using TimePoint = std::chrono::sys_time<std::chrono::milliseconds>;

// 時計が返す値。経過時間とセーブには utc、日付の区切りと昼夜には utc + utc_offset を使う
struct ClockReading {
    TimePoint utc;
    std::chrono::minutes utc_offset{0};  // 日本なら +540
};

// 現地時刻
inline std::chrono::local_time<std::chrono::milliseconds> local_now(const ClockReading& reading)
{
    return std::chrono::local_time<std::chrono::milliseconds>{
        reading.utc.time_since_epoch() + reading.utc_offset};
}

// 現地の日付。「1日」の区切りは現地時刻の0時（ADR 0023）
inline std::chrono::local_days local_day(const ClockReading& reading)
{
    return std::chrono::floor<std::chrono::days>(local_now(reading));
}

// 一日の中の時刻（現地時刻の時、0〜24 の小数。ADR 0024）
inline double hours_of_day(const ClockReading& reading)
{
    const auto now = local_now(reading);
    const auto since_midnight = now - std::chrono::floor<std::chrono::days>(now);
    return std::chrono::duration<double, std::ratio<3600>>(since_midnight).count();
}

// 時計のインターフェース。実装は platform（本番）・app（デバッグ）・tests（テスト）に置く
class Clock {
public:
    virtual ~Clock() = default;
    virtual ClockReading now() const = 0;
};

} // namespace dal::core
