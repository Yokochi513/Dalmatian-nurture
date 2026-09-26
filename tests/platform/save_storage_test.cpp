#include "platform/save_storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace std::chrono_literals;
using dal::core::ClockReading;
using dal::core::SaveData;
using dal::core::TimePoint;
using dal::platform::LoadStatus;
using dal::platform::SaveStorage;

namespace {

// 2026-09-26 03:04:05 UTC ＝ 12:04:05 JST
const TimePoint kNow{std::chrono::sys_days{std::chrono::year{2026} / 9 / 26} + 3h + 4min + 5s};
const ClockReading kReading{kNow, std::chrono::hours{9}};

// テストごとに空の一時ディレクトリを作り、終わったら消す
class TempDir {
public:
    explicit TempDir(const std::string& name)
        : path_(std::filesystem::temp_directory_path() / ("dalmatian_test_" + name))
    {
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }
    ~TempDir() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

SaveData data_named(const std::string& name)
{
    SaveData data;
    data.dog.name = name;
    data.last_saved = kNow;
    return data;
}

void write_text(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::binary);
    out << text;
}

int count_broken(const std::filesystem::path& dir)
{
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().filename().string().find(".broken-") != std::string::npos) {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST_CASE("セーブがなければ NoSave")
{
    TempDir dir{"no_save"};
    SaveStorage storage{dir.path()};

    CHECK(storage.load(kReading).status == LoadStatus::NoSave);
}

TEST_CASE("書いて読むと元に戻り、2回目の書き込みで1回目が .bak になる")
{
    TempDir dir{"round_trip"};
    SaveStorage storage{dir.path()};

    REQUIRE(storage.save(data_named("ポチ")));
    REQUIRE(storage.save(data_named("タロウ")));

    const auto loaded = storage.load(kReading);
    REQUIRE(loaded.status == LoadStatus::Loaded);
    CHECK(loaded.data->dog.name == "タロウ");
    CHECK_FALSE(loaded.from_backup);
    CHECK(std::filesystem::exists(storage.backup_path()));
    CHECK_FALSE(std::filesystem::exists(storage.temp_path()));
}

TEST_CASE("save.json がなく .bak だけあれば .bak を読む")
{
    TempDir dir{"backup_only"};
    SaveStorage storage{dir.path()};
    REQUIRE(storage.save(data_named("ポチ")));
    std::filesystem::rename(storage.main_path(), storage.backup_path());

    const auto loaded = storage.load(kReading);
    REQUIRE(loaded.status == LoadStatus::Loaded);
    CHECK(loaded.from_backup);
    CHECK(loaded.data->dog.name == "ポチ");
}

TEST_CASE("save.json が壊れていれば退避して .bak を読み、次の書き込みで .bak を失わない")
{
    TempDir dir{"main_broken"};
    SaveStorage storage{dir.path()};
    REQUIRE(storage.save(data_named("ポチ")));
    REQUIRE(storage.save(data_named("タロウ")));  // .bak はポチ
    write_text(storage.main_path(), "{ broken");

    const auto loaded = storage.load(kReading);
    REQUIRE(loaded.status == LoadStatus::Loaded);
    CHECK(loaded.from_backup);
    CHECK(loaded.data->dog.name == "ポチ");
    CHECK(loaded.messages.size() == 1);
    CHECK(std::filesystem::exists(dir.path() / "save.json.broken-20260926-120405"));

    // 退避したので、次の書き込みで壊れたファイルが .bak に回らない
    REQUIRE(storage.save(data_named("ハナ")));
    CHECK(storage.load(kReading).data->dog.name == "ハナ");
    std::filesystem::remove(storage.main_path());
    CHECK(storage.load(kReading).data->dog.name == "ポチ");
}

TEST_CASE("save.json も .bak も壊れていれば両方を退避し Unreadable")
{
    TempDir dir{"both_broken"};
    SaveStorage storage{dir.path()};
    write_text(storage.main_path(), "{ broken");
    write_text(storage.backup_path(), "");

    const auto loaded = storage.load(kReading);
    CHECK(loaded.status == LoadStatus::Unreadable);
    CHECK_FALSE(loaded.data.has_value());
    CHECK(loaded.messages.size() == 2);
    CHECK(count_broken(dir.path()) == 2);
    CHECK_FALSE(std::filesystem::exists(storage.main_path()));
    CHECK_FALSE(std::filesystem::exists(storage.backup_path()));
}

TEST_CASE("形式の番号が違えば退避する")
{
    TempDir dir{"version"};
    SaveStorage storage{dir.path()};
    REQUIRE(storage.save(data_named("ポチ")));

    std::ifstream in(storage.main_path(), std::ios::binary);
    std::string text{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    in.close();
    text.replace(text.find("\"version\": 1"), 12, "\"version\": 9");
    write_text(storage.main_path(), text);

    CHECK(storage.load(kReading).status == LoadStatus::Unreadable);
    CHECK(count_broken(dir.path()) == 1);
}

TEST_CASE("同じ時刻に退避しても名前が重ならない")
{
    TempDir dir{"same_stamp"};
    SaveStorage storage{dir.path()};

    write_text(storage.main_path(), "{ broken");
    storage.load(kReading);
    write_text(storage.main_path(), "{ broken");
    storage.load(kReading);

    CHECK(std::filesystem::exists(dir.path() / "save.json.broken-20260926-120405"));
    CHECK(std::filesystem::exists(dir.path() / "save.json.broken-20260926-120405-2"));
}
