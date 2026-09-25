#include "core/tricks.hpp"

#include "core/needs.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace dal::core {

namespace {

constexpr double kLearned = 100.0;

// 芸ごとの練習できる段階（Trick の順）
constexpr std::array<GrowthStage, kTrickCount> kUnlockStage{
    GrowthStage::Puppy,  // Sit
    GrowthStage::Puppy,  // Paw
    GrowthStage::Young,  // Down
    GrowthStage::Young,  // Stay
    GrowthStage::Adult,  // Spin
};

std::size_t index_of(Trick trick)
{
    return static_cast<std::size_t>(trick);
}

} // namespace

bool is_unlocked(Trick trick, GrowthStage stage)
{
    return stage >= kUnlockStage[index_of(trick)];
}

std::vector<Trick> unlocked_tricks(GrowthStage stage)
{
    std::vector<Trick> tricks;
    for (std::size_t i = 0; i < kTrickCount; ++i) {
        const auto trick = static_cast<Trick>(i);
        if (is_unlocked(trick, stage)) {
            tricks.push_back(trick);
        }
    }
    return tricks;
}

bool is_learned(const DogState& state, Trick trick)
{
    return proficiency_of(state, trick) >= kLearned;
}

double& proficiency_of(DogState& state, Trick trick)
{
    return state.trick_proficiency[index_of(trick)];
}

double proficiency_of(const DogState& state, Trick trick)
{
    return state.trick_proficiency[index_of(trick)];
}

double training_gain(const DogState& state, const Tuning& tuning)
{
    const double factor = 0.5 + 0.5 * (mood(state.needs) + state.affection) / 200.0;
    return tuning.train_base * factor;
}

CareAvailability check_trick(const DogState& state, Care care, Trick trick, const ClockReading& now,
                             const Tuning& tuning)
{
    if (!is_trick_care(care)) {
        return check_care(state, care, now, tuning);
    }
    if (!is_unlocked(trick, state.stage)) {
        return {CareBlock::TrickLocked};
    }
    if (care == Care::Train && is_learned(state, trick)) {
        return {CareBlock::TrickLearned};
    }
    if (care == Care::PerformTrick && !is_learned(state, trick)) {
        return {CareBlock::TrickNotLearned};
    }
    return check_care(state, care, now, tuning);
}

TrickResult apply_trick(DogState& state, Care care, Trick trick, const ClockReading& now, const Tuning& tuning)
{
    TrickResult result;
    if (!is_trick_care(care)) {
        result.availability = apply_care(state, care, now, tuning);
        return result;
    }

    result.availability = check_trick(state, care, trick, now, tuning);
    if (!result.availability.available()) {
        return result;
    }

    // 上がり幅は、世話の効果（なつき度の上昇など）を反映する前の機嫌となつき度で決める
    const double gain = training_gain(state, tuning);
    commit_care(state, care, now, tuning);

    if (care == Care::Train) {
        double& proficiency = proficiency_of(state, trick);
        proficiency = std::min(proficiency + gain, kLearned);
        if (proficiency >= kLearned) {
            Event event{EventKind::LearnedTrick};
            event.trick = trick;
            result.events.push_back(event);
        }
    }
    return result;
}

} // namespace dal::core
