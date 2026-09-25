#pragma once

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

// 犬の状態。書き換えてよいのは Simulation だけ
struct DogState {
    std::string name;
    Needs needs;
    double affection = 0.0;  // なつき度（0〜100）
    GrowthStage stage = GrowthStage::Puppy;
};

} // namespace dal::core
