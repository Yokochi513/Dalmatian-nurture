#include "core/growth.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

using Catch::Approx;
using dal::core::DogState;
using dal::core::EventKind;
using dal::core::GrowthStage;
using dal::core::Tuning;

namespace {

const std::chrono::local_days kDay1{std::chrono::year{2026} / 9 / 26};
const std::chrono::local_days kDay2 = kDay1 + std::chrono::days{1};

Tuning growth_tuning()
{
    Tuning t;
    t.growth.young_at = 30.0;
    t.growth.adult_at = 100.0;
    t.growth.daily_cap = 50.0;
    return t;
}

} // namespace

TEST_CASE("しきい値に届くまでは成長しない")
{
    DogState state;
    const auto events = add_growth(state, 29.0, kDay1, growth_tuning());

    CHECK(events.empty());
    CHECK(state.stage == GrowthStage::Puppy);
    CHECK(state.growth_points == Approx(29.0));
}

TEST_CASE("しきい値に届いたら成長し、Grew を返す")
{
    DogState state;
    const auto events = add_growth(state, 30.0, kDay1, growth_tuning());

    REQUIRE(events.size() == 1);
    CHECK(events[0].kind == EventKind::Grew);
    CHECK(events[0].stage == GrowthStage::Young);
    CHECK(state.stage == GrowthStage::Young);
}

TEST_CASE("1日にたまる成長ポイントには上限がある")
{
    DogState state;
    add_growth(state, 40.0, kDay1, growth_tuning());
    add_growth(state, 40.0, kDay1, growth_tuning());

    CHECK(state.growth_points == Approx(50.0));
    CHECK(state.growth_today == Approx(50.0));
}

TEST_CASE("日付が変わると、1日の上限まで再びたまる")
{
    DogState state;
    add_growth(state, 50.0, kDay1, growth_tuning());
    add_growth(state, 20.0, kDay2, growth_tuning());

    CHECK(state.growth_points == Approx(70.0));
    CHECK(state.growth_today == Approx(20.0));
    CHECK(state.growth_day == kDay2);
}

TEST_CASE("1回で2段階を超えたら、段階ごとに Grew を返す")
{
    Tuning t = growth_tuning();
    t.growth.daily_cap = 1000.0;
    DogState state;
    const auto events = add_growth(state, 100.0, kDay1, t);

    REQUIRE(events.size() == 2);
    CHECK(events[0].stage == GrowthStage::Young);
    CHECK(events[1].stage == GrowthStage::Adult);
}

TEST_CASE("次の段階までの進み具合")
{
    const Tuning t = growth_tuning();
    DogState state;

    state.growth_points = 15.0;
    CHECK(growth_progress(state, t) == Approx(0.5));

    state.stage = GrowthStage::Young;
    state.growth_points = 65.0;  // 30〜100 の半分
    CHECK(growth_progress(state, t) == Approx(0.5));

    state.stage = GrowthStage::Adult;
    CHECK(growth_progress(state, t) == Approx(1.0));
}
