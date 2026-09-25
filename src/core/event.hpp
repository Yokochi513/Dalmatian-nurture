#pragma once

#include "core/dog.hpp"

namespace dal::core {

// core を進める関数が返す、一度きりの出来事（ADR 0020）
enum class EventKind {
    Grew,  // 成長した。stage に新しい段階
    // 芸の習得（LearnedTrick）などは各部品の設計で追加する
};

struct Event {
    EventKind kind;
    GrowthStage stage = GrowthStage::Puppy;  // Grew のとき
};

} // namespace dal::core
