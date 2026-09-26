# platform / 音声（audio）

ソース：`src/platform/audio.hpp`、`src/platform/audio.cpp`、`src/platform/miniaudio_impl.cpp`　概要：[platform.md](../platform.md)

効果音（吠える声など。ADR 0037）の再生。miniaudio の高水準 API（`ma_engine`）を使う（ADR 0006）。

## 決めたこと

- `Audio` の作成時に音声エンジンを初期化する。**初期化に失敗しても例外にせず、音なしで続ける**
  （音声デバイスがない環境でも遊べるように）。`available()` で使えるかを返す
- 効果音はファイルのパスを渡して1回だけ鳴らす（鳴らしっぱなし・止めるは扱わない）
- 全体の音量を 0〜1 で設定できる
- 読み込めないファイルは鳴らさず、失敗を返す（例外にしない）
- BGM（ループ再生・止める）は、必要になった時点で設計する

## インターフェース

```cpp
class Audio {
public:
    Audio();
    ~Audio();
    bool available() const;
    bool play(const std::filesystem::path& file);  // 鳴らせたら true
    void set_volume(float volume);                 // 0〜1
};
```

ヘッダでは miniaudio を含めない（実装の詳細を .cpp に隠す）。

## 検討して採らなかった案

- **初期化に失敗したら例外にする**: 問題に気付きやすいが、音声デバイスがないだけでゲームが起動しなくなる
- **音声を独立したターゲットにする**: ADR 0009 のとおり、比重が大きくなるまで platform に含める
