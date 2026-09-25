# 0001. 決定事項はADRとして記録する

- Status: Accepted
- Date: 2026-09-25

## Context

本プロジェクトは個人開発であり、技術選定の経緯が記録されないと、数ヶ月後の自分が
「なぜこの構成なのか」を再現できない。ポートフォリオとして公開する前提もあるため、
判断の経緯そのものが成果物の一部になる。

## Decision

各決定事項を1件ずつ `docs/adr/` 配下のADR（Architecture Decision Record）として記録する。

- ファイル名は `NNNN-<英小文字ハイフン区切りの要約>.md`（連番は0001から、欠番を作らない）
- 書式は [template.md](template.md) に従う
- Status は `Proposed` / `Accepted` / `Superseded by NNNN` のいずれか
- 決定を覆す場合は既存ADRを書き換えず、新しいADRを追加して旧ADRの Status を
  `Superseded by NNNN` に変更する

## Consequences

- 決定の経緯が追跡可能になり、再検討時に同じ議論を繰り返さずに済む
- 決定ごとにファイルを追加する手間が発生する
- 「何がまだ決まっていないか」もADRの不在として可視化される

## Alternatives considered

- **README や単一の設計ドキュメントに集約**: 記述が上書きされ、過去の判断理由が失われる
- **コミットメッセージのみで残す**: 決定単位で検索できず、経緯が履歴に埋没する
