#pragma once

#include <chrono>

namespace dal::core {

// バランスの数値。既定値は仮の値で、バランス調整の中で決める。
// テストは既定値に依存せず、必要な数値を明示する
struct Tuning {
    // 欲求の変化率（1時間あたり）
    double hunger_per_hour = 12.5;         // 8時間で 0 → 100
    double exercise_per_hour = 8.0;
    double boredom_per_hour = 16.0;
    double loneliness_per_hour = 10.0;
    double sleepiness_per_hour = 6.0;
    double sleep_recovery_per_hour = 12.5; // Sleep 中・不在中の眠気の減り
    double walk_exercise_per_hour = 180.0; // 散歩中の運動の減り（20分で 60）

    // 放置
    double neglect_threshold = 80.0;          // なつき度が下がり始める欲求の値
    double affection_loss_per_hour = 1.0;
    double offline_need_cap = 80.0;           // 不在中に欲求が届く上限
    double offline_affection_loss_max = 10.0; // 1回の不在でのなつき度の低下の上限

    // 前回の step() からこれ以上空いたら不在として扱う。
    // デバッグ時計の ×3600（1フレームで約60秒）が不在扱いにならない長さにする
    std::chrono::seconds offline_gap{std::chrono::minutes{10}};
};

} // namespace dal::core
