#pragma once

#include "core/dog.hpp"
#include "core/tuning.hpp"

#include <chrono>

namespace dal::core {

using Duration = std::chrono::duration<double>;

// 欲求の増減に影響する犬の活動
enum class Activity { Awake, Sleeping, Walking };

// 起動中の欲求の変化。値は 0〜100 に切り詰める
void advance_needs(Needs& needs, Activity activity, Duration elapsed, const Tuning& tuning);

// 起動中のなつき度の低下。いずれかの欲求がしきい値以上なら下がる
void apply_neglect(double& affection, const Needs& needs, Duration elapsed, const Tuning& tuning);

// 閉じていた間の一括計算（ADR 0022）。欲求となつき度を更新する
void apply_absence(DogState& state, Duration elapsed, const Tuning& tuning);

double max_need(const Needs& needs);

// 機嫌（0〜100）。保存せず毎回計算する（ADR 0030）
double mood(const Needs& needs);

} // namespace dal::core
