#pragma once

#include "core/clock.hpp"
#include "core/dog.hpp"
#include "core/tuning.hpp"

#include <chrono>

namespace dal::core {

// 育成ロジックの入口。犬の状態を持ち、時計を読んで時間を進める
class Simulation {
public:
    // last_saved: セーブの最終終了時刻（ADR 0026）
    Simulation(const Clock& clock, DogState state, TimePoint last_saved, Tuning tuning = {});

    // 起動時に1回呼ぶ。最終終了時刻からの経過を不在として一括で進める（ADR 0022）
    void resume();

    // 毎フレーム呼ぶ。前回からの経過を1秒刻みで進め、端数は持ち越す（ADR 0021）
    void step();

    const DogState& state() const { return state_; }

private:
    void tick();

    const Clock& clock_;
    DogState state_;
    Tuning tuning_;
    TimePoint last_processed_;
    std::chrono::milliseconds remainder_{0};
};

} // namespace dal::core
