# 0040. ソースは UTF-8 としてコンパイルし、実行ファイルのコードページも UTF-8 にする

- Status: Accepted
- Date: 2026-09-26

## Context

ソースのコメント、テスト名、エラーメッセージには日本語を使う。MSVC は既定でシステムのコードページ
（日本語環境では Shift_JIS）でソースを読むため、UTF-8 で書いた日本語が誤って解釈されるおそれがある。

また、CMake の骨組みを作った際、CTest が日本語のテスト名でテストを1件ずつ実行すると、
コマンドライン引数が Shift_JIS に変換されて Catch2 が名前を照合できず、テストが失敗した。
ゲーム本体でも、日本語を含むファイルパス（`%APPDATA%` 配下のセーブなど。[0027](0027-save-atomic-write-and-backup.md)）
や文字列を扱う際に同じ問題が起きうる。

## Decision

- 自前のターゲットは **`/utf-8`** を付けてコンパイルする（`cmake/CompilerOptions.cmake`）
- 実行ファイル（`dalmatian`・`dalmatian_tests`）には、**アクティブなコードページを UTF-8 にする
  マニフェスト**（`cmake/utf8.manifest`）を埋め込む

## Consequences

- ソースの日本語が、ビルドする環境のコードページに関係なく正しく扱われる
- コマンドライン引数や `char` のファイルパス・文字列が UTF-8 で扱われ、日本語のテスト名で CTest から
  テストを実行できる
- マニフェストの UTF-8 指定は Windows 10 バージョン 1903 以降で有効になる。それより古い Windows は
  対象外とする（[0002](0002-target-platform-windows-desktop.md) の範囲で実用上の影響はない）
- 依存ライブラリには `/utf-8` を付けない。依存ライブラリのソースは ASCII のため影響はない

## Alternatives considered

- **テスト名を英語（ASCII）にする**: 問題は避けられるが、日本語で書くという本リポジトリの方針と合わず、
  ゲーム本体のパスや文字列の問題は残る
- **Windows の wide 文字（`wchar_t`）API を使う**: 確実だが、標準C++ や依存ライブラリの `char` の API と
  変換が必要になり、コードが煩雑になる
