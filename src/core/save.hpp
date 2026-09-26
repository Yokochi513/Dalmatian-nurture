#pragma once

#include "core/clock.hpp"
#include "core/dog.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace dal::core {

// セーブの形式の番号（ADR 0028）
inline constexpr int kSaveVersion = 1;

// セーブデータ（ADR 0029）。DogState の walking と intent は保存しない
struct SaveData {
    DogState dog;
    TimePoint last_saved;  // 最後にセーブした時刻（UTC）＝最終終了時刻（ADR 0026）
};

enum class LoadError {
    None,
    Parse,    // JSON として解釈できない
    Version,  // 形式の番号が違う
    Invalid,  // 項目がない・型が違う・値が範囲外など
};

struct LoadResult {
    std::optional<SaveData> data;  // 読めたときだけ
    LoadError error = LoadError::None;
    std::string message;           // 読めなかった理由（ログ用）
};

// 整形済みの JSON 文字列にする
std::string serialize(const SaveData& data);

// JSON 文字列から読み込み、検査する（ADR 0027）。例外は外に出さない
LoadResult deserialize(std::string_view text);

} // namespace dal::core
