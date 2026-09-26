# platform / 置き場所（paths）

ソース：`src/platform/paths.hpp`、`src/platform/paths.cpp`　概要：[platform.md](../platform.md)

実行ファイル・アセット・シェーダ・セーブの置き場所を求める。

## 決めたこと

| 置き場所 | 求め方 |
|---|---|
| 実行ファイルのディレクトリ | `GetModuleFileNameW` |
| アセット | 実行ファイルのディレクトリの `assets/` |
| シェーダ | 実行ファイルのディレクトリの `shaders/` |
| セーブ | `%APPDATA%\DalmatianNurture\`（`SHGetKnownFolderPath(FOLDERID_RoamingAppData)`）。なければ作る |

- アセットとシェーダは、ビルド時に CMake でリポジトリの `assets/`・`shaders/` を実行ファイルの隣へコピーする。
  開発中も配布時も同じ場所から読める
- パスは `std::filesystem::path` で扱う。Windows API からは wide 文字列で受け取り、そのまま `path` にする
  （日本語を含むユーザー名でも壊れない。ADR 0040）
- セーブの置き場所を求められない場合は例外を投げる（セーブできないまま遊ぶと進行が失われるため）

## インターフェース

```cpp
std::filesystem::path executable_dir();
std::filesystem::path assets_dir();
std::filesystem::path shaders_dir();
std::filesystem::path save_dir();  // なければ作る
```

## 検討して採らなかった案

- **開発中はリポジトリのディレクトリを直接読む（ソースのパスを埋め込む）**: コピーが要らないが、開発中と
  配布時で読み込み先が変わり、配布時にだけ起きる問題に気付きにくい
- **セーブを実行ファイルの隣に置く**: 分かりやすいが、`Program Files` などに置くと書き込めない
