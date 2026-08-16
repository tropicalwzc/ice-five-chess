#!/usr/bin/env python3
"""Freeze baselines and explain the failed v2 assignment-line opening book."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def score(game: dict[str, Any]) -> float:
    winner = game["winner"]
    color = game["newColor"]
    return 1.0 if winner == color else 0.5 if winner == 0 else 0.0


def first_divergence(on: dict[str, Any], off: dict[str, Any]) -> dict[str, Any] | None:
    on_moves = on["moves"]
    off_moves = off["moves"]
    limit = min(len(on_moves), len(off_moves))
    ply = next((i for i in range(limit) if on_moves[i] != off_moves[i]), None)
    if ply is None:
        if len(on_moves) == len(off_moves):
            return None
        ply = limit
    on_opening = len(on_moves) - len(on["steps"])
    off_opening = len(off_moves) - len(off["steps"])
    on_step = on["steps"][ply - on_opening] if on_opening <= ply < len(on_moves) else None
    off_step = off["steps"][ply - off_opening] if off_opening <= ply < len(off_moves) else None
    return {
        "ply": ply,
        "side": on_moves[ply][2] if ply < len(on_moves) else None,
        "bookOnMove": on_moves[ply][:2] if ply < len(on_moves) else None,
        "bookOffMove": off_moves[ply][:2] if ply < len(off_moves) else None,
        "changedNewEngineDecision": bool(
            on_step and off_step and on_step.get("engine") == "new"
            and off_step.get("engine") == "new"
        ),
        "bookId": on_step.get("bookId", -1) if on_step else -1,
        "bookPly": on_step.get("bookPly", -1) if on_step else -1,
        "proofStatus": on_step.get("proofStatus", 0) if on_step else 0,
        "proofCertificateVerified": bool(
            on_step.get("proofCertificateVerified", False) if on_step else False
        ),
        "overrideReason": on_step.get("overrideReason", 0) if on_step else 0,
        "bookOnResult": score(on),
        "bookOffResult": score(off),
        "bookOnWinner": on["winner"],
        "bookOffWinner": off["winner"],
    }


def profile(headers: list[dict[str, Any]]) -> dict[str, Any]:
    profiles = [header["newProfile"] for header in headers]
    if not profiles or any(item != profiles[0] for item in profiles[1:]):
        raise ValueError("raw report contains inconsistent profile headers")
    return profiles[0]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    source = repo / "reports/five_chess/proof_search_formal"
    on_path = source / "proof_search_book_on_raw.json"
    off_path = source / "proof_search_book_off_raw.json"
    on_raw, off_raw = load(on_path), load(off_path)
    on_games = {(g["openingId"], g["newColor"]): g for g in on_raw["games"]}
    off_games = {(g["openingId"], g["newColor"]): g for g in off_raw["games"]}
    if on_games.keys() != off_games.keys() or len(on_games) != 200:
        raise ValueError("book-on/off schedules are not the same 200-game domain")

    pairs: list[dict[str, Any]] = []
    counts = {
        "pairedGames": len(on_games), "bookHits": 0, "gamesWithBookHit": 0,
        "divergedGames": 0, "betterToWorse": 0, "worseToBetter": 0,
        "unchangedWin": 0, "unchangedLoss": 0, "unchangedDraw": 0,
        "divergenceWithNoForcedWinInScope": 0,
        "divergenceWithVerifiedCertificate": 0,
    }
    for key in sorted(on_games):
        on, off = on_games[key], off_games[key]
        hits = [s for s in on["steps"] if s.get("engine") == "new" and s.get("bookId", -1) >= 0]
        counts["bookHits"] += len(hits)
        counts["gamesWithBookHit"] += bool(hits)
        divergence = first_divergence(on, off)
        item = {
            "openingId": key[0], "newColor": key[1],
            "bookHitCount": len(hits), "divergence": divergence,
        }
        if divergence:
            counts["divergedGames"] += 1
            before, after = divergence["bookOffResult"], divergence["bookOnResult"]
            if before > after:
                counts["betterToWorse"] += 1
            elif before < after:
                counts["worseToBetter"] += 1
            elif after == 1.0:
                counts["unchangedWin"] += 1
            elif after == 0.0:
                counts["unchangedLoss"] += 1
            else:
                counts["unchangedDraw"] += 1
            counts["divergenceWithNoForcedWinInScope"] += divergence["proofStatus"] == 2
            counts["divergenceWithVerifiedCertificate"] += divergence["proofCertificateVerified"]
        pairs.append(item)

    args.output.mkdir(parents=True, exist_ok=True)
    baseline_files = [
        repo / "ice five chess/FiveChessAI.c",
        repo / "ice five chess/FiveChessAI.h",
        repo / "ice five chess/doublethree.m",
        repo / "ice five chess/FiveChessOpeningBook.inc",
        repo / "tools/legacy_three_star_manifest.json",
        on_path, off_path,
        source / "proof_search_book_on.md",
        source / "proof_search_book_off.md",
        source / "proof_search_opening_book_comparison.md",
    ]
    manifest = {
        "schemaVersion": 1,
        "frozenAt": "2026-08-13",
        "fourStarProfile": profile(off_raw["headers"]),
        "failedBookProfile": profile(on_raw["headers"]),
        "legacyThreeStar": load(repo / "tools/legacy_three_star_manifest.json"),
        "files": [{"path": str(p.relative_to(repo)), "sha256": sha256(p)} for p in baseline_files],
    }
    (args.output / "baseline_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    result = {
        "schemaVersion": 1,
        "scope": "paired corrected proof_search_formal book-on versus book-off",
        "counts": counts,
        "pointDelta": sum(score(on_games[k]) - score(off_games[k]) for k in on_games),
        "pairs": pairs,
    }
    (args.output / "failed_book_root_cause.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    md = f"""# 旧开局库降级根因审计

生成日期：2026-08-13。分析对象是已纠正颜色统计后的同一 200 局正式赛程，逐一按 `openingId + newColor` 配对。相关性用于定位机制，不把单局结果误写成落子因果。

## 可复现结论

- 200 局全部成功配对；开局库共命中 {counts['bookHits']} 次，涉及 {counts['gamesWithBookHit']} 局。
- book-on 与 book-off 实际分叉 {counts['divergedGames']} 局；其中原本较好结果变差 {counts['betterToWorse']} 局，改善 {counts['worseToBetter']} 局，同为胜/负/和分别 {counts['unchangedWin']}/{counts['unchangedLoss']}/{counts['unchangedDraw']} 局。
- book-on 相对 book-off 净变化 {result['pointDelta']:+.1f}/200 分，即 {result['pointDelta']/200:+.1%}。
- 首次分叉处有 {counts['divergenceWithNoForcedWinInScope']} 次仅得到 `NO_FORCED_WIN_IN_SCOPE`，有 {counts['divergenceWithVerifiedCertificate']} 次具有已验证的获胜证明证书。

## 四层根因

1. **内容类别错误**：v2 资产的 24 条坐标是赛事用于平衡先手的指定起始局面，不是某一方在自由局面中选择的高质量建议。
2. **选择策略错误**：旧查询在八种对称匹配后，对所有合法延续均匀抽样；没有胜率、棋手/引擎强度、独立对局数、赛事数或来源数门槛。
3. **支持度缺失**：单条指定线即可成为默认着，重复位置证据与执黑/执白统计均不存在。
4. **证明门错误**：`NO_FORCED_WIN_IN_SCOPE` 只表示限定深度内未找到强制杀，并不证明该着与四星原着战略等价；旧实现却让开局着先成为默认，只在证明出明显战术失败时撤销。

因此五行级不得对旧资产重新调权。新方案必须先计算完全不看库的四星着，只把顶级完整对局中“指定开局结束后的真实决策”聚合为当前局面候选，并在任何证据不足或不可比时原样返回四星着。

## 审计文件

- `failed_book_root_cause.json`：全部 200 个配对、首次分叉、book ID/ply、证明状态和双方结果。
- `baseline_manifest.json`：四星、冻结旧三星、v2 资产及纠正后正式报告的配置快照与 SHA-256。
"""
    (args.output / "failed_book_root_cause.md").write_text(md, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
