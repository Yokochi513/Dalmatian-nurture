# AGENT.md

このリポジトリで作業するエージェントが守るルール。

## ルール

1. 各決定事項はADRとして`docs/adr/`内に1つずつ必ず記録する
2. ただし、1つのモジュールの内部の決定は`docs/module_design/<モジュール名>.md`に記録する（[ADR 0041](docs/adr/0041-module-design-docs.md)）。ほかのモジュールに影響する場合や、どちらか迷う場合はADRにする
