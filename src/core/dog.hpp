#pragma once

#include "core/clock.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

namespace dal::core {

// 欲求。0〜100 で、大きいほど強く欲しがっている（ADR 0031）
struct Needs {
    double hunger = 0.0;      // 空腹   ← ごはん
    double exercise = 0.0;    // 運動   ← 散歩
    double boredom = 0.0;     // 退屈   ← 遊ぶ
    double loneliness = 0.0;  // 寂しさ ← なでる
    double sleepiness = 0.0;  // 眠気   ← 犬が自分で寝る
};

enum class GrowthStage { Puppy, Young, Adult };

// 世話の種類。しつける・芸をさせるは芸（tricks）の設計で追加する
enum class Care { Feed, Pet, Play, Walk };
inline constexpr std::size_t kCareCount = 4;

// 世話ごとの記録。クールダウンと1日の上限の判定に使う
struct CareRecord {
    std::optional<TimePoint> last_done;  // 最後に受け付けた時刻（UTC）
    int count_today = 0;                 // care_day の日に受け付けた回数
};

// 犬の状態。書き換えてよいのは Simulation だけ
struct DogState {
    std::string name;
    Needs needs;
    double affection = 0.0;  // なつき度（0〜100）
    GrowthStage stage = GrowthStage::Puppy;

    std::array<CareRecord, kCareCount> care_records{};
    std::chrono::local_days care_day{};  // count_today を数えている現地の日付
    bool walking = false;                // 散歩中か（ADR 0018）

    double growth_points = 0.0;            // 累計の成長ポイント
    double growth_today = 0.0;             // growth_day の日にたまった量
    std::chrono::local_days growth_day{};  // growth_today を数えている現地の日付
};

// 初回に迎えた子犬の状態。最初からいくつかの世話ができるよう、欲求をある程度高くしておく
DogState new_dog(std::string name);

inline CareRecord& record_of(DogState& state, Care care)
{
    return state.care_records[static_cast<std::size_t>(care)];
}

inline const CareRecord& record_of(const DogState& state, Care care)
{
    return state.care_records[static_cast<std::size_t>(care)];
}

} // namespace dal::core
