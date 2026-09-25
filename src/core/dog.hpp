#pragma once

#include "core/clock.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
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

// 世話の種類。Train・PerformTrick は芸を指定して実行する（tricks）
enum class Care { Feed, Pet, Play, Walk, Train, PerformTrick };
inline constexpr std::size_t kCareCount = 6;

// 芸（ADR 0034）
enum class Trick { Sit, Paw, Down, Stay, Spin };
inline constexpr std::size_t kTrickCount = 5;

// 行動意図（ADR 0037）。決め方は behavior
enum class IntentKind {
    Idle, Wander, Sleep, Stretch, Beg, Bark, Greet,  // 自律・家
    Eat, Petted, Play, Train, PerformTrick,          // 世話への反応
    Follow, Sniff, Dig,                              // 散歩中
};

// 行き先の種類（ADR 0019）。座標は app が決める
enum class Destination { Here, Any, Bowl, Bed, Owner };

enum class NeedKind { Hunger, Exercise, Boredom, Loneliness, Sleepiness };

struct Intent {
    IntentKind kind = IntentKind::Idle;
    Destination destination = Destination::Here;
    NeedKind need = NeedKind::Hunger;  // Beg のとき：何が欲しいか
    Trick trick = Trick::Sit;          // Train・PerformTrick のとき
    TimePoint since{};                 // この行動意図になった時刻（UTC）
    std::uint64_t id = 0;              // 行動意図が変わるたびに増える通し番号
};

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

    std::array<double, kTrickCount> trick_proficiency{};  // Trick の順。0〜100、100 で習得

    Intent intent;  // 今の行動意図
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
