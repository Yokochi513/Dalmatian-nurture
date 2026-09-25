#pragma once

#include "core/dog.hpp"
#include "core/event.hpp"
#include "core/tuning.hpp"

#include <chrono>
#include <vector>

namespace dal::core {

// 成長ポイントを足す（1日の上限つき）。成長したら段階ごとに Grew を返す
std::vector<Event> add_growth(DogState& state, double points, std::chrono::local_days today, const Tuning& tuning);

// 累計の成長ポイントに対応する段階
GrowthStage stage_for(double growth_points, const Tuning& tuning);

// 次の段階までの進み具合（0〜1）。成犬なら 1
double growth_progress(const DogState& state, const Tuning& tuning);

} // namespace dal::core
