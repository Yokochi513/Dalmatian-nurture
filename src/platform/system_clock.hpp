#pragma once

#include "core/clock.hpp"

namespace dal::platform {

// OS の時計とタイムゾーンを読む本番用の時計
class SystemClock final : public core::Clock {
public:
    core::ClockReading now() const override;
};

} // namespace dal::platform
