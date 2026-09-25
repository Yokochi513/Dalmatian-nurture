#include "core/simulation.hpp"

#include "core/growth.hpp"
#include "core/needs.hpp"

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
{
}

std::vector<Event> Simulation::resume()
{
    const TimePoint now = clock_.now().utc;
    state_.walking = false;
    if (now > last_processed_) {
        apply_absence(state_, now - last_processed_, tuning_);
    }
    last_processed_ = now;
    remainder_ = {};
    return {};
}

std::vector<Event> Simulation::step()
{
    const TimePoint now = clock_.now().utc;
    const auto elapsed = now - last_processed_;
    last_processed_ = now;

    // 時計が戻った場合は経過を 0 とする
    if (elapsed <= std::chrono::milliseconds::zero()) {
        return {};
    }

    // 起動したまま PC がスリープした場合などは、不在として一括で進める
    if (elapsed >= tuning_.offline_gap) {
        apply_absence(state_, elapsed, tuning_);
        remainder_ = {};
        return {};
    }

    remainder_ += elapsed;
    while (remainder_ >= kTick) {
        tick();
        remainder_ -= kTick;
    }
    return {};
}

CareAvailability Simulation::availability(Care care) const
{
    return check_care(state_, care, clock_.now(), tuning_);
}

CareResult Simulation::do_care(Care care)
{
    CareResult result;
    result.events = step();

    const ClockReading now = clock_.now();
    result.availability = apply_care(state_, care, now, tuning_);
    if (result.availability.available()) {
        const auto grown = add_growth(state_, tuning_of(tuning_, care).growth_points, local_day(now), tuning_);
        result.events.insert(result.events.end(), grown.begin(), grown.end());
    }
    return result;
}

std::vector<Event> Simulation::end_walk()
{
    auto events = step();
    state_.walking = false;
    return events;
}

void Simulation::tick()
{
    // 寝ている間（Sleeping）は行動意図（behavior）の設計で反映する
    const Activity activity = state_.walking ? Activity::Walking : Activity::Awake;
    advance_needs(state_.needs, activity, kTick, tuning_);
    apply_neglect(state_.affection, state_.needs, kTick, tuning_);
}

} // namespace dal::core
