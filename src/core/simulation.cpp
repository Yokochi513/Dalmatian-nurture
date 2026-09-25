#include "core/simulation.hpp"

#include "core/behavior.hpp"
#include "core/growth.hpp"
#include "core/needs.hpp"
#include "core/tricks.hpp"

#include <utility>

namespace dal::core {

namespace {

constexpr std::chrono::seconds kTick{1};

} // namespace

Simulation::Simulation(const Clock& clock, DogState state, TimePoint last_saved, Tuning tuning)
    : clock_(clock)
    , state_(std::move(state))
    , tuning_(tuning)
    , last_processed_(last_saved)
    , tick_time_(last_saved)
{
}

std::vector<Event> Simulation::resume()
{
    const ClockReading now = clock_.now();
    state_.walking = false;
    if (now.utc > last_processed_) {
        apply_absence(state_, now.utc - last_processed_, tuning_);
    }
    last_processed_ = now.utc;
    tick_time_ = now.utc;
    remainder_ = {};
    set_intent(state_, IntentKind::Greet, now);
    return {};
}

std::vector<Event> Simulation::step()
{
    const ClockReading now = clock_.now();
    const auto elapsed = now.utc - last_processed_;
    last_processed_ = now.utc;

    // 時計が戻った場合は経過を 0 とし、戻った時刻から改めて進める
    if (elapsed <= std::chrono::milliseconds::zero()) {
        tick_time_ = now.utc;
        remainder_ = {};
        return {};
    }

    // 起動したまま PC がスリープした場合などは、不在として一括で進め、行動意図を決め直す
    if (elapsed >= tuning_.offline_gap) {
        apply_absence(state_, elapsed, tuning_);
        tick_time_ = now.utc;
        remainder_ = {};
        state_.intent = decide_intent(state_, now, tuning_);
        return {};
    }

    remainder_ += elapsed;
    while (remainder_ >= kTick) {
        tick_time_ += kTick;
        remainder_ -= kTick;
        tick({tick_time_, now.utc_offset});
    }
    return {};
}

CareAvailability Simulation::availability(Care care) const
{
    return check_care(state_, care, clock_.now(), tuning_);
}

CareAvailability Simulation::availability(Care care, Trick trick) const
{
    return check_trick(state_, care, trick, clock_.now(), tuning_);
}

CareResult Simulation::do_care(Care care)
{
    CareResult result;
    result.events = step();

    const ClockReading now = clock_.now();
    result.availability = apply_care(state_, care, now, tuning_);
    if (result.availability.available()) {
        after_care(care, Trick::Sit, now, result.events);
    }
    return result;
}

CareResult Simulation::do_trick(Care care, Trick trick)
{
    CareResult result;
    result.events = step();

    const ClockReading now = clock_.now();
    TrickResult trick_result = apply_trick(state_, care, trick, now, tuning_);
    result.availability = trick_result.availability;
    result.events.insert(result.events.end(), trick_result.events.begin(), trick_result.events.end());
    if (result.availability.available()) {
        after_care(care, trick, now, result.events);
    }
    return result;
}

std::vector<Event> Simulation::end_walk()
{
    auto events = step();
    state_.walking = false;
    set_intent(state_, IntentKind::Greet, clock_.now());
    return events;
}

std::vector<Event> Simulation::finish_intent(std::uint64_t id)
{
    auto events = step();
    core::finish_intent(state_, id, clock_.now(), tuning_);
    return events;
}

double Simulation::time_of_day() const
{
    return hours_of_day(clock_.now());
}

void Simulation::tick(const ClockReading& at)
{
    Activity activity = Activity::Awake;
    if (state_.walking) {
        activity = Activity::Walking;
    } else if (state_.intent.kind == IntentKind::Sleep) {
        activity = Activity::Sleeping;
    }
    advance_needs(state_.needs, activity, kTick, tuning_);
    apply_neglect(state_.affection, state_.needs, kTick, tuning_);
    update_intent(state_, at, tuning_);
}

// 世話を受け付けた後の処理：成長ポイントを足し、行動意図を切り替える
void Simulation::after_care(Care care, Trick trick, const ClockReading& now, std::vector<Event>& events)
{
    const auto grown = add_growth(state_, tuning_of(tuning_, care).growth_points, local_day(now), tuning_);
    events.insert(events.end(), grown.begin(), grown.end());
    start_care_intent(state_, care, trick, now);
}

} // namespace dal::core
