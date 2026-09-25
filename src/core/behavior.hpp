#pragma once

#include "core/clock.hpp"
#include "core/dog.hpp"
#include "core/tuning.hpp"

#include <cstdint>

namespace dal::core {

// 動作の意図（app の終了通知で終わる）か。Idle・Sleep・Beg・Follow は状態の意図（core が見直す）
bool is_action(IntentKind kind);

// 行動意図の種類ごとの行き先（ADR 0037）
Destination destination_of(IntentKind kind);

// 夜か（hours は現地時刻の時）
bool is_night(double hours, const Tuning& tuning);

// 規則で次の意図を決める。状態の意図で同じ意図が選ばれたら、今の意図をそのまま返す
Intent decide_intent(const DogState& state, const ClockReading& now, const Tuning& tuning);

// 意図を切り替える（id を増やし、since を now にする）
void set_intent(DogState& state, IntentKind kind, const ClockReading& now);

// 1秒刻みごとに呼ぶ。状態の意図を見直し、長すぎる動作の意図を終わらせる
void update_intent(DogState& state, const ClockReading& now, const Tuning& tuning);

// 動作の終了通知。id が今の意図と同じ動作の意図なら、規則で次の意図を決める
void finish_intent(DogState& state, std::uint64_t id, const ClockReading& now, const Tuning& tuning);

// 世話を受け付けたときの意図（ADR 0016）
void start_care_intent(DogState& state, Care care, Trick trick, const ClockReading& now);

} // namespace dal::core
