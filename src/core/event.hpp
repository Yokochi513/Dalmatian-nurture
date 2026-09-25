#pragma once

#include "core/dog.hpp"

namespace dal::core {

// core を進める関数が返す、一度きりの出来事（ADR 0020）
enum class EventKind {
    Grew,          // 成長した。stage に新しい段階
    LearnedTrick,  // 芸を習得した。trick に芸
};

struct Event {
    EventKind kind;
    GrowthStage stage = GrowthStage::Puppy;  // Grew のとき
    Trick trick = Trick::Sit;                // LearnedTrick のとき
};

} // namespace dal::core
