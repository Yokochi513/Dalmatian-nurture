#include "core/care.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

using namespace std::chrono_literals;
using Catch::Approx;
using dal::core::Care;
using dal::core::CareBlock;
using dal::core::ClockReading;
using dal::core::DogState;
using dal::core::TimePoint;
using dal::core::Tuning;

namespace {

// 2026-09-26 03:00 UTC ＝ 12:00 JST
const TimePoint kNoonJst{std::chrono::sys_days{std::chrono::year{2026} / 9 / 26} + 3h};

ClockReading jst(TimePoint utc)
{
    return {utc, std::chrono::hours{9}};
}

Tuning feed_tuning()
{
    Tuning t;
    t.feed.cooldown = 1h;
    t.feed.daily_limit = 2;
    t.feed.min_need = 30.0;
    t.feed.relief = 50.0;
    t.feed.affection_gain = 5.0;
    return t;
}

DogState hungry_dog()
{
    DogState state;
    state.needs.hunger = 60.0;
    state.affection = 10.0;
    return state;
}

} // namespace

TEST_CASE("世話を受け付けると、欲求が減り、なつき度が上がり、記録が残る")
{
    DogState state = hungry_dog();
    const auto result = apply_care(state, Care::Feed, jst(kNoonJst), feed_tuning());

    CHECK(result.available());
    CHECK(state.needs.hunger == Approx(10.0));
    CHECK(state.affection == Approx(15.0));
    CHECK(record_of(state, Care::Feed).last_done == kNoonJst);
    CHECK(record_of(state, Care::Feed).count_today == 1);
}

TEST_CASE("クールダウン中は、残り時間とともに断る")
{
    const Tuning t = feed_tuning();
    DogState state = hungry_dog();
    state.needs.hunger = 100.0;
    apply_care(state, Care::Feed, jst(kNoonJst), t);

    const auto soon = check_care(state, Care::Feed, jst(kNoonJst + 30min), t);
    CHECK(soon.block == CareBlock::Cooldown);
    CHECK(soon.remaining == 30min);

    CHECK(check_care(state, Care::Feed, jst(kNoonJst + 1h), t).available());
}

TEST_CASE("1日の上限に達したら、現地の0時まで断る")
{
    Tuning t = feed_tuning();
    t.feed.cooldown = 0s;
    t.feed.relief = 0.0;
    DogState state = hungry_dog();
    apply_care(state, Care::Feed, jst(kNoonJst), t);
    apply_care(state, Care::Feed, jst(kNoonJst), t);

    const auto limited = check_care(state, Care::Feed, jst(kNoonJst), t);
    CHECK(limited.block == CareBlock::DailyLimit);
    CHECK(limited.remaining == 12h);  // 12:00 JST → 翌 0:00 JST

    // 23:59 JST はまだ同じ日、0:00 JST（15:00 UTC）から次の日
    CHECK(check_care(state, Care::Feed, jst(kNoonJst + 11h + 59min), t).block == CareBlock::DailyLimit);
    CHECK(check_care(state, Care::Feed, jst(kNoonJst + 12h), t).available());
}

TEST_CASE("日付が変わって世話を受け付けると、すべての世話の回数が数え直される")
{
    Tuning t = feed_tuning();
    t.feed.cooldown = 0s;
    t.feed.relief = 0.0;
    t.pet.cooldown = 0s;
    t.pet.min_need = 0.0;
    DogState state = hungry_dog();
    apply_care(state, Care::Feed, jst(kNoonJst), t);
    apply_care(state, Care::Pet, jst(kNoonJst), t);

    apply_care(state, Care::Feed, jst(kNoonJst + 12h), t);
    CHECK(record_of(state, Care::Feed).count_today == 1);
    CHECK(record_of(state, Care::Pet).count_today == 0);
}

TEST_CASE("対応する欲求が満たされていれば断る")
{
    DogState state = hungry_dog();
    state.needs.hunger = 29.0;

    CHECK(check_care(state, Care::Feed, jst(kNoonJst), feed_tuning()).block == CareBlock::NotNeeded);
}

TEST_CASE("理由が複数あるときは、1日の上限をクールダウンより優先する")
{
    Tuning t = feed_tuning();
    t.feed.daily_limit = 1;
    DogState state = hungry_dog();
    apply_care(state, Care::Feed, jst(kNoonJst), t);

    CHECK(check_care(state, Care::Feed, jst(kNoonJst), t).block == CareBlock::DailyLimit);
}

TEST_CASE("断ったときは状態を変えない")
{
    DogState state = hungry_dog();
    state.needs.hunger = 10.0;
    apply_care(state, Care::Feed, jst(kNoonJst), feed_tuning());

    CHECK(state.needs.hunger == Approx(10.0));
    CHECK(state.affection == Approx(10.0));
    CHECK_FALSE(record_of(state, Care::Feed).last_done.has_value());
}

TEST_CASE("時計が戻っても、クールダウンの残り時間はクールダウンを超えない")
{
    const Tuning t = feed_tuning();
    DogState state = hungry_dog();
    apply_care(state, Care::Feed, jst(kNoonJst), t);

    const auto result = check_care(state, Care::Feed, jst(kNoonJst - 5h), t);
    CHECK(result.block == CareBlock::Cooldown);
    CHECK(result.remaining == 1h);
}

TEST_CASE("散歩を始めると散歩中になり、散歩中は散歩を始められない")
{
    Tuning t;
    t.walk.min_need = 0.0;
    DogState state;
    apply_care(state, Care::Walk, jst(kNoonJst), t);

    CHECK(state.walking);
    CHECK(check_care(state, Care::Walk, jst(kNoonJst + 24h), t).block == CareBlock::AlreadyWalking);
}
