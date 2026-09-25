#pragma once

#include <chrono>

namespace dal::core {

// 世話ごとの数値
struct CareTuning {
    std::chrono::seconds cooldown{0};  // 次に同じ世話ができるまでの時間
    int daily_limit = 0;               // 1日に受け付ける回数
    double min_need = 0.0;             // 対応する欲求がこれ未満なら「満たされている」として断る
    double relief = 0.0;               // 対応する欲求を減らす量（散歩は時間で減るため 0）
    double affection_gain = 0.0;       // 受け付けたときのなつき度の上がり幅
    double growth_points = 0.0;        // 受け付けたときにたまる成長ポイント
};

// 成長の数値（ADR 0036）
struct GrowthTuning {
    double young_at = 30.0;    // 若犬になる累計の成長ポイント（初回の10分以内）
    double adult_at = 450.0;   // 成犬になる累計の成長ポイント（数日）
    double daily_cap = 150.0;  // 1日にたまる成長ポイントの上限
};

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

    // 世話
    CareTuning feed{std::chrono::hours{3}, 3, 30.0, 70.0, 1.0, 10.0};
    CareTuning pet{std::chrono::minutes{10}, 10, 10.0, 40.0, 2.0, 10.0};
    CareTuning play{std::chrono::minutes{30}, 6, 20.0, 50.0, 2.0, 10.0};
    CareTuning walk{std::chrono::hours{2}, 3, 30.0, 0.0, 3.0, 10.0};
    CareTuning train{std::chrono::minutes{15}, 8, 0.0, 10.0, 1.0, 10.0};
    CareTuning perform_trick{std::chrono::minutes{5}, 10, 0.0, 10.0, 3.0, 5.0};

    // 芸
    double train_base = 25.0;  // 練習1回の習熟度の上がり幅の基本値

    // 成長
    GrowthTuning growth;
};

} // namespace dal::core
