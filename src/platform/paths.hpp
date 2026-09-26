#pragma once

#include <filesystem>

namespace dal::platform {

// 実行ファイルのディレクトリ
std::filesystem::path executable_dir();

// アセット・シェーダ（ビルド時に実行ファイルの隣へコピーする）
std::filesystem::path assets_dir();
std::filesystem::path shaders_dir();

// セーブの置き場所（%APPDATA%\DalmatianNurture）。なければ作る。求められなければ例外
std::filesystem::path save_dir();

} // namespace dal::platform
