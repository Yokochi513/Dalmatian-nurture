#include "core/simulation.hpp"

#include "fake_clock.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

using namespace std::chrono_literals;
using Catch::Approx;
using dal::core::DogState;
using dal::core::Simulation;
using dal::core::TimePoint;
using dal::core::Tuning;
using dal::core::test::FakeClock;

namespace {

// 空腹だけが1秒に 1 増える
Tuning hunger_per_second()
{
    Tuning t;
    t.hunger_per_hour = 3600.0;
    t.exercise_per_hour = 0.0;
    t.boredom_per_hour = 0.0;
    t.loneliness_per_hour = 0.0;
    t.sleepiness_per_hour = 0.0;
    t.sleep_recovery_per_hour = 0.0;
    t.neglect_threshold = 1000.0;  // なつき度は下げない
    t.offline_need_cap = 50.0;
    t.offline_gap = 60s;
    return t;
}

const TimePoint kStart{std::chrono::sys_days{std::chrono::year{2026} / 9 / 26}};

} // namespace

TEST_CASE("step は1秒刻みで進め、端数を持ち越す")
{
    FakeClock clock{kStart};
    Simulation sim{clock, DogState{}, kStart, hunger_per_second()};

    clock.advance(2700ms);
    sim.step();
    CHECK(sim.state().needs.hunger == Approx(2.0));

    clock.advance(400ms);  // 端数 0.7 秒と合わせて 1.1 秒
    sim.step();
    CHECK(sim.state().needs.hunger == Approx(3.0));
}

TEST_CASE("時計が戻っても状態は変わらない")
{
    FakeClock clock{kStart};
    Simulation sim{clock, DogState{}, kStart, hunger_per_second()};

    clock.set(kStart - 1h);
    sim.step();
    CHECK(sim.state().needs.hunger == Approx(0.0));

    // 戻った時刻から改めて進む
    clock.advance(3s);
    sim.step();
    CHECK(sim.state().needs.hunger == Approx(3.0));
}

TEST_CASE("前回から offline_gap 以上空いた step は不在として扱う")
{
    FakeClock clock{kStart};
    Simulation sim{clock, DogState{}, kStart, hunger_per_second()};

    clock.advance(2min);
    sim.step();
    // 1秒刻みなら 100 まで増えるが、不在の上限 50 で止まる
    CHECK(sim.state().needs.hunger == Approx(50.0));
}

TEST_CASE("resume は最終終了時刻からの経過を不在として進める")
{
    FakeClock clock{kStart + 10s};
    Simulation sim{clock, DogState{}, kStart, hunger_per_second()};

    sim.resume();
    CHECK(sim.state().needs.hunger == Approx(10.0));

    // resume した時刻から step が続く
    clock.advance(1s);
    sim.step();
    CHECK(sim.state().needs.hunger == Approx(11.0));
}

TEST_CASE("do_care は時間を進めてから判定する")
{
    Tuning t = hunger_per_second();
    t.feed.min_need = 30.0;
    FakeClock clock{kStart};
    DogState state;
    state.needs.hunger = 25.0;
    Simulation sim{clock, state, kStart, t};

    clock.advance(5s);  // step() を呼ばなくても、do_care の中で空腹が 30 になる
    CHECK(sim.do_care(dal::core::Care::Feed).availability.available());
}

TEST_CASE("世話を受け付けると成長ポイントがたまり、成長したら出来事を返す")
{
    Tuning t = hunger_per_second();
    t.feed.min_need = 0.0;
    t.feed.growth_points = 10.0;
    t.pet.min_need = 0.0;
    t.pet.growth_points = 10.0;
    t.growth.young_at = 20.0;
    FakeClock clock{kStart};
    Simulation sim{clock, DogState{}, kStart, t};

    const auto first = sim.do_care(dal::core::Care::Feed);
    CHECK(first.events.empty());
    CHECK(sim.state().growth_points == Approx(10.0));

    const auto second = sim.do_care(dal::core::Care::Pet);
    REQUIRE(second.events.size() == 1);
    CHECK(second.events[0].kind == dal::core::EventKind::Grew);
    CHECK(second.events[0].stage == dal::core::GrowthStage::Young);
    CHECK(sim.state().stage == dal::core::GrowthStage::Young);
}

TEST_CASE("do_trick は習得の出来事と成長の出来事をまとめて返す")
{
    Tuning t = hunger_per_second();
    t.train_base = 100.0;
    t.train.growth_points = 30.0;
    t.growth.young_at = 30.0;
    FakeClock clock{kStart};
    DogState state;
    state.affection = 100.0;
    Simulation sim{clock, state, kStart, t};

    const auto result = sim.do_trick(dal::core::Care::Train, dal::core::Trick::Sit);
    REQUIRE(result.events.size() == 2);
    CHECK(result.events[0].kind == dal::core::EventKind::LearnedTrick);
    CHECK(result.events[1].kind == dal::core::EventKind::Grew);
}

TEST_CASE("断られた世話では成長ポイントはたまらない")
{
    Tuning t = hunger_per_second();
    t.feed.min_need = 50.0;
    FakeClock clock{kStart};
    Simulation sim{clock, DogState{}, kStart, t};

    const auto result = sim.do_care(dal::core::Care::Feed);
    CHECK_FALSE(result.availability.available());
    CHECK(sim.state().growth_points == Approx(0.0));
}

TEST_CASE("散歩中は運動の欲求が時間とともに減り、end_walk で止まる")
{
    Tuning t = hunger_per_second();
    t.walk_exercise_per_hour = 3600.0;  // 1秒に 1 減る
    t.walk.min_need = 0.0;
    FakeClock clock{kStart};
    DogState state;
    state.needs.exercise = 50.0;
    Simulation sim{clock, state, kStart, t};

    sim.do_care(dal::core::Care::Walk);
    clock.advance(10s);
    sim.step();
    CHECK(sim.state().needs.exercise == Approx(40.0));

    sim.end_walk();
    CHECK_FALSE(sim.state().walking);
    clock.advance(10s);
    sim.step();
    CHECK(sim.state().needs.exercise == Approx(40.0));  // 運動の増え方は 0
}

TEST_CASE("散歩中に終了していた場合、resume で散歩を終える")
{
    FakeClock clock{kStart + 1h};
    DogState state;
    state.walking = true;
    Simulation sim{clock, state, kStart, hunger_per_second()};

    sim.resume();
    CHECK_FALSE(sim.state().walking);
}

TEST_CASE("最終終了時刻が未来なら resume は何もしない")
{
    FakeClock clock{kStart};
    Simulation sim{clock, DogState{}, kStart + 1h, hunger_per_second()};

    sim.resume();
    CHECK(sim.state().needs.hunger == Approx(0.0));
}
