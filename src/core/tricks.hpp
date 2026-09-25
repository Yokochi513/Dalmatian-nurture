#pragma once

#include "core/care.hpp"
#include "core/clock.hpp"
#include "core/dog.hpp"
#include "core/event.hpp"
#include "core/tuning.hpp"

#include <vector>

namespace dal::core {

struct TrickResult {
    CareAvailability availability;
    std::vector<Event> events;  // LearnedTrick
};

// 今の成長段階で練習できるか（ADR 0034）
bool is_unlocked(Trick trick, GrowthStage stage);
std::vector<Trick> unlocked_tricks(GrowthStage stage);

// 習熟度が 100 なら習得済み
bool is_learned(const DogState& state, Trick trick);
double& proficiency_of(DogState& state, Trick trick);
double proficiency_of(const DogState& state, Trick trick);

// 練習1回の習熟度の上がり幅。乱数は使わない（ADR 0035）
double training_gain(const DogState& state, const Tuning& tuning);

// 芸を指定した判定。芸ごとの判定 → 世話の判定の順。care は Train か PerformTrick
CareAvailability check_trick(const DogState& state, Care care, Trick trick, const ClockReading& now,
                             const Tuning& tuning);

// 芸を指定して実行する。受け付けたら世話の効果に加えて、Train なら習熟度を上げる
TrickResult apply_trick(DogState& state, Care care, Trick trick, const ClockReading& now, const Tuning& tuning);

} // namespace dal::core
