#include "platform/system_clock.hpp"

#include <chrono>
#include <exception>

namespace dal::platform {

core::ClockReading SystemClock::now() const
{
    using namespace std::chrono;

    const auto utc = floor<milliseconds>(system_clock::now());
    minutes offset{0};
    try {
        offset = duration_cast<minutes>(current_zone()->get_info(utc).offset);
    } catch (const std::exception&) {
        // タイムゾーン情報を読めない環境では UTC として扱う
    }
    return {utc, offset};
}

} // namespace dal::platform
