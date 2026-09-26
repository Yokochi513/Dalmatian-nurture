#pragma once

#include <filesystem>
#include <memory>

namespace dal::platform {

// 効果音の再生（miniaudio）。初期化に失敗しても例外にせず、音なしで続ける
class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool available() const;

    // ファイルを1回だけ鳴らす。鳴らせたら true
    bool play(const std::filesystem::path& file);

    // 全体の音量（0〜1）
    void set_volume(float volume);

private:
    struct Engine;
    std::unique_ptr<Engine> engine_;  // 初期化に失敗したら nullptr
};

} // namespace dal::platform
