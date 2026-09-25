#include "core/behavior.hpp"

#include <utility>

namespace dal::core {

namespace {

struct Choice {
    IntentKind kind = IntentKind::Idle;
    NeedKind need = NeedKind::Hunger;
};

// 眠気以外で最も高い欲求（同じ値なら Needs の順で先のもの）
std::pair<NeedKind, double> top_need(const Needs& needs)
{
    std::pair<NeedKind, double> top{NeedKind::Hunger, needs.hunger};
    const std::pair<NeedKind, double> others[] = {
        {NeedKind::Exercise, needs.exercise},
        {NeedKind::Boredom, needs.boredom},
        {NeedKind::Loneliness, needs.loneliness},
    };
    for (const auto& candidate : others) {
        if (candidate.second > top.second) {
            top = candidate;
        }
    }
    return top;
}

Choice choose(const DogState& state, const ClockReading& now, const Tuning& tuning)
{
    const BehaviorTuning& b = tuning.behavior;
    const Intent& current = state.intent;
    const auto elapsed = now.utc - current.since;

    if (state.walking) {
        if (current.kind == IntentKind::Follow && elapsed >= b.sniff_interval) {
            return {state.needs.boredom >= b.dig_threshold ? IntentKind::Dig : IntentKind::Sniff};
        }
        return {IntentKind::Follow};
    }

    if (current.kind == IntentKind::Sleep) {
        return {state.needs.sleepiness <= b.wake_threshold ? IntentKind::Stretch : IntentKind::Sleep};
    }

    const double sleep_threshold =
        is_night(hours_of_day(now), tuning) ? b.sleep_threshold_night : b.sleep_threshold_day;
    if (state.needs.sleepiness >= sleep_threshold) {
        return {IntentKind::Sleep};
    }

    const auto [need, value] = top_need(state.needs);
    if (value >= b.beg_threshold) {
        if (current.kind == IntentKind::Beg && current.need == need && elapsed >= b.bark_after) {
            return {IntentKind::Bark};
        }
        return {IntentKind::Beg, need};
    }

    if (current.kind == IntentKind::Idle && elapsed >= b.idle_before_wander
        && state.needs.boredom >= b.wander_threshold) {
        return {IntentKind::Wander};
    }
    return {IntentKind::Idle};
}

Intent make_intent(const Intent& current, IntentKind kind, const ClockReading& now)
{
    Intent next;
    next.kind = kind;
    next.destination = destination_of(kind);
    next.since = now.utc;
    next.id = current.id + 1;
    return next;
}

} // namespace

bool is_action(IntentKind kind)
{
    switch (kind) {
    case IntentKind::Idle:
    case IntentKind::Sleep:
    case IntentKind::Beg:
    case IntentKind::Follow:
        return false;
    default:
        return true;
    }
}

Destination destination_of(IntentKind kind)
{
    switch (kind) {
    case IntentKind::Idle:
    case IntentKind::Stretch:
    case IntentKind::Bark:
        return Destination::Here;
    case IntentKind::Wander:
    case IntentKind::Sniff:
    case IntentKind::Dig:
        return Destination::Any;
    case IntentKind::Sleep:
        return Destination::Bed;
    case IntentKind::Eat:
        return Destination::Bowl;
    case IntentKind::Beg:
    case IntentKind::Greet:
    case IntentKind::Petted:
    case IntentKind::Play:
    case IntentKind::Train:
    case IntentKind::PerformTrick:
    case IntentKind::Follow:
        return Destination::Owner;
    }
    return Destination::Here;
}

bool is_night(double hours, const Tuning& tuning)
{
    const double start = tuning.behavior.night_start;
    const double end = tuning.behavior.night_end;
    if (start > end) {
        return hours >= start || hours < end;  // 日付をまたぐ（例：21時〜6時）
    }
    return hours >= start && hours < end;
}

Intent decide_intent(const DogState& state, const ClockReading& now, const Tuning& tuning)
{
    const Intent& current = state.intent;
    const Choice choice = choose(state, now, tuning);

    const bool same = !is_action(current.kind) && choice.kind == current.kind
        && (choice.kind != IntentKind::Beg || choice.need == current.need);
    if (same) {
        return current;
    }

    Intent next = make_intent(current, choice.kind, now);
    next.need = choice.need;
    return next;
}

void set_intent(DogState& state, IntentKind kind, const ClockReading& now)
{
    state.intent = make_intent(state.intent, kind, now);
}

void update_intent(DogState& state, const ClockReading& now, const Tuning& tuning)
{
    if (is_action(state.intent.kind)) {
        // 終了通知が来ない場合の保険（ADR 0016）
        if (now.utc - state.intent.since >= tuning.behavior.max_action_duration) {
            state.intent = decide_intent(state, now, tuning);
        }
        return;
    }
    state.intent = decide_intent(state, now, tuning);
}

void finish_intent(DogState& state, std::uint64_t id, const ClockReading& now, const Tuning& tuning)
{
    if (id != state.intent.id || !is_action(state.intent.kind)) {
        return;
    }
    state.intent = decide_intent(state, now, tuning);
}

void start_care_intent(DogState& state, Care care, Trick trick, const ClockReading& now)
{
    IntentKind kind = IntentKind::Idle;
    switch (care) {
    case Care::Feed: kind = IntentKind::Eat; break;
    case Care::Pet: kind = IntentKind::Petted; break;
    case Care::Play: kind = IntentKind::Play; break;
    case Care::Walk: kind = IntentKind::Follow; break;
    case Care::Train: kind = IntentKind::Train; break;
    case Care::PerformTrick: kind = IntentKind::PerformTrick; break;
    }
    set_intent(state, kind, now);
    state.intent.trick = trick;
}

} // namespace dal::core
