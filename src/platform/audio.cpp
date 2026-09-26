#include "platform/audio.hpp"

#include <miniaudio.h>

#include <algorithm>
#include <string>

namespace dal::platform {

struct Audio::Engine {
    ma_engine engine{};
};

Audio::Audio()
{
    auto engine = std::make_unique<Engine>();
    if (ma_engine_init(nullptr, &engine->engine) == MA_SUCCESS) {
        engine_ = std::move(engine);
    }
}

Audio::~Audio()
{
    if (engine_) {
        ma_engine_uninit(&engine_->engine);
    }
}

bool Audio::available() const
{
    return engine_ != nullptr;
}

bool Audio::play(const std::filesystem::path& file)
{
    if (!engine_) {
        return false;
    }
    // miniaudio は char のパスを UTF-8 として扱う
    const std::u8string utf8 = file.u8string();
    const std::string path(utf8.begin(), utf8.end());
    return ma_engine_play_sound(&engine_->engine, path.c_str(), nullptr) == MA_SUCCESS;
}

void Audio::set_volume(float volume)
{
    if (engine_) {
        ma_engine_set_volume(&engine_->engine, std::clamp(volume, 0.0f, 1.0f));
    }
}

} // namespace dal::platform
