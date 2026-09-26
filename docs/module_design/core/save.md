# core / セーブ（save）

ソース：`src/core/save.hpp`、`src/core/save.cpp`　概要：[core.md](../core.md)

セーブデータと JSON 文字列の相互変換、読み込み時の検査、形式の番号（ADR 0006・0027・0028・0029）。

## 範囲

- core が担うのは **JSON 文字列との相互変換と検査** だけ（ADR 0009・0027）
- ファイルの書き込み・一時ファイルとの入れ替え・`.bak` と `*.broken` の扱い・保存先（`%APPDATA%`）は
  `platform` が担う。platform の設計書で扱う
- セーブする時期（ADR 0026）は `app` が決める

## 保存するもの（ADR 0029）

```cpp
inline constexpr int kSaveVersion = 1;

struct SaveData {
    DogState dog;
    TimePoint last_saved;  // 最後にセーブした時刻（UTC）＝最終終了時刻（ADR 0026）
};
```

| `DogState` のメンバー | 保存 | 理由 |
|---|---|---|
| `name`・`needs`・`affection`・`stage` | する | |
| `care_records`・`care_day` | する | クールダウンと1日の上限を再開後も守るため |
| `growth_points`・`growth_today`・`growth_day` | する | |
| `trick_proficiency` | する | |
| `walking` | **しない** | `resume()` で散歩を終えるため（ADR 0029） |
| `intent` | **しない** | 閉じていた間は行動意図を進めず、`resume()` で決め直すため（ADR 0022） |

読み込んだ `DogState` の `walking` は `false`、`intent` は既定値（Idle）になる。
シーンや位置は保存しない（ADR 0029）。

## JSON の形式

目視で確認・編集できる形式にする（ADR 0006）。

- 時刻は ISO 8601 の UTC、ミリ秒まで：`"2026-09-26T03:00:00.000Z"`
- 現地の日付は `"2026-09-26"`
- 列挙は小文字の文字列：`"puppy"`、`"feed"`、`"sit"` など。列挙の順番を変えても既存のセーブが壊れない
- 世話の記録と芸の習熟度は、名前をキーにしたオブジェクトで持つ
- インデント2の整形済み JSON で書き出す

```json
{
  "version": 1,
  "last_saved": "2026-09-26T03:00:00.000Z",
  "dog": {
    "name": "ポチ",
    "needs": { "hunger": 42.5, "exercise": 50.0, "boredom": 60.0, "loneliness": 60.0, "sleepiness": 20.0 },
    "affection": 20.0,
    "stage": "puppy",
    "care": {
      "day": "2026-09-26",
      "records": {
        "feed": { "last_done": "2026-09-26T02:30:00.000Z", "count_today": 1 },
        "pet": { "last_done": null, "count_today": 0 },
        "play": { "last_done": null, "count_today": 0 },
        "walk": { "last_done": null, "count_today": 0 },
        "train": { "last_done": null, "count_today": 0 },
        "perform_trick": { "last_done": null, "count_today": 0 }
      }
    },
    "growth": { "points": 10.0, "today": 10.0, "day": "2026-09-26" },
    "tricks": { "sit": 0.0, "paw": 0.0, "down": 0.0, "stay": 0.0, "spin": 0.0 }
  }
}
```

## 読み込み時の検査（ADR 0027）

次のどれかに当てはまれば「読めない」とする。

| エラー（`LoadError`） | 条件 |
|---|---|
| `Parse` | JSON として解釈できない |
| `Version` | `version` が `kSaveVersion` と違う（公開前。ADR 0028） |
| `Invalid` | 必要な項目がない、型が違う、列挙の文字列が知らない値、時刻・日付の書式が違う、値が範囲外 |

範囲の検査：

- 欲求・なつき度・習熟度：0〜100
- 回数（`count_today`）：0 以上
- 成長ポイント（`points`・`today`）：0 以上
- 名前：空でない

知らないキーは無視する（手で追加したメモなどで壊れないように）。
段階と成長ポイントの食い違いは検査しない（バランスの数値を変えると食い違うため）。

「読めない」場合の扱い（`.bak` を読む、`*.broken` に退避して新しく始める）は `platform` と `app` が行う。

## 形式の番号と変換（ADR 0028）

- 今の形式の番号は `kSaveVersion = 1`
- **公開前** は、番号が違えば `Version` を返す（古いセーブを退避して新しく始める）
- **公開後** は、形式を変えるたびに1段分の変換（v1 → v2 …）を本書と `save.cpp` に足し、古い形式の
  見本ファイル（`tests/core/saves/v1.json` など）で単体テストする。公開時にこの運用に切り替える

## 関数

```cpp
enum class LoadError { None, Parse, Version, Invalid };

struct LoadResult {
    std::optional<SaveData> data;  // 読めたときだけ
    LoadError error = LoadError::None;
    std::string message;           // 読めなかった理由（ログ用。どの項目か）
};

std::string serialize(const SaveData& data);
LoadResult deserialize(std::string_view text);
```

- 例外は外に出さない。JSON ライブラリの例外は `deserialize` の中で `Invalid` に変える
- 書き出したものを読み込むと元に戻る（`walking` と `intent` を除く）ことを単体テストで確かめる

## 検討して採らなかった案

- **時刻を UNIX 時間の数値で持つ**: 変換は簡単だが、目視で確認・編集しにくい
- **列挙を数値で持つ**: 短いが、目視で意味が分からず、列挙の順番を変えるとセーブが壊れる
- **`walking` と `intent` も保存する**: 状態をそのまま保存できるが、読み込み後に `resume()` で必ず上書きされるため意味がない
- **足りない項目は既定値で埋める**: 項目の追加に強いが、名前の変更などに気付かず誤った値で読み込む（ADR 0028）
