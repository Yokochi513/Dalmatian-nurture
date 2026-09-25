# core モジュール設計

`dalmatian_core`（`src/core/`、名前空間 `dal::core`）の設計の概要。
モジュールをまたぐ決定は [docs/adr/](../adr/) にあり、ここではそれを前提に core の内部を決める。
各部品の設計は [core/](core/) に、ソースファイルと対応する単位で置く（ADR 0043）。

## 責務

- 犬の状態を持ち、時間の経過と世話の命令によって更新する
- 犬の行動意図（何をしたいか・どこへ行きたいか）を決める
- セーブデータ（JSON）との相互変換を行う
- 描画・アニメーション・空間・OS を知らない。依存は標準C++ と nlohmann/json のみ（ADR 0009）
- OS に依存する実装（時計など）は `platform` が core のインターフェースを実装して提供する（ADR 0042）

## 設計の方針

- **状態は単純な構造体、処理は関数** とする。`DogState` は公開メンバーだけを持ち、`needs` などは
  状態を受け取って更新する関数として書く。関数ごとに単体テストできる
- **状態を書き換えてよいのは `Simulation` だけ**。`app` には `const DogState&` を渡す（ADR 0020）
- **バランスの数値は `Tuning` 構造体**（`tuning.hpp`）にまとめ、既定値をメンバーに書く。`Simulation` の
  作成時に渡す。テストは計算しやすい数値を自分で指定するため、バランスを調整してもテストは壊れない。
  各項目の意味と仮の既定値は、その数値を使う部品の設計書に書く
- **乱数を使わない**（ADR 0035）。使う必要が出た場合は外から注入する

## 部品と設計書

| ソース | 内容 | 設計書 | 状態 |
|---|---|---|---|
| `clock.hpp` | 時計のインターフェース、現地時刻の計算 | [clock.md](core/clock.md) | 実装済み |
| `dog.hpp/.cpp` | 犬の状態（`DogState`）と関連する列挙、初回の子犬の状態 | [dog.md](core/dog.md) | 実装済み（部品の追加に合わせて拡張） |
| `needs.hpp/.cpp` | 欲求の時間変化、なつき度の低下、不在中の一括計算、機嫌 | [needs.md](core/needs.md) | 実装済み |
| `care.hpp/.cpp` | 世話の可否と効果、クールダウン、1日の上限、散歩 | [care.md](core/care.md) | 実装済み |
| `simulation.hpp/.cpp` | 1秒刻みで進める入口、不在からの復帰、世話の入口 | [simulation.md](core/simulation.md) | 実装済み（部品の追加に合わせて拡張） |
| `tuning.hpp` | バランスの数値（`Tuning`） | 各部品の設計書 | — |
| `growth.hpp/.cpp`、`event.hpp` | 成長ポイントと成長段階、出来事 | [growth.md](core/growth.md) | 実装済み |
| `tricks.hpp/.cpp` | 芸の習熟度と習得、しつける・芸をさせる | [tricks.md](core/tricks.md) | 実装済み |
| `behavior.hpp/.cpp` | 行動意図の決定、行動の終了通知、寝ている間 | behavior.md | 未設計 |
| `save.hpp/.cpp` | JSON との相互変換、形式の番号と変換 | save.md | 未設計 |

## テスト方針

- テストは `tests/core/` に部品ごとのファイルで置く（`needs_test.cpp` など）
- `FakeClock`（`tests/core/fake_clock.hpp`）で時刻と時差を操作し、`Simulation` の結果を確かめる
- 部品の関数は `Simulation` を通さずに直接テストする
- テストでは `Tuning` の数値を明示する（既定値に依存しない）
- 小数の比較は誤差を許容する（ADR 0031）
