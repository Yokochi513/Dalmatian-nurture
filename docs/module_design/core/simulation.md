# core / 時間の進め方と入口（simulation）

ソース：`src/core/simulation.hpp`、`src/core/simulation.cpp`　概要：[core.md](../core.md)

育成ロジックの入口。犬の状態を持ち、時計を読んで時間を進め、世話の命令を受け付ける。
状態を書き換えてよいのはこのクラスだけ。

## インターフェース

```cpp
class Simulation {
public:
    // last_saved: セーブの最終終了時刻（ADR 0026）
    Simulation(const Clock& clock, DogState state, TimePoint last_saved, Tuning tuning = {});

    std::vector<Event> resume();  // 起動時に1回。散歩中だったら散歩を終える
    std::vector<Event> step();    // 毎フレーム
    CareAvailability availability(Care care) const;               // メニューの表示用
    CareAvailability availability(Care care, Trick trick) const;  // 芸を指定した判定（tricks.md）
    CareResult do_care(Care care);               // 先に step() で時間を進めてから判定・実行する
    CareResult do_trick(Care care, Trick trick); // しつける・芸をさせる（tricks.md）
    std::vector<Event> end_walk();  // 散歩から家に戻ったとき
    void finish_intent(std::uint64_t id);  // 動作の意図の演出が終わったとき（behavior.md）
    double time_of_day() const;            // 一日の中の時刻（現地時刻の時。ADR 0024）
    const DogState& state() const;
};
```

## 時間の進め方

- 前回処理した時刻と、1秒に満たない端数を持つ（ADR 0021）
- `step()` は時計を読み、前回からの経過を1秒刻みで処理し、端数を持ち越す
- 1秒刻みごとの処理：活動（散歩中なら `Walking`、行動意図が Sleep なら `Sleeping`、それ以外は `Awake`）を決め、
  `advance_needs` → `apply_neglect` → `update_intent` を呼ぶ（[needs.md](needs.md)、[behavior.md](behavior.md)）。
  1秒刻みの時刻は、前回の刻みから1秒ずつ進めた時刻とする（行動意図の `since` に使う）
- 経過がマイナス（時計が戻った）なら 0 として扱い、戻った時刻から改めて進める（ADR 0022）
- **起動中でも、前回から `offline_gap` 以上空いた場合は不在として扱う**（`apply_absence`）。
  起動したまま PC がスリープした場合などに、1秒刻みで何時間分も処理しないため。
  デバッグ時計の ×3600（ADR 0025）では1フレームで約60秒進むため、それが不在扱いにならない長さにする。
  このとき行動意図は規則で決め直す

## 起動時（resume）

- 散歩中だったら散歩を終える（ADR 0029）
- セーブの最終終了時刻からの経過を不在の一括計算で進める（ADR 0022）
- 最終終了時刻が未来（時計が戻った）なら何もしない
- 行動意図を Greet にする（「おかえり」。[behavior.md](behavior.md)）

## 世話の入口

- `availability()` は、メニューの表示のために現在の状態で判定する（状態を変えない）
- `do_care()` は、先に `step()` で時間を進めてから判定・実行する。直前に `step()` を呼んでいなくても、
  最新の欲求で判定できる（[care.md](care.md)）
- 世話を受け付けたら、その世話の `growth_points` を `add_growth` で足す（[growth.md](growth.md)）。
  `care` は成長を知らず、組み合わせるのは `Simulation` の役目とする
- 世話を受け付けたら、行動意図を世話に対応する動作の意図に切り替える（`start_care_intent`、[behavior.md](behavior.md)）
- `do_care()` は判定結果と出来事をまとめて返す
- しつける・芸をさせるは `do_trick(care, trick)` で実行する。`do_care(Train)` のように芸を指定しなければ
  `TrickNotSpecified` で断る。受け付けたら `do_care()` と同じく成長ポイントを足し、習得の出来事
  （`LearnedTrick`）も返す

```cpp
struct CareResult {
    CareAvailability availability;
    std::vector<Event> events;  // step() で起きた出来事と、世話による成長など
};
```

- `end_walk()` も先に `step()` で時間を進めてから散歩を終え、行動意図を Greet にし、`step()` で起きた出来事を返す
- `finish_intent(id)` は先に `step()` で時間を進めてから、[behavior.md](behavior.md) の `finish_intent` を呼ぶ

## 出来事の受け渡し

- `resume()`・`step()`・`do_care()`・`end_walk()` は、起きた出来事（`Event`、[growth.md](growth.md)）のリストを返す（ADR 0020）
- 不在中の出来事は「おかえり」の表示に使う（ADR 0011）
- 今のところ出来事は世話による成長（`Grew`）だけで、`resume()` と `step()` は空のリストを返す。
  時間の経過で起きる出来事は、行動意図や芸の設計で加わる

## Tuning

| 項目 | 仮の既定値 | 意味 |
|---|---|---|
| `offline_gap` | 10分 | 前回の `step()` からこれ以上空いたら不在として扱う |

## 検討して採らなかった案

- **`offline_gap` を60秒にする**: スリープからの復帰を早く検出できるが、デバッグ時計の ×3600 で早送りが不在扱いになる
