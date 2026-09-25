#include "core/care.hpp"

#include <algorithm>

namespace dal::core {

namespace {

// 世話に対応する欲求
double& need_of(Needs& needs, Care care)
{
    switch (care) {
    case Care::Feed: return needs.hunger;
    case Care::Pet: return needs.loneliness;
    case Care::Play: return needs.boredom;
    case Care::Walk: return needs.exercise;
    case Care::Train: return needs.boredom;
    case Care::PerformTrick: return needs.boredom;
    }
    return needs.hunger;
}

double need_of(const Needs& needs, Care care)
{
    switch (care) {
    case Care::Feed: return needs.hunger;
    case Care::Pet: return needs.loneliness;
    case Care::Play: return needs.boredom;
    case Care::Walk: return needs.exercise;
    case Care::Train: return needs.boredom;
    case Care::PerformTrick: return needs.boredom;
    }
    return needs.hunger;
}

int count_today(const DogState& state, Care care, std::chrono::local_days today)
{
    return state.care_day == today ? record_of(state, care).count_today : 0;
}

// 次の現地の0時までの時間
std::chrono::seconds until_next_day(const ClockReading& now)
{
    const auto next_day = local_day(now) + std::chrono::days{1};
    return std::chrono::ceil<std::chrono::seconds>(next_day - local_now(now));
}

} // namespace

const CareTuning& tuning_of(const Tuning& tuning, Care care)
{
    switch (care) {
    case Care::Feed: return tuning.feed;
    case Care::Pet: return tuning.pet;
    case Care::Play: return tuning.play;
    case Care::Walk: return tuning.walk;
    case Care::Train: return tuning.train;
    case Care::PerformTrick: return tuning.perform_trick;
    }
    return tuning.feed;
}

bool is_trick_care(Care care)
{
    return care == Care::Train || care == Care::PerformTrick;
}

CareAvailability check_care(const DogState& state, Care care, const ClockReading& now, const Tuning& tuning)
{
    const CareTuning& t = tuning_of(tuning, care);

    if (care == Care::Walk && state.walking) {
        return {CareBlock::AlreadyWalking};
    }

    if (state.intent.kind == IntentKind::Sleep) {
        return {CareBlock::Sleeping};
    }

    if (count_today(state, care, local_day(now)) >= t.daily_limit) {
        return {CareBlock::DailyLimit, until_next_day(now)};
    }

    if (const auto& last = record_of(state, care).last_done) {
        // 時計が戻って last が未来になっても、残り時間は cooldown を超えない
        const auto elapsed = std::max(std::chrono::floor<std::chrono::seconds>(now.utc - *last),
                                      std::chrono::seconds::zero());
        if (elapsed < t.cooldown) {
            return {CareBlock::Cooldown, t.cooldown - elapsed};
        }
    }

    if (need_of(state.needs, care) < t.min_need) {
        return {CareBlock::NotNeeded};
    }

    return {};
}

CareAvailability apply_care(DogState& state, Care care, const ClockReading& now, const Tuning& tuning)
{
    if (is_trick_care(care)) {
        return {CareBlock::TrickNotSpecified};
    }
    const CareAvailability availability = check_care(state, care, now, tuning);
    if (availability.available()) {
        commit_care(state, care, now, tuning);
    }
    return availability;
}

void commit_care(DogState& state, Care care, const ClockReading& now, const Tuning& tuning)
{
    const CareTuning& t = tuning_of(tuning, care);
    const auto today = local_day(now);
    if (state.care_day != today) {
        for (CareRecord& record : state.care_records) {
            record.count_today = 0;
        }
        state.care_day = today;
    }

    CareRecord& record = record_of(state, care);
    record.last_done = now.utc;
    ++record.count_today;

    double& need = need_of(state.needs, care);
    need = std::clamp(need - t.relief, 0.0, 100.0);
    state.affection = std::clamp(state.affection + t.affection_gain, 0.0, 100.0);

    if (care == Care::Walk) {
        state.walking = true;
    }
}

} // namespace dal::core
