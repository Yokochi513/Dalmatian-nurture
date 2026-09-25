#pragma once

#include "core/care.hpp"
#include "core/clock.hpp"
#include "core/dog.hpp"
#include "core/event.hpp"
#include "core/tuning.hpp"

#include <chrono>
#include <vector>

namespace dal::core {

// 世話の命令の結果
struct CareResult {
    CareAvailability availability;
    std::vector<Event> events;  // step() で起きた出来事と、世話による成長など
};

// 育成ロジックの入口。犬の状態を持ち、時計を読んで時間を進める
class Simulation {
public:
    // last_saved: セーブの最終終了時刻（ADR 0026）
    Simulation(const Clock& clock, DogState state, TimePoint last_saved, Tuning tuning = {});

    // 起動時に1回呼ぶ。最終終了時刻からの経過を不在として一括で進める（ADR 0022）。
    // 散歩中に終了していた場合は散歩を終える（ADR 0029）
    std::vector<Event> resume();

    // 毎フレーム呼ぶ。前回からの経過を1秒刻みで進め、端数は持ち越す（ADR 0021）
    std::vector<Event> step();

    // 世話を実行できるか（ADR 0017）。芸を指定しない場合、しつける・芸をさせるは上限とクールダウンだけを判定する
    CareAvailability availability(Care care) const;
    CareAvailability availability(Care care, Trick trick) const;

    // 世話を実行する。先に step() で時間を進めてから判定し、受け付けたら成長ポイントを足す。
    // しつける・芸をさせるは do_trick を使う（do_care では TrickNotSpecified で断る）
    CareResult do_care(Care care);
    CareResult do_trick(Care care, Trick trick);

    // 散歩から家に戻ったときに呼ぶ（ADR 0018）
    std::vector<Event> end_walk();

    const DogState& state() const { return state_; }

private:
    void tick();
    void add_growth_for(Care care, const ClockReading& now, std::vector<Event>& events);

    const Clock& clock_;
    DogState state_;
    Tuning tuning_;
    TimePoint last_processed_;
    std::chrono::milliseconds remainder_{0};
};

} // namespace dal::core
