#include "core/tricks.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <vector>

using namespace std::chrono_literals;
using Catch::Approx;
using dal::core::Care;
using dal::core::CareBlock;
using dal::core::ClockReading;
using dal::core::DogState;
using dal::core::EventKind;
using dal::core::GrowthStage;
using dal::core::Needs;
using dal::core::TimePoint;
using dal::core::Trick;
using dal::core::Tuning;

namespace {

const TimePoint kNoonJst{std::chrono::sys_days{std::chrono::year{2026} / 9 / 26} + 3h};

ClockReading jst(TimePoint utc)
{
    return {utc, std::chrono::hours{9}};
}

// 機嫌 100・なつき度 100 なら上がり幅は train_base（= 25）そのまま
DogState happy_dog()
{
    DogState state;
    state.affection = 100.0;
    return state;
}

Tuning trick_tuning()
{
    Tuning t;
    t.train_base = 25.0;
    t.train.cooldown = 0s;
    t.train.daily_limit = 100;
    t.train.relief = 0.0;
    t.train.affection_gain = 0.0;
    t.perform_trick.cooldown = 0s;
    t.perform_trick.daily_limit = 100;
    t.perform_trick.affection_gain = 3.0;
    return t;
}

} // namespace

TEST_CASE("芸は成長段階に応じて解禁される")
{
    CHECK(unlocked_tricks(GrowthStage::Puppy) == std::vector<Trick>{Trick::Sit, Trick::Paw});
    CHECK(unlocked_tricks(GrowthStage::Young)
          == std::vector<Trick>{Trick::Sit, Trick::Paw, Trick::Down, Trick::Stay});
    CHECK(unlocked_tricks(GrowthStage::Adult).size() == 5);
}

TEST_CASE("練習の上がり幅は機嫌となつき度で 0.5 倍〜1 倍になる")
{
    const Tuning t = trick_tuning();

    DogState best = happy_dog();
    CHECK(training_gain(best, t) == Approx(25.0));

    DogState worst;
    worst.needs = Needs{100.0, 100.0, 100.0, 100.0, 100.0};  // 機嫌 0
    worst.affection = 0.0;
    CHECK(training_gain(worst, t) == Approx(12.5));
}

TEST_CASE("しつけると習熟度が上がり、100 で習得して LearnedTrick を返す")
{
    const Tuning t = trick_tuning();
    DogState state = happy_dog();

    for (int i = 0; i < 3; ++i) {
        const auto result = apply_trick(state, Care::Train, Trick::Sit, jst(kNoonJst), t);
        CHECK(result.availability.available());
        CHECK(result.events.empty());
    }
    CHECK(proficiency_of(state, Trick::Sit) == Approx(75.0));

    const auto last = apply_trick(state, Care::Train, Trick::Sit, jst(kNoonJst), t);
    REQUIRE(last.events.size() == 1);
    CHECK(last.events[0].kind == EventKind::LearnedTrick);
    CHECK(last.events[0].trick == Trick::Sit);
    CHECK(is_learned(state, Trick::Sit));
}

TEST_CASE("習熟度は 100 を超えない")
{
    const Tuning t = trick_tuning();
    DogState state = happy_dog();
    proficiency_of(state, Trick::Sit) = 90.0;

    apply_trick(state, Care::Train, Trick::Sit, jst(kNoonJst), t);
    CHECK(proficiency_of(state, Trick::Sit) == Approx(100.0));
}

TEST_CASE("芸ごとの理由")
{
    const Tuning t = trick_tuning();
    DogState state = happy_dog();

    SECTION("解禁前の芸は練習も披露もできない")
    {
        CHECK(check_trick(state, Care::Train, Trick::Down, jst(kNoonJst), t).block == CareBlock::TrickLocked);
        CHECK(check_trick(state, Care::PerformTrick, Trick::Down, jst(kNoonJst), t).block
              == CareBlock::TrickLocked);
    }
    SECTION("習得済みの芸は練習できない")
    {
        proficiency_of(state, Trick::Sit) = 100.0;
        CHECK(check_trick(state, Care::Train, Trick::Sit, jst(kNoonJst), t).block == CareBlock::TrickLearned);
    }
    SECTION("未習得の芸は披露できない")
    {
        CHECK(check_trick(state, Care::PerformTrick, Trick::Sit, jst(kNoonJst), t).block
              == CareBlock::TrickNotLearned);
    }
}

TEST_CASE("芸ごとの理由はクールダウンより先に判定する")
{
    Tuning t = trick_tuning();
    t.train.cooldown = 15min;
    DogState state = happy_dog();
    apply_trick(state, Care::Train, Trick::Sit, jst(kNoonJst), t);

    CHECK(check_trick(state, Care::Train, Trick::Down, jst(kNoonJst), t).block == CareBlock::TrickLocked);
}

TEST_CASE("クールダウンは芸ごとではなく、しつけ全体で共有する")
{
    Tuning t = trick_tuning();
    t.train.cooldown = 15min;
    DogState state = happy_dog();
    apply_trick(state, Care::Train, Trick::Sit, jst(kNoonJst), t);

    const auto result = check_trick(state, Care::Train, Trick::Paw, jst(kNoonJst), t);
    CHECK(result.block == CareBlock::Cooldown);
    CHECK(result.remaining == 15min);
}

TEST_CASE("芸をさせると、なつき度が上がり、習熟度は変わらない")
{
    const Tuning t = trick_tuning();
    DogState state;
    state.affection = 50.0;
    proficiency_of(state, Trick::Sit) = 100.0;

    const auto result = apply_trick(state, Care::PerformTrick, Trick::Sit, jst(kNoonJst), t);
    CHECK(result.availability.available());
    CHECK(state.affection == Approx(53.0));
    CHECK(proficiency_of(state, Trick::Sit) == Approx(100.0));
}

TEST_CASE("しつける・芸をさせるは、芸を指定しないと実行できない")
{
    DogState state = happy_dog();
    CHECK(apply_care(state, Care::Train, jst(kNoonJst), trick_tuning()).block == CareBlock::TrickNotSpecified);
}
