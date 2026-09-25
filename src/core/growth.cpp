#include "core/growth.hpp"

#include <algorithm>

namespace dal::core {

std::vector<Event> add_growth(DogState& state, double points, std::chrono::local_days today, const Tuning& tuning)
{
    if (state.growth_day != today) {
        state.growth_today = 0.0;
        state.growth_day = today;
    }

    const double room = std::max(tuning.growth.daily_cap - state.growth_today, 0.0);
    const double added = std::clamp(points, 0.0, room);
    state.growth_today += added;
    state.growth_points += added;

    std::vector<Event> events;
    const GrowthStage target = stage_for(state.growth_points, tuning);
    while (state.stage < target) {
        state.stage = static_cast<GrowthStage>(static_cast<int>(state.stage) + 1);
        events.push_back({EventKind::Grew, state.stage});
    }
    return events;
}

GrowthStage stage_for(double growth_points, const Tuning& tuning)
{
    if (growth_points >= tuning.growth.adult_at) {
        return GrowthStage::Adult;
    }
    if (growth_points >= tuning.growth.young_at) {
        return GrowthStage::Young;
    }
    return GrowthStage::Puppy;
}

double growth_progress(const DogState& state, const Tuning& tuning)
{
    double from = 0.0;
    double to = 0.0;
    switch (state.stage) {
    case GrowthStage::Puppy:
        from = 0.0;
        to = tuning.growth.young_at;
        break;
    case GrowthStage::Young:
        from = tuning.growth.young_at;
        to = tuning.growth.adult_at;
        break;
    case GrowthStage::Adult:
        return 1.0;
    }
    if (to <= from) {
        return 1.0;
    }
    return std::clamp((state.growth_points - from) / (to - from), 0.0, 1.0);
}

} // namespace dal::core
