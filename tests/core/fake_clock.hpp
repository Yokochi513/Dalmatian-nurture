#pragma once

#include "core/clock.hpp"

#include <chrono>

namespace dal::core::test {

// テスト用の時計。時刻と時差を自由に設定し、進める
class FakeClock final : public Clock {
public:
    explicit FakeClock(TimePoint utc = TimePoint{}, std::chrono::minutes utc_offset = std::chrono::hours{9})
        : reading_{utc, utc_offset}
    {
    }

    ClockReading now() const override { return reading_; }

    void advance(std::chrono::milliseconds duration) { reading_.utc += duration; }
    void set(TimePoint utc) { reading_.utc = utc; }

private:
    ClockReading reading_;
};

} // namespace dal::core::test
