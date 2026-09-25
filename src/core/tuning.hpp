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

// 行動意図の数値
struct BehaviorTuning {
    double sleep_threshold_day = 80.0;    // 昼に寝始める眠気
    double sleep_threshold_night = 40.0;  // 夜に寝始める眠気（ADR 0032）
    double night_start = 21.0;            // 夜の始まり（現地時刻の時）
    double night_end = 6.0;               // 夜の終わり（現地時刻の時）
    double wake_threshold = 5.0;          // 起きる眠気
    double beg_threshold = 60.0;          // おねだりを始める欲求
    std::chrono::seconds bark_after{std::chrono::minutes{2}};  // おねだりが続いたら吠えるまで
    double wander_threshold = 30.0;       // うろうろし始める退屈
    std::chrono::seconds idle_before_wander{20};  // うろうろする前にじっとしている時間
    std::chrono::seconds sniff_interval{30};      // 散歩中に匂いを嗅ぐ（または掘る）間隔
    double dig_threshold = 60.0;                  // 散歩中に匂いを嗅ぐ代わりに掘る退屈
    std::chrono::seconds max_action_duration{60}; // 終了通知が来なくても動作の意図を終わらせる時間
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

    // 行動意図
    BehaviorTuning behavior;
};

} // namespace dal::core
