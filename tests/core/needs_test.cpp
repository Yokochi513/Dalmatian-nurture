#include "core/needs.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

using namespace std::chrono_literals;
using Catch::Approx;
using dal::core::Activity;
using dal::core::DogState;
using dal::core::Needs;
using dal::core::Tuning;

namespace {

// 計算しやすい数値（どの欲求も1時間に 10 増える）
Tuning simple_tuning()
{
    Tuning t;
    t.hunger_per_hour = 10.0;
    t.exercise_per_hour = 10.0;
    t.boredom_per_hour = 10.0;
    t.loneliness_per_hour = 10.0;
    t.sleepiness_per_hour = 10.0;
    t.sleep_recovery_per_hour = 20.0;
    t.walk_exercise_per_hour = 30.0;
    t.neglect_threshold = 80.0;
    t.affection_loss_per_hour = 2.0;
    t.offline_need_cap = 80.0;
    t.offline_affection_loss_max = 10.0;
    return t;
}

} // namespace

TEST_CASE("起きている間はすべての欲求が変化率どおりに増える")
{
    Needs needs;
    advance_needs(needs, Activity::Awake, 2h, simple_tuning());

    CHECK(needs.hunger == Approx(20.0));
    CHECK(needs.exercise == Approx(20.0));
    CHECK(needs.boredom == Approx(20.0));
    CHECK(needs.loneliness == Approx(20.0));
    CHECK(needs.sleepiness == Approx(20.0));
}

TEST_CASE("欲求は 100 を超えない")
{
    Needs needs{.hunger = 95.0};
    advance_needs(needs, Activity::Awake, 1h, simple_tuning());

    CHECK(needs.hunger == Approx(100.0));
}

TEST_CASE("寝ている間は眠気だけが減る")
{
    Needs needs{.hunger = 10.0, .sleepiness = 50.0};
    advance_needs(needs, Activity::Sleeping, 1h, simple_tuning());

    CHECK(needs.sleepiness == Approx(30.0));
    CHECK(needs.hunger == Approx(20.0));
}

TEST_CASE("散歩中は運動の欲求が減り、0 を下回らない")
{
    Needs needs{.exercise = 20.0};
    advance_needs(needs, Activity::Walking, 1h, simple_tuning());

    CHECK(needs.exercise == Approx(0.0));
}

TEST_CASE("起動中のなつき度は、いずれかの欲求がしきい値以上のときだけ下がる")
{
    const Tuning t = simple_tuning();

    SECTION("しきい値未満なら下がらない")
    {
        double affection = 50.0;
        apply_neglect(affection, Needs{.hunger = 79.0}, 1h, t);
        CHECK(affection == Approx(50.0));
    }
    SECTION("しきい値以上なら下がる")
    {
        double affection = 50.0;
        apply_neglect(affection, Needs{.boredom = 80.0}, 1h, t);
        CHECK(affection == Approx(48.0));
    }
}

TEST_CASE("不在中の欲求は上限までしか増えない")
{
    DogState state;
    state.needs = Needs{.hunger = 30.0, .exercise = 90.0};
    apply_absence(state, 168h, simple_tuning());

    CHECK(state.needs.hunger == Approx(80.0));
    // すでに上限を超えていれば、そのまま
    CHECK(state.needs.exercise == Approx(90.0));
}

TEST_CASE("不在中は寝ていたものとして眠気が減る")
{
    DogState state;
    state.needs.sleepiness = 50.0;
    apply_absence(state, 2h, simple_tuning());

    CHECK(state.needs.sleepiness == Approx(10.0));
}

TEST_CASE("不在中のなつき度は、欲求がしきい値以上だった時間に応じて下がる")
{
    const Tuning t = simple_tuning();

    SECTION("欲求がしきい値に達してからの時間だけ下がる")
    {
        DogState state;
        state.affection = 50.0;
        state.needs.hunger = 60.0;           // 2時間でしきい値 80 に達する
        apply_absence(state, 5h, t);        // しきい値以上は 3 時間
        CHECK(state.affection == Approx(44.0));
    }
    SECTION("眠気は最初しきい値以上でも、減って下回るまでの間だけ数える")
    {
        DogState state;
        state.affection = 50.0;
        state.needs.sleepiness = 100.0;      // 1時間でしきい値 80 を下回る
        apply_absence(state, 3h, t);
        CHECK(state.affection == Approx(48.0));
    }
    SECTION("眠気と他の欲求の時間が重なる分は二重に数えない")
    {
        DogState state;
        state.affection = 50.0;
        state.needs.sleepiness = 100.0;      // 0〜1時間
        state.needs.hunger = 75.0;           // 0.5時間以降
        apply_absence(state, 2h, t);        // 和集合は 0〜2時間
        CHECK(state.affection == Approx(46.0));
    }
    SECTION("上限がしきい値より低ければ、増える欲求では下がらない")
    {
        Tuning low_cap = t;
        low_cap.offline_need_cap = 70.0;
        DogState state;
        state.affection = 50.0;
        state.needs.hunger = 30.0;
        apply_absence(state, 100h, low_cap);
        CHECK(state.affection == Approx(50.0));
    }
    SECTION("1回の不在での低下には上限がある")
    {
        DogState state;
        state.affection = 50.0;
        state.needs.hunger = 80.0;
        apply_absence(state, 168h, t);
        CHECK(state.affection == Approx(40.0));
    }
}

TEST_CASE("上限に届かない範囲では、起動中の計算と不在の計算で眠気以外の欲求が一致する")
{
    const Tuning t = simple_tuning();
    Needs online{.hunger = 10.0, .exercise = 20.0, .boredom = 30.0, .loneliness = 40.0};
    DogState offline;
    offline.needs = online;

    for (int i = 0; i < 3 * 3600; ++i) {
        advance_needs(online, Activity::Awake, 1s, t);
    }
    apply_absence(offline, 3h, t);

    CHECK(online.hunger == Approx(offline.needs.hunger));
    CHECK(online.exercise == Approx(offline.needs.exercise));
    CHECK(online.boredom == Approx(offline.needs.boredom));
    CHECK(online.loneliness == Approx(offline.needs.loneliness));
}

TEST_CASE("機嫌は 100 から欲求の平均を引いた値")
{
    CHECK(dal::core::mood(Needs{}) == Approx(100.0));
    CHECK(dal::core::mood(Needs{.hunger = 50.0, .exercise = 50.0}) == Approx(80.0));
}
