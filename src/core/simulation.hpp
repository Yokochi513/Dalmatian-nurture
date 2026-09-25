#pragma once

#include "core/care.hpp"
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

    // 起動時に1回呼ぶ。最終終了時刻からの経過を不在として一括で進める（ADR 0022）。
    // 散歩中に終了していた場合は散歩を終える（ADR 0029）
    void resume();

    // 毎フレーム呼ぶ。前回からの経過を1秒刻みで進め、端数は持ち越す（ADR 0021）
    void step();

    // 世話を実行できるか（ADR 0017）
    CareAvailability availability(Care care) const;

    // 世話を実行する。先に step() で時間を進めてから判定する
    CareAvailability do_care(Care care);

    // 散歩から家に戻ったときに呼ぶ（ADR 0018）
    void end_walk();

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
