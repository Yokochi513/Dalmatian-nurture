#pragma once

namespace dal::physics {

// Jolt の全体の初期化（アロケータ・ファクトリ・型の登録）と後始末を行う。
// 当たり判定を使う間、1つだけ生かしておく
class JoltRuntime {
public:
    JoltRuntime();
    ~JoltRuntime();

    JoltRuntime(const JoltRuntime&) = delete;
    JoltRuntime& operator=(const JoltRuntime&) = delete;
};

} // namespace dal::physics
