#include "core/save.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>

using namespace std::chrono_literals;
using Catch::Approx;
using dal::core::Care;
using dal::core::DogState;
using dal::core::GrowthStage;
using dal::core::IntentKind;
using dal::core::LoadError;
using dal::core::SaveData;
using dal::core::TimePoint;
using dal::core::Trick;

namespace {

const TimePoint kSaved{std::chrono::sys_days{std::chrono::year{2026} / 9 / 26} + 3h + 4min + 5s + 678ms};
const std::chrono::local_days kLocalDay{std::chrono::year{2026} / 9 / 26};

SaveData sample()
{
    DogState dog;
    dog.name = "ポチ";
    dog.needs = {12.5, 20.0, 30.0, 40.0, 50.0};
    dog.affection = 33.0;
    dog.stage = GrowthStage::Young;
    record_of(dog, Care::Feed).last_done = kSaved - 1h;
    record_of(dog, Care::Feed).count_today = 2;
    dog.care_day = kLocalDay;
    dog.growth_points = 120.0;
    dog.growth_today = 40.0;
    dog.growth_day = kLocalDay;
    dog.trick_proficiency[static_cast<std::size_t>(Trick::Paw)] = 75.0;
    dog.walking = true;
    dog.intent.kind = IntentKind::Sleep;
    return {dog, kSaved};
}

// 設計書（save.md）の例と同じ形の JSON
const char* const kDocumentExample = R"({
  "version": 1,
  "last_saved": "2026-09-26T03:00:00.000Z",
  "dog": {
    "name": "ポチ",
    "needs": { "hunger": 42.5, "exercise": 50.0, "boredom": 60.0, "loneliness": 60.0, "sleepiness": 20.0 },
    "affection": 20.0,
    "stage": "puppy",
    "care": {
      "day": "2026-09-26",
      "records": {
        "feed": { "last_done": "2026-09-26T02:30:00.000Z", "count_today": 1 },
        "pet": { "last_done": null, "count_today": 0 },
        "play": { "last_done": null, "count_today": 0 },
        "walk": { "last_done": null, "count_today": 0 },
        "train": { "last_done": null, "count_today": 0 },
        "perform_trick": { "last_done": null, "count_today": 0 }
      }
    },
    "growth": { "points": 10.0, "today": 10.0, "day": "2026-09-26" },
    "tricks": { "sit": 0.0, "paw": 0.0, "down": 0.0, "stay": 0.0, "spin": 0.0 },
    "memo": "知らないキーは無視する"
  }
})";

// 例の一部を置き換えた JSON
std::string example_with(const std::string& from, const std::string& to)
{
    std::string text = kDocumentExample;
    const auto pos = text.find(from);
    REQUIRE(pos != std::string::npos);
    text.replace(pos, from.size(), to);
    return text;
}

LoadError error_of(const std::string& text)
{
    return dal::core::deserialize(text).error;
}

} // namespace

TEST_CASE("書き出したものを読み込むと元に戻る（walking と intent を除く）")
{
    const SaveData original = sample();
    const auto result = dal::core::deserialize(dal::core::serialize(original));

    REQUIRE(result.error == LoadError::None);
    REQUIRE(result.data.has_value());
    const SaveData& loaded = *result.data;
    const DogState& dog = loaded.dog;

    CHECK(loaded.last_saved == kSaved);
    CHECK(dog.name == "ポチ");
    CHECK(dog.needs.hunger == Approx(12.5));
    CHECK(dog.needs.sleepiness == Approx(50.0));
    CHECK(dog.affection == Approx(33.0));
    CHECK(dog.stage == GrowthStage::Young);
    CHECK(record_of(dog, Care::Feed).last_done == kSaved - 1h);
    CHECK(record_of(dog, Care::Feed).count_today == 2);
    CHECK_FALSE(record_of(dog, Care::Pet).last_done.has_value());
    CHECK(dog.care_day == kLocalDay);
    CHECK(dog.growth_points == Approx(120.0));
    CHECK(dog.growth_today == Approx(40.0));
    CHECK(dog.growth_day == kLocalDay);
    CHECK(dog.trick_proficiency[static_cast<std::size_t>(Trick::Paw)] == Approx(75.0));

    CHECK_FALSE(dog.walking);
    CHECK(dog.intent.kind == IntentKind::Idle);
}

TEST_CASE("時刻は ISO 8601 の UTC で書き出す")
{
    const std::string text = dal::core::serialize(sample());
    CHECK(text.find("\"last_saved\": \"2026-09-26T03:04:05.678Z\"") != std::string::npos);
    CHECK(text.find("\"version\": 1") != std::string::npos);
    CHECK(text.find("\"stage\": \"young\"") != std::string::npos);
}

TEST_CASE("設計書の例を読み込める（知らないキーは無視する）")
{
    const auto result = dal::core::deserialize(kDocumentExample);

    REQUIRE(result.error == LoadError::None);
    const DogState& dog = result.data->dog;
    CHECK(dog.needs.hunger == Approx(42.5));
    CHECK(dog.stage == GrowthStage::Puppy);
    CHECK(record_of(dog, Care::Feed).count_today == 1);
}

TEST_CASE("JSON として解釈できなければ Parse")
{
    CHECK(error_of("{ \"version\": ") == LoadError::Parse);
}

TEST_CASE("形式の番号が違えば Version")
{
    CHECK(error_of(example_with("\"version\": 1", "\"version\": 2")) == LoadError::Version);
}

TEST_CASE("項目・型・値に問題があれば Invalid")
{
    CHECK(error_of(example_with("\"affection\": 20.0,", "")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"affection\": 20.0", "\"affection\": \"20\"")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"affection\": 20.0", "\"affection\": 120.0")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"stage\": \"puppy\"", "\"stage\": \"senior\"")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"name\": \"ポチ\"", "\"name\": \"\"")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"count_today\": 1", "\"count_today\": -1")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"points\": 10.0", "\"points\": -1.0")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"sit\": 0.0, ", "")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"pet\": { \"last_done\": null, \"count_today\": 0 },", "")) == LoadError::Invalid);
}

TEST_CASE("時刻と日付の書式が違えば Invalid")
{
    CHECK(error_of(example_with("2026-09-26T03:00:00.000Z", "2026-09-26 03:00:00")) == LoadError::Invalid);
    CHECK(error_of(example_with("2026-09-26T03:00:00.000Z", "2026-09-26T25:00:00.000Z")) == LoadError::Invalid);
    CHECK(error_of(example_with("\"day\": \"2026-09-26\" },", "\"day\": \"2026-02-30\" },")) == LoadError::Invalid);
}

TEST_CASE("読めなかった理由には項目の場所が入る")
{
    const auto result = dal::core::deserialize(example_with("\"affection\": 20.0", "\"affection\": 120.0"));
    CHECK(result.message.find("$.dog.affection") != std::string::npos);
}
