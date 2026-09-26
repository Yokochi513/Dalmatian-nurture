#include "platform/paths.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>

#include <stdexcept>
#include <string>

namespace dal::platform {

std::filesystem::path executable_dir()
{
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            throw std::runtime_error("実行ファイルのパスを取得できませんでした");
        }
        if (length < buffer.size()) {
            buffer.resize(length);
            return std::filesystem::path{buffer}.parent_path();
        }
        buffer.resize(buffer.size() * 2);  // パスが長くて切り詰められた
    }
}

std::filesystem::path assets_dir()
{
    return executable_dir() / "assets";
}

std::filesystem::path shaders_dir()
{
    return executable_dir() / "shaders";
}

std::filesystem::path save_dir()
{
    PWSTR roaming = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &roaming))) {
        CoTaskMemFree(roaming);
        throw std::runtime_error("%APPDATA% を取得できませんでした");
    }
    const std::filesystem::path dir = std::filesystem::path{roaming} / L"DalmatianNurture";
    CoTaskMemFree(roaming);

    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace dal::platform
