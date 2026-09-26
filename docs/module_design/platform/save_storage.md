# platform / セーブファイル（save_storage）

ソース：`src/platform/save_storage.hpp`、`src/platform/save_storage.cpp`　概要：[platform.md](../platform.md)

セーブファイルの書き込み・読み込み・退避（ADR 0027・0028）。JSON との変換と検査は core の
`serialize`・`deserialize`（[core/save.md](../core/save.md)）を使う（ADR 0042 で platform は core に依存する）。

## ファイル

保存先のディレクトリ（本番は `paths::save_dir()`、テストでは一時ディレクトリ）に次を置く。

| ファイル | 内容 |
|---|---|
| `save.json` | 本番のセーブ |
| `save.json.tmp` | 書き込み途中の一時ファイル |
| `save.json.bak` | 直前の1世代 |
| `save.json.broken-YYYYMMDD-HHMMSS` | 読めなかったため退避した `save.json`（現地時刻。消さずに残す） |
| `save.json.bak.broken-YYYYMMDD-HHMMSS` | 同じく退避した `.bak` |

同じ名前の退避ファイルがすでにあれば、末尾に `-2`、`-3` … を付ける。

## 書き込み（ADR 0027）

```
1. save.json.tmp に書き終えて閉じる
2. save.json があれば save.json.bak に名前を変える（既存の .bak を置き換える）
3. save.json.tmp を save.json に名前を変える
```

- 名前の変更は `std::filesystem::rename`（Windows では既存のファイルを置き換える）
- 2 と 3 の間に落ちると `save.json` がない状態になるが、読み込みで `.bak` を使うため失われない
- 書き込みに失敗したら `false` を返す（例外にしない）。`app` はログに残して遊び続ける
- OS のキャッシュからディスクへの書き出し（`FlushFileBuffers`）までは行わない。停電などでの損失は許容する

## 読み込み（ADR 0027・0028）

```
save.json があり、読めた             → それを使う
save.json がない                     → .bak があり読めればそれを使う。どちらもなければ「セーブなし」
save.json が読めない                 → save.json を退避し、.bak が読めればそれを使う
save.json も .bak も読めない          → 両方を退避し、「読めなかった」として新しく始める
```

- **`save.json` が読めず `.bak` が読めた場合も、その時点で `save.json` を退避する**。退避しないと、次の書き込みで
  壊れた `save.json` が `.bak` に回り、読めていた `.bak` が失われるため
- 「読めない」は、ファイルを開けない場合と、core の `deserialize` が `Parse`・`Version`・`Invalid` を返した場合。
  公開前は形式の番号が違う場合（`Version`）も退避して新しく始める（ADR 0028）
- 退避したときは理由（`LoadResult::message`）を返し、`app` がプレイヤーに伝える（ADR 0027）

## インターフェース

```cpp
enum class LoadStatus {
    Loaded,       // save.json か .bak から読めた
    NoSave,       // セーブがない（初回）
    Unreadable,   // 読めなかったため退避した。新しく始める
};

struct StorageLoad {
    LoadStatus status = LoadStatus::NoSave;
    std::optional<core::SaveData> data;  // Loaded のとき
    bool from_backup = false;            // .bak から読んだ
    std::vector<std::string> messages;   // 退避したファイルと理由（ログ・プレイヤーへの表示用）
};

class SaveStorage {
public:
    explicit SaveStorage(std::filesystem::path directory);
    bool save(const core::SaveData& data);
    StorageLoad load(core::ClockReading now);  // now は退避ファイル名の現地時刻に使う
};
```

## テスト

`tests/platform/save_storage_test.cpp` で、一時ディレクトリに対して次を確かめる（ADR 0044）。

- 書いて読むと元に戻る。2回目の書き込みで1回目が `.bak` になる
- `save.json` がなく `.bak` だけあれば `.bak` を読む
- `save.json` が壊れていれば退避し、`.bak` を読む。次の書き込みで `.bak` が壊れたファイルで上書きされない
- 両方壊れていれば両方を退避し、`Unreadable` を返す
- 形式の番号が違えば退避する

## 検討して採らなかった案

- **`ReplaceFileW` で入れ替える**: `save.json` がない瞬間をなくせるが、`save.json` がない初回は別の手順が要り、
  Windows API に依存する。`.bak` を読む手順で失われないため不要
- **読み込みの手順を `app` に置き、platform はファイル操作だけを提供する**: 部品は小さくなるが、手順を単体テストする
  場所がなくなり、壊れたファイルの扱いを誤りやすい
