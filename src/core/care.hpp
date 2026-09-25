#pragma once

#include "core/clock.hpp"
#include "core/dog.hpp"
#include "core/tuning.hpp"

#include <chrono>

namespace dal::core {

// 世話を実行できない理由（ADR 0017）。表示する文言への変換は app が行う
enum class CareBlock {
    None,            // 実行できる
    AlreadyWalking,  // すでに散歩中（散歩の開始のみ）
    DailyLimit,      // 今日の上限に達した
    Cooldown,        // クールダウン中
    NotNeeded,       // 対応する欲求が満たされている
    // 芸を指定した判定（tricks）
    TrickLocked,        // 今の成長段階ではまだ練習できない
    TrickLearned,       // すでに習得している（しつける）
    TrickNotLearned,    // まだ習得していない（芸をさせる）
    TrickNotSpecified,  // しつける・芸をさせるを芸を指定せずに実行しようとした
};

struct CareAvailability {
    CareBlock block = CareBlock::None;
    std::chrono::seconds remaining{0};  // 実行できるようになるまでの時間（DailyLimit・Cooldown のみ）

    bool available() const { return block == CareBlock::None; }
};

const CareTuning& tuning_of(const Tuning& tuning, Care care);

// 世話を実行できるか。理由が複数ある場合は、AlreadyWalking・DailyLimit・Cooldown・NotNeeded の順に優先する
CareAvailability check_care(const DogState& state, Care care, const ClockReading& now, const Tuning& tuning);

// 世話を実行する。実行できなければ状態を変えずに理由を返す。
// 受け付けた時点で効果を確定する（ADR 0016）。Train・PerformTrick は TrickNotSpecified で断る（tricks を使う）
CareAvailability apply_care(DogState& state, Care care, const ClockReading& now, const Tuning& tuning);

// 判定をせずに世話の効果を適用し、記録する。判定を済ませた呼び出し側（apply_care・tricks）が使う
void commit_care(DogState& state, Care care, const ClockReading& now, const Tuning& tuning);

bool is_trick_care(Care care);

} // namespace dal::core
