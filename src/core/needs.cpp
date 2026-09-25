#include "core/needs.hpp"

#include <algorithm>
#include <limits>

namespace dal::core {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

double to_hours(Duration elapsed)
{
    return elapsed.count() / 3600.0;
}

double clamp_value(double value)
{
    return std::clamp(value, 0.0, 100.0);
}

// 不在中に増える欲求。cap を超えて増えないが、すでに超えていれば下げない
double grow_capped(double value, double rate, double hours, double cap)
{
    return clamp_value(std::min(value + rate * hours, std::max(value, cap)));
}

// 不在中に増える欲求が threshold に達するまでの時間（時間単位）。達しないなら無限大
double hours_until_reach(double value, double rate, double cap, double threshold)
{
    if (value >= threshold) {
        return 0.0;
    }
    if (rate <= 0.0 || std::max(value, cap) < threshold) {
        return kInfinity;
    }
    return (threshold - value) / rate;
}

// 不在中に「いずれかの欲求がしきい値以上」だった時間（時間単位）
double hours_neglected_during_absence(const Needs& needs, double hours, const Tuning& t)
{
    const double threshold = t.neglect_threshold;
    const double cap = t.offline_need_cap;

    // 増える4つの欲求：最初にしきい値に達した時刻から後はずっとしきい値以上
    const double rising_from = std::min({
        hours_until_reach(needs.hunger, t.hunger_per_hour, cap, threshold),
        hours_until_reach(needs.exercise, t.exercise_per_hour, cap, threshold),
        hours_until_reach(needs.boredom, t.boredom_per_hour, cap, threshold),
        hours_until_reach(needs.loneliness, t.loneliness_per_hour, cap, threshold),
    });

    // 減る眠気：最初からしきい値以上なら、下回るまでの間
    double sleepy_until = 0.0;
    if (needs.sleepiness >= threshold) {
        sleepy_until = t.sleep_recovery_per_hour > 0.0
            ? (needs.sleepiness - threshold) / t.sleep_recovery_per_hour
            : kInfinity;
    }

    // [0, sleepy_until) と [rising_from, hours) の和集合の長さ
    const double a = std::min(sleepy_until, hours);
    const double b = std::min(rising_from, hours);
    return a + (hours - std::max(a, b));
}

} // namespace

void advance_needs(Needs& needs, Activity activity, Duration elapsed, const Tuning& t)
{
    const double hours = to_hours(elapsed);
    const double exercise_rate =
        activity == Activity::Walking ? -t.walk_exercise_per_hour : t.exercise_per_hour;
    const double sleepiness_rate =
        activity == Activity::Sleeping ? -t.sleep_recovery_per_hour : t.sleepiness_per_hour;

    needs.hunger = clamp_value(needs.hunger + t.hunger_per_hour * hours);
    needs.exercise = clamp_value(needs.exercise + exercise_rate * hours);
    needs.boredom = clamp_value(needs.boredom + t.boredom_per_hour * hours);
    needs.loneliness = clamp_value(needs.loneliness + t.loneliness_per_hour * hours);
    needs.sleepiness = clamp_value(needs.sleepiness + sleepiness_rate * hours);
}

void apply_neglect(double& affection, const Needs& needs, Duration elapsed, const Tuning& t)
{
    if (max_need(needs) >= t.neglect_threshold) {
        affection = clamp_value(affection - t.affection_loss_per_hour * to_hours(elapsed));
    }
}

void apply_absence(DogState& state, Duration elapsed, const Tuning& t)
{
    const double hours = to_hours(elapsed);
    if (hours <= 0.0) {
        return;
    }

    // なつき度は不在前の欲求から計算するため、欲求より先に更新する
    const double loss = std::min(
        t.affection_loss_per_hour * hours_neglected_during_absence(state.needs, hours, t),
        t.offline_affection_loss_max);
    state.affection = clamp_value(state.affection - loss);

    Needs& needs = state.needs;
    const double cap = t.offline_need_cap;
    needs.hunger = grow_capped(needs.hunger, t.hunger_per_hour, hours, cap);
    needs.exercise = grow_capped(needs.exercise, t.exercise_per_hour, hours, cap);
    needs.boredom = grow_capped(needs.boredom, t.boredom_per_hour, hours, cap);
    needs.loneliness = grow_capped(needs.loneliness, t.loneliness_per_hour, hours, cap);
    // 留守中は寝ていたものとして扱う
    needs.sleepiness = clamp_value(needs.sleepiness - t.sleep_recovery_per_hour * hours);
}

double max_need(const Needs& needs)
{
    return std::max({needs.hunger, needs.exercise, needs.boredom, needs.loneliness, needs.sleepiness});
}

double mood(const Needs& needs)
{
    const double average =
        (needs.hunger + needs.exercise + needs.boredom + needs.loneliness + needs.sleepiness) / 5.0;
    return clamp_value(100.0 - average);
}

} // namespace dal::core
