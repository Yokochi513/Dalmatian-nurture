#pragma once

#include "core/clock.hpp"
#include "core/save.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dal::platform {

enum class LoadStatus {
    Loaded,      // save.json か .bak から読めた
    NoSave,      // セーブがない（初回）
    Unreadable,  // 読めなかったため退避した。新しく始める
};

struct StorageLoad {
    LoadStatus status = LoadStatus::NoSave;
    std::optional<core::SaveData> data;  // Loaded のとき
    bool from_backup = false;            // .bak から読んだ
    std::vector<std::string> messages;   // 退避したファイルと理由（ログ・プレイヤーへの表示用）
};

// セーブファイルの書き込み・読み込み・退避（ADR 0027・0028）
class SaveStorage {
public:
    explicit SaveStorage(std::filesystem::path directory);

    // save.json.tmp に書いてから save.json → .bak、.tmp → save.json の順に入れ替える。失敗したら false
    bool save(const core::SaveData& data);

    // save.json → .bak の順に読み、読めないファイルは退避する。now は退避ファイル名の現地時刻に使う
    StorageLoad load(const core::ClockReading& now);

    std::filesystem::path main_path() const { return directory_ / "save.json"; }
    std::filesystem::path backup_path() const { return directory_ / "save.json.bak"; }
    std::filesystem::path temp_path() const { return directory_ / "save.json.tmp"; }

private:
    const std::filesystem::path directory_;
};

} // namespace dal::platform
