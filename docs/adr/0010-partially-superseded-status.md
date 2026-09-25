# 0010. ADRの一部だけを置き換える場合の Status を追加する

- Status: Accepted
- Date: 2026-09-25

## Context

[0001](0001-record-decisions-in-adr.md) で、決定を覆す場合は新しいADRを追加し、
旧ADRの Status を `Superseded by NNNN` に変更すると定めた。

しかし [0005](0005-build-system-cmake-msvc-cpp20.md) のように、1つのADRに複数の決定が
含まれている場合がある。[0009](0009-module-structure.md) は 0005 のターゲット分割部分のみを
置き換えるが、`Superseded by` を付けると CMake・MSVC などの決定まで無効になったように読める。

## Decision

Status に `Partially superseded by NNNN（置き換えられた範囲）` を追加する。

- 旧ADRの一部だけを新しいADRが置き換える場合に使う
- 括弧内に置き換えられた範囲を書く
- 旧ADRの本文は書き換えない（0001の原則を維持する）
- 複数のADRから部分的に置き換えられた場合は、行を分けて列挙する

[template.md](template.md) の Status の選択肢にも追加する。

## Consequences

- 1つのADRに複数の決定が含まれていても、有効な部分と置き換えられた部分を区別できる
- 読み手は旧ADRと新ADRの両方を読まないと、現在有効な内容が分からない
- 本来は「1つのADRにつき1つの決定」を守れば不要な仕組みであり、新しいADRでは
  引き続き決定を1つに絞る

## Alternatives considered

- **旧ADRを `Superseded by` にし、新ADRに有効な決定をすべて引き継ぐ**: Status の種類は
  増えないが、置き換えていない決定まで新ADRへ複製することになり、変更の範囲が分かりにくい
- **旧ADRを決定ごとに分割し直す**: 構造としては最も正しいが、0001の「本文を書き換えない」
  原則に反し、既存のリンクも壊れる
