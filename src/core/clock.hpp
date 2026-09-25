#pragma once

#include <chrono>

namespace dal::core {

using TimePoint = std::chrono::sys_time<std::chrono::milliseconds>;

// 時計が返す値。経過時間とセーブには utc、日付の区切りと昼夜には utc + utc_offset を使う
struct ClockReading {
    TimePoint utc;
    std::chrono::minutes utc_offset{0};  // 日本なら +540
};

// 時計のインターフェース。実装は platform（本番）・app（デバッグ）・tests（テスト）に置く
class Clock {
public:
    virtual ~Clock() = default;
    virtual ClockReading now() const = 0;
};

} // namespace dal::core
