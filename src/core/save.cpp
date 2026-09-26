#include "core/save.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>

namespace dal::core {

namespace {

using nlohmann::json;

// 検査に通らなかったことを表す。deserialize の中だけで使う
class InvalidSave : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

constexpr std::array<std::string_view, 3> kStageNames{"puppy", "young", "adult"};
constexpr std::array<std::string_view, kCareCount> kCareNames{"feed", "pet", "play", "walk", "train", "perform_trick"};
constexpr std::array<std::string_view, kTrickCount> kTrickNames{"sit", "paw", "down", "stay", "spin"};

template <std::size_t N>
std::size_t index_of_name(const std::array<std::string_view, N>& names, const std::string& name, const std::string& path)
{
    for (std::size_t i = 0; i < N; ++i) {
        if (names[i] == name) {
            return i;
        }
    }
    throw InvalidSave(std::format("{}: 知らない値 \"{}\"", path, name));
}

// ---- 時刻と日付の文字列 ----

std::string format_day(std::chrono::year_month_day ymd)
{
    return std::format("{:04}-{:02}-{:02}", static_cast<int>(ymd.year()), static_cast<unsigned>(ymd.month()),
                       static_cast<unsigned>(ymd.day()));
}

std::string format_time(TimePoint time)
{
    const auto day = std::chrono::floor<std::chrono::days>(time);
    const std::chrono::hh_mm_ss hms{time - day};
    return std::format("{}T{:02}:{:02}:{:02}.{:03}Z", format_day(std::chrono::year_month_day{day}),
                       hms.hours().count(), hms.minutes().count(), hms.seconds().count(),
                       hms.subseconds().count());
}

std::string format_local_day(std::chrono::local_days day)
{
    return format_day(std::chrono::year_month_day{std::chrono::sys_days{day.time_since_epoch()}});
}

// text[pos, pos + len) を数字として読む
int read_digits(const std::string& text, std::size_t pos, std::size_t len, const std::string& path)
{
    int value = 0;
    const char* first = text.data() + pos;
    const char* last = first + len;
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        throw InvalidSave(std::format("{}: 書式が違う \"{}\"", path, text));
    }
    return value;
}

void expect_char(const std::string& text, std::size_t pos, char c, const std::string& path)
{
    if (text[pos] != c) {
        throw InvalidSave(std::format("{}: 書式が違う \"{}\"", path, text));
    }
}

// "YYYY-MM-DD"（text の先頭から）
std::chrono::sys_days parse_day_prefix(const std::string& text, const std::string& path)
{
    if (text.size() < 10) {
        throw InvalidSave(std::format("{}: 書式が違う \"{}\"", path, text));
    }
    expect_char(text, 4, '-', path);
    expect_char(text, 7, '-', path);
    const std::chrono::year_month_day ymd{
        std::chrono::year{read_digits(text, 0, 4, path)},
        std::chrono::month{static_cast<unsigned>(read_digits(text, 5, 2, path))},
        std::chrono::day{static_cast<unsigned>(read_digits(text, 8, 2, path))},
    };
    if (!ymd.ok()) {
        throw InvalidSave(std::format("{}: 存在しない日付 \"{}\"", path, text));
    }
    return std::chrono::sys_days{ymd};
}

std::chrono::local_days parse_local_day(const std::string& text, const std::string& path)
{
    if (text.size() != 10) {
        throw InvalidSave(std::format("{}: 書式が違う \"{}\"", path, text));
    }
    return std::chrono::local_days{parse_day_prefix(text, path).time_since_epoch()};
}

// "YYYY-MM-DDTHH:MM:SS.mmmZ"
TimePoint parse_time(const std::string& text, const std::string& path)
{
    if (text.size() != 24) {
        throw InvalidSave(std::format("{}: 書式が違う \"{}\"", path, text));
    }
    const auto day = parse_day_prefix(text, path);
    expect_char(text, 10, 'T', path);
    expect_char(text, 13, ':', path);
    expect_char(text, 16, ':', path);
    expect_char(text, 19, '.', path);
    expect_char(text, 23, 'Z', path);
    const int hours = read_digits(text, 11, 2, path);
    const int minutes = read_digits(text, 14, 2, path);
    const int seconds = read_digits(text, 17, 2, path);
    const int millis = read_digits(text, 20, 3, path);
    if (hours > 23 || minutes > 59 || seconds > 59) {
        throw InvalidSave(std::format("{}: 存在しない時刻 \"{}\"", path, text));
    }
    return TimePoint{day} + std::chrono::hours{hours} + std::chrono::minutes{minutes}
        + std::chrono::seconds{seconds} + std::chrono::milliseconds{millis};
}

// ---- 項目の読み取り ----

const json& field(const json& object, const char* key, const std::string& path)
{
    if (!object.is_object()) {
        throw InvalidSave(std::format("{}: オブジェクトでない", path));
    }
    const auto it = object.find(key);
    if (it == object.end()) {
        throw InvalidSave(std::format("{}.{}: 項目がない", path, key));
    }
    return *it;
}

double read_number(const json& object, const char* key, const std::string& path, double min, double max)
{
    const json& value = field(object, key, path);
    if (!value.is_number()) {
        throw InvalidSave(std::format("{}.{}: 数値でない", path, key));
    }
    const double number = value.get<double>();
    if (number < min || number > max) {
        throw InvalidSave(std::format("{}.{}: 範囲外 {}", path, key, number));
    }
    return number;
}

double read_percent(const json& object, const char* key, const std::string& path)
{
    return read_number(object, key, path, 0.0, 100.0);
}

double read_non_negative(const json& object, const char* key, const std::string& path)
{
    return read_number(object, key, path, 0.0, std::numeric_limits<double>::max());
}

std::string read_string(const json& object, const char* key, const std::string& path)
{
    const json& value = field(object, key, path);
    if (!value.is_string()) {
        throw InvalidSave(std::format("{}.{}: 文字列でない", path, key));
    }
    return value.get<std::string>();
}

// ---- DogState ----

json needs_to_json(const Needs& needs)
{
    return {
        {"hunger", needs.hunger},
        {"exercise", needs.exercise},
        {"boredom", needs.boredom},
        {"loneliness", needs.loneliness},
        {"sleepiness", needs.sleepiness},
    };
}

Needs needs_from_json(const json& j, const std::string& path)
{
    return Needs{
        .hunger = read_percent(j, "hunger", path),
        .exercise = read_percent(j, "exercise", path),
        .boredom = read_percent(j, "boredom", path),
        .loneliness = read_percent(j, "loneliness", path),
        .sleepiness = read_percent(j, "sleepiness", path),
    };
}

json dog_to_json(const DogState& dog)
{
    json records = json::object();
    for (std::size_t i = 0; i < kCareCount; ++i) {
        const CareRecord& record = dog.care_records[i];
        records[std::string{kCareNames[i]}] = {
            {"last_done", record.last_done ? json(format_time(*record.last_done)) : json(nullptr)},
            {"count_today", record.count_today},
        };
    }

    json tricks = json::object();
    for (std::size_t i = 0; i < kTrickCount; ++i) {
        tricks[std::string{kTrickNames[i]}] = dog.trick_proficiency[i];
    }

    return {
        {"name", dog.name},
        {"needs", needs_to_json(dog.needs)},
        {"affection", dog.affection},
        {"stage", std::string{kStageNames[static_cast<std::size_t>(dog.stage)]}},
        {"care", {{"day", format_local_day(dog.care_day)}, {"records", records}}},
        {"growth",
         {{"points", dog.growth_points}, {"today", dog.growth_today}, {"day", format_local_day(dog.growth_day)}}},
        {"tricks", tricks},
    };
}

DogState dog_from_json(const json& j, const std::string& path)
{
    DogState dog;

    dog.name = read_string(j, "name", path);
    if (dog.name.empty()) {
        throw InvalidSave(std::format("{}.name: 空", path));
    }
    dog.needs = needs_from_json(field(j, "needs", path), path + ".needs");
    dog.affection = read_percent(j, "affection", path);
    dog.stage = static_cast<GrowthStage>(index_of_name(kStageNames, read_string(j, "stage", path), path + ".stage"));

    const std::string care_path = path + ".care";
    const json& care = field(j, "care", path);
    dog.care_day = parse_local_day(read_string(care, "day", care_path), care_path + ".day");
    const json& records = field(care, "records", care_path);
    for (std::size_t i = 0; i < kCareCount; ++i) {
        const std::string name{kCareNames[i]};
        const std::string record_path = care_path + ".records." + name;
        const json& record = field(records, name.c_str(), care_path + ".records");
        const json& last_done = field(record, "last_done", record_path);
        if (last_done.is_null()) {
            dog.care_records[i].last_done.reset();
        } else if (last_done.is_string()) {
            dog.care_records[i].last_done = parse_time(last_done.get<std::string>(), record_path + ".last_done");
        } else {
            throw InvalidSave(std::format("{}.last_done: 文字列でも null でもない", record_path));
        }
        const json& count = field(record, "count_today", record_path);
        if (!count.is_number_integer() || count.get<long long>() < 0
            || count.get<long long>() > std::numeric_limits<int>::max()) {
            throw InvalidSave(std::format("{}.count_today: 0 以上の整数でない", record_path));
        }
        dog.care_records[i].count_today = count.get<int>();
    }

    const std::string growth_path = path + ".growth";
    const json& growth = field(j, "growth", path);
    dog.growth_points = read_non_negative(growth, "points", growth_path);
    dog.growth_today = read_non_negative(growth, "today", growth_path);
    dog.growth_day = parse_local_day(read_string(growth, "day", growth_path), growth_path + ".day");

    const std::string tricks_path = path + ".tricks";
    const json& tricks = field(j, "tricks", path);
    for (std::size_t i = 0; i < kTrickCount; ++i) {
        dog.trick_proficiency[i] = read_percent(tricks, std::string{kTrickNames[i]}.c_str(), tricks_path);
    }

    // walking と intent は保存しない（読み込み後は既定値）
    return dog;
}

} // namespace

std::string serialize(const SaveData& data)
{
    const json root = {
        {"version", kSaveVersion},
        {"last_saved", format_time(data.last_saved)},
        {"dog", dog_to_json(data.dog)},
    };
    return root.dump(2);
}

LoadResult deserialize(std::string_view text)
{
    const json root = json::parse(text, nullptr, false);
    if (root.is_discarded()) {
        return {std::nullopt, LoadError::Parse, "JSON として解釈できない"};
    }

    try {
        const json& version = field(root, "version", "$");
        if (!version.is_number_integer()) {
            throw InvalidSave("$.version: 整数でない");
        }
        if (version.get<long long>() != kSaveVersion) {
            return {std::nullopt, LoadError::Version,
                    std::format("形式の番号が違う（{}、今は {}）", version.get<long long>(), kSaveVersion)};
        }

        SaveData data;
        data.last_saved = parse_time(read_string(root, "last_saved", "$"), "$.last_saved");
        data.dog = dog_from_json(field(root, "dog", "$"), "$.dog");
        return {std::move(data), LoadError::None, {}};
    } catch (const InvalidSave& e) {
        return {std::nullopt, LoadError::Invalid, e.what()};
    } catch (const json::exception& e) {
        return {std::nullopt, LoadError::Invalid, e.what()};
    }
}

} // namespace dal::core
