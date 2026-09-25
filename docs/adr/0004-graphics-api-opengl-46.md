# 0004. グラフィックスAPIに OpenGL 4.6 Core を採用する

- Status: Accepted
- Date: 2026-09-25

## Context

[0003](0003-no-game-engine-cpp-with-small-libraries.md) によりレンダラを自作するため、
グラフィックスAPIを直接選ぶ必要がある。対象は Windows のみ（[0002](0002-target-platform-windows-desktop.md)）。

## Decision

**OpenGL 4.6 Core Profile** を採用する。

## Consequences

- LearnOpenGL をはじめ学習資料が圧倒的に多く、三角形が表示されるまでの距離が最短
- Compute Shader、SPIR-V、Direct State Access が利用可能で、モダンな書き方ができる
- 描画コマンドの発行コストはVulkan/DX12に劣るが、本作の規模では問題にならない
- macOS は OpenGL 4.1 で打ち止めのため、将来のmac対応は事実上不可能になる。
  [0002](0002-target-platform-windows-desktop.md) の方針と整合する
- グラフィックス専門職に対する訴求力ではVulkan/DX12に劣る

## Alternatives considered

- **Vulkan**: 初回描画までのコード量がOpenGLの数倍。エンジン／グラフィックス職を
  狙う場合のみ見合う
- **DirectX 12**: Windowsゲーム業界の本命だが、記述量と難易度はVulkan並み
- **DirectX 11**: DX12より容易でWindowsネイティブだが、新規学習対象としてはやや
  レガシー扱いで、資料の新規供給が少ない
