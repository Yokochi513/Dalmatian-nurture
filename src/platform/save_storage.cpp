#include "platform/save_storage.hpp"

#include <chrono>
#include <format>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace dal::platform {

namespace {

struct ReadAttempt {
    bool exists = false;
    std::optional<core::SaveData> data;
    std::string reason;  // 読めなかった理由
};

std::string to_utf8(const std::filesystem::path& path)
{
    const std::u8string utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

ReadAttempt read_file(const std::filesystem::path& path)
{
    ReadAttempt attempt;
    std::error_code ec;
    attempt.exists = std::filesystem::exists(path, ec);
    if (!attempt.exists) {
        return attempt;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        attempt.reason = "ファイルを開けない";
        return attempt;
    }
    const std::string text{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};

    core::LoadResult result = core::deserialize(text);
    if (result.data) {
        attempt.data = std::move(result.data);
    } else {
        attempt.reason = std::move(result.message);
    }
    return attempt;
}

// 読めなかったファイルを <名前>.broken-YYYYMMDD-HHMMSS に退避する。同名があれば -2, -3 … を付ける
void quarantine(const std::filesystem::path& path, const std::string& reason, const core::ClockReading& now,
                std::vector<std::string>& messages)
{
    const auto local = std::chrono::floor<std::chrono::seconds>(core::local_now(now));
    const auto day = std::chrono::floor<std::chrono::days>(local);
    const std::chrono::year_month_day ymd{std::chrono::sys_days{day.time_since_epoch()}};
    const std::chrono::hh_mm_ss hms{local - day};
    const std::string stamp = std::format("{:04}{:02}{:02}-{:02}{:02}{:02}", static_cast<int>(ymd.year()),
                                          static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()),
                                          hms.hours().count(), hms.minutes().count(), hms.seconds().count());

    const std::string base = to_utf8(path.filename()) + ".broken-" + stamp;
    std::filesystem::path target = path.parent_path() / std::filesystem::path{std::u8string{base.begin(), base.end()}};
    for (int n = 2; std::filesystem::exists(target); ++n) {
        const std::string numbered = std::format("{}-{}", base, n);
        target = path.parent_path() / std::filesystem::path{std::u8string{numbered.begin(), numbered.end()}};
    }

    std::error_code ec;
    std::filesystem::rename(path, target, ec);
    if (ec) {
        messages.push_back(std::format("{} を退避できなかった（{}）：{}", to_utf8(path.filename()), ec.message(), reason));
    } else {
        messages.push_back(std::format("{} を {} に退避した：{}", to_utf8(path.filename()),
                                       to_utf8(target.filename()), reason));
    }
}

} // namespace

SaveStorage::SaveStorage(std::filesystem::path directory)
    : directory_(std::move(directory))
{
}

bool SaveStorage::save(const core::SaveData& data)
{
    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    if (ec) {
        return false;
    }

    {
        std::ofstream out(temp_path(), std::ios::binary | std::ios::trunc);
        if (!out) {
            return false;
        }
        out << core::serialize(data);
        out.close();
        if (!out) {
            return false;
        }
    }

    if (std::filesystem::exists(main_path(), ec)) {
        std::filesystem::rename(main_path(), backup_path(), ec);
        if (ec) {
            return false;
        }
    }
    std::filesystem::rename(temp_path(), main_path(), ec);
    return !ec;
}

StorageLoad SaveStorage::load(const core::ClockReading& now)
{
    StorageLoad result;

    ReadAttempt main = read_file(main_path());
    if (main.data) {
        result.status = LoadStatus::Loaded;
        result.data = std::move(main.data);
        return result;
    }
    // 読めない save.json は、.bak を読む前に退避する。
    // 残すと次の書き込みで .bak に回り、読めていた .bak が失われるため
    if (main.exists) {
        quarantine(main_path(), main.reason, now, result.messages);
    }

    ReadAttempt backup = read_file(backup_path());
    if (backup.data) {
        result.status = LoadStatus::Loaded;
        result.data = std::move(backup.data);
        result.from_backup = true;
        return result;
    }
    if (backup.exists) {
        quarantine(backup_path(), backup.reason, now, result.messages);
    }

    result.status = (main.exists || backup.exists) ? LoadStatus::Unreadable : LoadStatus::NoSave;
    return result;
}

} // namespace dal::platform
