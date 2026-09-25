#include "core/behavior.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

using namespace std::chrono_literals;
using Catch::Approx;
using dal::core::Care;
using dal::core::ClockReading;
using dal::core::Destination;
using dal::core::DogState;
using dal::core::IntentKind;
using dal::core::NeedKind;
using dal::core::TimePoint;
using dal::core::Trick;
using dal::core::Tuning;

namespace {

const TimePoint kDay{std::chrono::sys_days{std::chrono::year{2026} / 9 / 26}};
const TimePoint kNoonJst = kDay + 3h;    // 12:00 JST
const TimePoint kNightJst = kDay + 13h;  // 22:00 JST

ClockReading jst(TimePoint utc)
{
    return {utc, std::chrono::hours{9}};
}

// 今の意図を kind にし、since を at にした犬
DogState dog_in(IntentKind kind, TimePoint at)
{
    DogState state;
    state.intent.kind = kind;
    state.intent.since = at;
    state.intent.id = 1;
    return state;
}

} // namespace

TEST_CASE("一日の中の時刻と夜の判定")
{
    const Tuning t;
    CHECK(dal::core::hours_of_day(jst(kNoonJst)) == Approx(12.0));
    CHECK(dal::core::hours_of_day(jst(kNightJst)) == Approx(22.0));
    CHECK_FALSE(is_night(12.0, t));
    CHECK(is_night(22.0, t));
    CHECK(is_night(3.0, t));
    CHECK_FALSE(is_night(6.0, t));
}

TEST_CASE("眠気が寝るしきい値以上なら寝る。夜はしきい値が低い")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Idle, kNoonJst);
    state.needs.sleepiness = 50.0;

    CHECK(decide_intent(state, jst(kNoonJst), t).kind == IntentKind::Idle);
    CHECK(decide_intent(state, jst(kNightJst), t).kind == IntentKind::Sleep);

    state.needs.sleepiness = 80.0;
    const auto sleep = decide_intent(state, jst(kNoonJst), t);
    CHECK(sleep.kind == IntentKind::Sleep);
    CHECK(sleep.destination == Destination::Bed);
}

TEST_CASE("寝ている間は、眠気が起きるしきい値に下がるまで寝続け、起きたら伸びをする")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Sleep, kNoonJst);
    state.needs.sleepiness = 10.0;  // 寝始めるしきい値より低くても寝続ける
    CHECK(decide_intent(state, jst(kNoonJst), t).kind == IntentKind::Sleep);

    state.needs.sleepiness = 5.0;
    CHECK(decide_intent(state, jst(kNoonJst), t).kind == IntentKind::Stretch);
}

TEST_CASE("欲求が高いとおねだりし、応えてもらえないと吠える")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Idle, kNoonJst);
    state.needs.hunger = 50.0;
    state.needs.loneliness = 70.0;

    const auto beg = decide_intent(state, jst(kNoonJst), t);
    CHECK(beg.kind == IntentKind::Beg);
    CHECK(beg.need == NeedKind::Loneliness);
    CHECK(beg.destination == Destination::Owner);

    state.intent = beg;
    CHECK(decide_intent(state, jst(kNoonJst + 119s), t).kind == IntentKind::Beg);
    CHECK(decide_intent(state, jst(kNoonJst + 2min), t).kind == IntentKind::Bark);
}

TEST_CASE("吠え終わったら、おねだりの時間を数え直す")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Bark, kNoonJst);
    state.needs.hunger = 70.0;

    finish_intent(state, 1, jst(kNoonJst + 5s), t);
    CHECK(state.intent.kind == IntentKind::Beg);
    CHECK(state.intent.since == kNoonJst + 5s);
}

TEST_CASE("退屈なら、しばらくじっとしてからうろうろする")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Idle, kNoonJst);
    state.needs.boredom = 30.0;

    CHECK(decide_intent(state, jst(kNoonJst + 19s), t).kind == IntentKind::Idle);
    const auto wander = decide_intent(state, jst(kNoonJst + 20s), t);
    CHECK(wander.kind == IntentKind::Wander);
    CHECK(wander.destination == Destination::Any);
}

TEST_CASE("散歩中はついていき、ときどき匂いを嗅ぐか掘る")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Follow, kNoonJst);
    state.walking = true;

    CHECK(decide_intent(state, jst(kNoonJst + 29s), t).kind == IntentKind::Follow);
    CHECK(decide_intent(state, jst(kNoonJst + 30s), t).kind == IntentKind::Sniff);

    state.needs.boredom = 60.0;
    CHECK(decide_intent(state, jst(kNoonJst + 30s), t).kind == IntentKind::Dig);

    SECTION("散歩中は眠くても寝ない")
    {
        state.needs.sleepiness = 100.0;
        CHECK(decide_intent(state, jst(kNoonJst), t).kind == IntentKind::Follow);
    }
    SECTION("匂いを嗅ぎ終わったら、ついていく")
    {
        state.intent.kind = IntentKind::Sniff;
        finish_intent(state, 1, jst(kNoonJst), t);
        CHECK(state.intent.kind == IntentKind::Follow);
    }
}

TEST_CASE("同じ状態の意図が選ばれたら、since と id を変えない")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Idle, kNoonJst);
    const auto same = decide_intent(state, jst(kNoonJst + 5s), t);

    CHECK(same.since == kNoonJst);
    CHECK(same.id == 1);
}

TEST_CASE("終了通知")
{
    const Tuning t;

    SECTION("id が今の意図と違えば無視する")
    {
        DogState state = dog_in(IntentKind::Eat, kNoonJst);
        finish_intent(state, 0, jst(kNoonJst), t);
        CHECK(state.intent.kind == IntentKind::Eat);
    }
    SECTION("状態の意図への通知は無視する")
    {
        DogState state = dog_in(IntentKind::Sleep, kNoonJst);
        state.needs.sleepiness = 50.0;
        finish_intent(state, 1, jst(kNoonJst), t);
        CHECK(state.intent.kind == IntentKind::Sleep);
    }
    SECTION("動作の意図が終わると、規則で次の意図になり id が増える")
    {
        DogState state = dog_in(IntentKind::Eat, kNoonJst);
        finish_intent(state, 1, jst(kNoonJst), t);
        CHECK(state.intent.kind == IntentKind::Idle);
        CHECK(state.intent.id == 2);
    }
}

TEST_CASE("終了通知が来なくても、動作の意図は一定時間で終わる")
{
    const Tuning t;
    DogState state = dog_in(IntentKind::Eat, kNoonJst);

    update_intent(state, jst(kNoonJst + 59s), t);
    CHECK(state.intent.kind == IntentKind::Eat);
    update_intent(state, jst(kNoonJst + 60s), t);
    CHECK(state.intent.kind == IntentKind::Idle);
}

TEST_CASE("世話を受け付けたときの意図")
{
    DogState state = dog_in(IntentKind::Idle, kNoonJst);

    start_care_intent(state, Care::Feed, Trick::Sit, jst(kNoonJst));
    CHECK(state.intent.kind == IntentKind::Eat);
    CHECK(state.intent.destination == Destination::Bowl);
    CHECK(state.intent.id == 2);

    start_care_intent(state, Care::Train, Trick::Paw, jst(kNoonJst));
    CHECK(state.intent.kind == IntentKind::Train);
    CHECK(state.intent.trick == Trick::Paw);

    start_care_intent(state, Care::Walk, Trick::Sit, jst(kNoonJst));
    CHECK(state.intent.kind == IntentKind::Follow);
}
