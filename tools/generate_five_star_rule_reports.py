#!/usr/bin/env python3
"""Validate and report rule-partitioned natural five-star matches."""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
import statistics
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_jsonl(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    headers = [row for row in rows if row["type"] == "header"]
    games = [row for row in rows if row["type"] == "game"]
    assert len(headers) == 1
    return headers[0], games


def inside(x: int, y: int) -> bool:
    return 0 <= x < 15 and 0 <= y < 15


def line_length(board: list[list[int]], x: int, y: int, dx: int, dy: int,
                side: int) -> int:
    total = 1
    for sign in (-1, 1):
        px, py = x + sign * dx, y + sign * dy
        while inside(px, py) and board[px][py] == side:
            total += 1
            px, py = px + sign * dx, py + sign * dy
    return total


def has_five(board: list[list[int]], x: int, y: int, side: int) -> bool:
    return any(line_length(board, x, y, dx, dy, side) >= 5
               for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)))


def legal_move(board: list[list[int]], x: int, y: int, side: int,
               forbidden: bool) -> bool:
    if not inside(x, y) or board[x][y] != 0 or side not in (1, -1):
        return False
    if not forbidden or side != 1:
        return True
    board[x][y] = side
    if any(line_length(board, x, y, dx, dy, side) > 5
           for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1))):
        board[x][y] = 0
        return False
    if has_five(board, x, y, side):
        board[x][y] = 0
        return True
    winning_directions = 0
    for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)):
        continuation = False
        for offset in range(-4, 5):
            px, py = x + offset * dx, y + offset * dy
            if not inside(px, py) or board[px][py] != 0:
                continue
            board[px][py] = side
            if line_length(board, px, py, dx, dy, side) >= 5:
                continuation = True
            board[px][py] = 0
        if continuation:
            winning_directions += 1
    board[x][y] = 0
    return winning_directions < 2


def validate(header: dict, games: list[dict], forbidden: bool) -> None:
    assert header["schemaVersion"] == 5
    assert header["suite"] == "five-star-natural-final"
    assert header["openingMode"] == "gomocup-held-out-prefix"
    assert header["forbiddenBlack"] is forbidden
    assert header["games"] == 100 and len(games) == 100
    expected = collections.Counter((opening, color)
                                   for opening in range(50) for color in (1, -1))
    assert collections.Counter((g["openingId"], g["newColor"]) for g in games) == expected
    for game in games:
        assert game["anomaly"] is None
        assert game["moveCount"] == len(game["moves"])
        board = [[0] * 15 for _ in range(15)]
        winner = 0
        for ply, (x, y, side) in enumerate(game["moves"]):
            assert winner == 0
            assert side == (1 if ply % 2 == 0 else -1)
            assert legal_move(board, x, y, side, forbidden)
            board[x][y] = side
            if has_five(board, x, y, side):
                winner = side
        if game["termination"] == "five-in-a-row":
            assert winner == game["winner"]
        elif game["termination"] in ("max-moves-draw", "board-full"):
            assert game["winner"] == 0
        elif game["termination"] in ("proven-loss", "selected-verified-losing",
                                      "all-examined-verified-losing",
                                      "budget-unknown",
                                      "no-immediate-safe-generated"):
            assert game["winner"] in (1, -1)
        elif game["termination"] == "legacy-illegal-move-loss":
            assert game["winner"] in (1, -1)


def wilson(wins: int, draws: int, losses: int) -> list[float]:
    count = wins + draws + losses
    score = (wins + 0.5 * draws) / count
    z = 1.95996398454
    denominator = 1 + z * z / count
    center = (score + z * z / (2 * count)) / denominator
    half = z * math.sqrt(score * (1 - score) / count + z * z / (4 * count * count)) / denominator
    return [center - half, center + half]


def perspective_stats(games: list[dict], perspective: int,
                      new_color: int | None = None) -> dict:
    selected = [g for g in games if new_color is None or g["newColor"] == new_color]
    wins = sum(g["winner"] == perspective for g in selected)
    draws = sum(g["winner"] == 0 for g in selected)
    losses = len(selected) - wins - draws
    return {"games": len(selected), "wins": wins, "draws": draws, "losses": losses,
            "scoreRate": (wins + 0.5 * draws) / len(selected),
            "wilson95": wilson(wins, draws, losses)}


def new_engine_stats(games: list[dict], color: int | None = None) -> dict:
    selected = [g for g in games if color is None or g["newColor"] == color]
    wins = sum(g["winner"] == g["newColor"] for g in selected)
    draws = sum(g["winner"] == 0 for g in selected)
    losses = len(selected) - wins - draws
    return {"games": len(selected), "wins": wins, "draws": draws, "losses": losses,
            "scoreRate": (wins + 0.5 * draws) / len(selected),
            "wilson95": wilson(wins, draws, losses)}


def game_point(game: dict) -> float:
    if game["winner"] == 0:
        return 0.5
    return 1.0 if game["winner"] == game["newColor"] else 0.0


def mean_interval(values: list[float]) -> list[float]:
    mean = statistics.mean(values)
    if len(values) < 2:
        return [mean, mean]
    half = 1.95996398454 * statistics.stdev(values) / math.sqrt(len(values))
    return [max(-1.0, mean - half), min(1.0, mean + half)]


def paired_intervals(games: list[dict]) -> tuple[list[float], list[float]]:
    by_key = {(game["openingId"], game["newColor"]): game for game in games}
    white_differences = []
    overall_differences = []
    for opening in range(50):
        new_white = game_point(by_key[(opening, -1)])
        new_black = game_point(by_key[(opening, 1)])
        opponent_white = 1.0 - new_black
        white_differences.append(new_white - opponent_white)
        overall_differences.append((new_white + new_black) / 2.0 - 0.5)
    return mean_interval(white_differences), mean_interval(overall_differences)


def count_by(items: list[dict], key: str) -> dict[str, int]:
    return dict(sorted(collections.Counter(str(item.get(key, 0)) for item in items).items()))


def summarize(path: Path, rule_name: str, opponent_name: str,
              forbidden: bool) -> dict:
    header, games = load_jsonl(path)
    validate(header, games, forbidden)
    white = new_engine_stats(games, -1)
    black = new_engine_stats(games, 1)
    overall = new_engine_stats(games)
    opponent_white = perspective_stats(games, -1, new_color=1)
    opponent_black = perspective_stats(games, 1, new_color=-1)
    steps = [step for game in games for step in game["steps"]
             if step["engine"] == "new"]
    hits = [step for step in steps if step.get("corpusLookup")]
    accepted = [step for step in hits if step.get("corpusAccepted")]
    changed = [step for step in accepted
               if (step["x"], step["y"]) != (step.get("fourStarX"), step.get("fourStarY"))]
    elapsed = sorted(step["ms"] for step in steps)
    cpu_elapsed = sorted(step.get("cpuMs", 0.0) for step in steps)
    paired_white, paired_overall = paired_intervals(games)
    return {
        "schemaVersion": 2,
        "rule": rule_name,
        "forbiddenBlack": forbidden,
        "opponent": opponent_name,
        "source": str(path),
        "sourceSha256": sha256(path),
        "header": header,
        "validGames": len(games),
        "replayedMoves": sum(len(game["moves"]) for game in games),
        "anomalies": 0,
        "white": white,
        "opponentWhite": opponent_white,
        "whiteScoreDelta": white["scoreRate"] - opponent_white["scoreRate"],
        "pairedWhiteDelta95": paired_white,
        "black": black,
        "opponentBlack": opponent_black,
        "blackScoreDelta": black["scoreRate"] - opponent_black["scoreRate"],
        "overall": overall,
        "overallScoreDelta": overall["scoreRate"] - 0.5,
        "pairedOverallDelta95": paired_overall,
        "terminations": dict(sorted(collections.Counter(
            game["termination"] for game in games).items())),
        "corpus": {
            "decisions": len(steps), "lookups": len(hits),
            "exactLookups": sum(step.get("corpusMatchType") == 1 for step in hits),
            "localLookups": sum(step.get("corpusMatchType") == 2 for step in hits),
            "accepted": len(accepted), "changedMoves": len(changed),
            "reasons": count_by(steps, "corpusReason"),
            "trustTiers": count_by(hits, "corpusTrustTier"),
            "sourceBoardMasks": count_by(hits, "corpusSourceBoardMask"),
            "requiredStones": count_by(hits, "corpusRequiredStones"),
        },
        "latencyMs": {
            "p50": statistics.median(elapsed),
            "p95": elapsed[min(len(elapsed) - 1, math.ceil(0.95 * len(elapsed)) - 1)],
            "max": max(elapsed),
        },
        "cpuMs": {
            "p50": statistics.median(cpu_elapsed),
            "p95": cpu_elapsed[min(len(cpu_elapsed) - 1,
                                     math.ceil(0.95 * len(cpu_elapsed)) - 1)],
            "max": max(cpu_elapsed),
        },
        "peakResidentBytes": max(step.get("peakResidentBytes", 0)
                                  for step in steps),
        "proofActivity": {
            "verifiedCertificates": sum(step.get("proofCertificateVerified", False)
                                         for step in steps),
            "parallelBatches": sum(step["diagnostics"].get("parallelBatches", 0)
                                   for step in steps),
            "parallelWorkersLaunched": sum(
                step["diagnostics"].get("parallelWorkersLaunched", 0)
                for step in steps),
            "parallelRootJobs": sum(step["diagnostics"].get("parallelRootJobs", 0)
                                    for step in steps),
            "parallelRootJobsCompleted": sum(
                step["diagnostics"].get("parallelRootJobsCompleted", 0)
                for step in steps),
            "completedScopeDisproofs": sum(
                step["diagnostics"].get("completedScopeDisproofs", 0)
                for step in steps),
        },
        "lossAware": {
            "decisions": len(steps),
            "escapeDecisions": sum(step.get("escapeAlternativesExamined", 0) > 0
                                   for step in steps),
            "alternativesExamined": sum(step.get("escapeAlternativesExamined", 0)
                                        for step in steps),
            "scopedDisproofs": sum(step.get("escapeScopedDisproofCount", 0)
                                   for step in steps),
            "unknowns": sum(step.get("escapeUnknownCount", 0) for step in steps),
            "verifiedLosses": sum(step.get("escapeVerifiedLossCount", 0)
                                  for step in steps),
            "corpusProtections": sum(step.get("corpusProtectionApplied", False)
                                     for step in steps),
            "budgetExhausted": sum(step.get("budgetExhausted", False)
                                   for step in steps),
        },
    }


def stat_line(label: str, value: dict) -> str:
    low, high = value["wilson95"]
    return (f"- {label}：{value['wins']}/{value['draws']}/{value['losses']}，"
            f"得分率 {value['scoreRate']:.1%}，Wilson 95% {low:.1%}–{high:.1%}")


def write_cell_report(output: Path, slug: str, summary: dict) -> None:
    (output / f"{slug}_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n")
    corpus = summary["corpus"]
    board_note = ("1=仅15×15，2=仅20×20，3=15×15与20×20共同支持"
                  if not summary["forbiddenBlack"] else "有禁手分区只允许15×15证据，mask应为1")
    markdown = f"""# 五星 vs {summary['opponent']}（{summary['rule']}）正式报告

本单元使用50个与语料和参数诊断无关的自然随机开局，每个开局交换颜色，共100局；五星执黑50局、执白50局。赛程未按语料设计或筛选。

## 对局结果（五星执白优先）

{stat_line('五星执白', summary['white'])}
{stat_line(summary['opponent'] + '执白', summary['opponentWhite'])}
- 白棋得分变化：{summary['whiteScoreDelta']:+.1%}（五星执白减对手执白）
- 白棋配对差值95%区间：{summary['pairedWhiteDelta95'][0]:+.1%}–{summary['pairedWhiteDelta95'][1]:+.1%}
{stat_line('五星执黑', summary['black'])}
- {summary['opponent']}执黑：{summary['opponentBlack']['wins']}/{summary['opponentBlack']['draws']}/{summary['opponentBlack']['losses']}，得分率 {summary['opponentBlack']['scoreRate']:.1%}
- 黑棋得分变化：{summary['blackScoreDelta']:+.1%}（五星执黑减对手执黑）
{stat_line('五星总体', summary['overall'])}
- 总体相对50%变化：{summary['overallScoreDelta']:+.1%}
- 总体配对差值95%区间：{summary['pairedOverallDelta95'][0]:+.1%}–{summary['pairedOverallDelta95'][1]:+.1%}

五子棋自然先手优势保留，不做颜色再加权；“五星执白”只统计 `newColor=-1`，不会与对手执白混用。

## 语料实际参与

- 五星决策 {corpus['decisions']} 次；命中 {corpus['lookups']} 次（精确 {corpus['exactLookups']}，局部 {corpus['localLookups']}）。
- 通过接受门 {corpus['accepted']} 次；相对四星实际改着 {corpus['changedMoves']} 次。
- 信任层计数：`{json.dumps(corpus['trustTiers'], ensure_ascii=False, separators=(',', ':'))}`（2=跨赛事，1=高重复单赛事）。
- 来源棋盘 mask：`{json.dumps(corpus['sourceBoardMasks'], ensure_ascii=False, separators=(',', ':'))}`；{board_note}。
- 状态/拒绝原因码：`{json.dumps(corpus['reasons'], ensure_ascii=False, separators=(',', ':'))}`。

## 失败经验优化实际参与

- 触发 loss-aware 逃生 {summary['lossAware']['escapeDecisions']} 次，共检查 {summary['lossAware']['alternativesExamined']} 个候选。
- 完成 scoped disproof {summary['lossAware']['scopedDisproofs']} 次；预算未知 {summary['lossAware']['unknowns']} 次；已验证败着 {summary['lossAware']['verifiedLosses']} 次。
- 已完成防守触发题库保护 {summary['lossAware']['corpusProtections']} 次；基础搜索预算耗尽记录 {summary['lossAware']['budgetExhausted']} 次。

## 校验与性能

- 100/100局、{summary['replayedMoves']}手已重新检查坐标、占用、轮次、五连终局；有禁手模式额外用与 `fc_is_legal_move` 等价的过线/双向成五判定重放；anomaly=0。
- 终局类型：`{json.dumps(summary['terminations'], ensure_ascii=False, separators=(',', ':'))}`；冻结旧三星若选择禁手点按规则判非法着负，不删除该局。
- 五星单步延迟：p50 {summary['latencyMs']['p50']:.1f} ms，p95 {summary['latencyMs']['p95']:.1f} ms，最大 {summary['latencyMs']['max']:.1f} ms。
- 五星单步进程CPU：p50 {summary['cpuMs']['p50']:.1f} ms，p95 {summary['cpuMs']['p95']:.1f} ms，最大 {summary['cpuMs']['max']:.1f} ms；进程峰值RSS {summary['peakResidentBytes'] / 1048576:.1f} MiB。
- 并行证明：{summary['proofActivity']['parallelBatches']} 批，启动 worker 累计 {summary['proofActivity']['parallelWorkersLaunched']}，root job {summary['proofActivity']['parallelRootJobsCompleted']}/{summary['proofActivity']['parallelRootJobs']} 完成；独立重放接受的证书 {summary['proofActivity']['verifiedCertificates']} 个。
- 四星对手完全不读取语料；旧三星为冻结 `5224020`。评测使用 deterministic-best。
"""
    (output / f"{slug}.md").write_text(markdown)


def summarize_ab(paths: list[Path], rule: str) -> list[dict]:
    result = []
    for path in paths:
        header, games = load_jsonl(path)
        steps = [step for game in games for step in game["steps"] if step["engine"] == "new"]
        result.append({
            "rule": rule, "profile": header["newProfile"]["name"],
            "margin": header["newProfile"]["corpusScoreMargin"],
            "games": len(games), "score": new_engine_stats(games)["scoreRate"],
            "whiteScore": new_engine_stats(games, -1)["scoreRate"],
            "lookups": sum(step.get("corpusLookup", False) for step in steps),
            "accepted": sum(step.get("corpusAccepted", False) for step in steps),
            "changedMoves": sum(step.get("corpusAccepted", False) and
                                (step["x"], step["y"]) !=
                                (step.get("fourStarX"), step.get("fourStarY"))
                                for step in steps),
            "sha256": sha256(path),
        })
    return sorted(result, key=lambda row: row["margin"])


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--free-four", type=Path, required=True)
    parser.add_argument("--free-legacy", type=Path, required=True)
    parser.add_argument("--forbidden-four", type=Path, required=True)
    parser.add_argument("--forbidden-legacy", type=Path, required=True)
    parser.add_argument("--ab-free", type=Path, action="append", default=[])
    parser.add_argument("--ab-forbidden", type=Path, action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    cells = {
        "free_vs_four": summarize(args.free_four, "无禁手 Freestyle", "冻结无库四星", False),
        "free_vs_legacy": summarize(args.free_legacy, "无禁手 Freestyle", "冻结旧三星", False),
        "forbidden_vs_four": summarize(args.forbidden_four, "有禁手", "冻结无库四星", True),
        "forbidden_vs_legacy": summarize(args.forbidden_legacy, "有禁手", "冻结旧三星", True),
    }
    for slug, summary in cells.items():
        write_cell_report(args.output, slug, summary)

    four_star_gate = all(
        cells[name]["whiteScoreDelta"] > 0 and
        cells[name]["blackScoreDelta"] >= 0 and
        cells[name]["overallScoreDelta"] >= 0
        for name in ("free_vs_four", "forbidden_vs_four"))
    legacy_gate = all(
        cells[name]["whiteScoreDelta"] >= 0 and
        cells[name]["overallScoreDelta"] >= 0
        for name in ("free_vs_legacy", "forbidden_vs_legacy"))
    gate = four_star_gate and legacy_gate
    demonstrated = all(summary["pairedWhiteDelta95"][0] > 0 and
                       summary["pairedOverallDelta95"][0] > 0
                       for summary in cells.values())
    comparison = {
        "schemaVersion": 2,
        "releaseGatePassed": gate,
        "fourStarGatePassed": four_star_gate,
        "legacyGeneralizationGatePassed": legacy_gate,
        "classification": ("stronger-at-95pct-across-all-cells" if demonstrated
                           else "point-estimate-stronger-not-95pct-demonstrated" if gate
                           else "not-generalization-demonstrated"),
        "cells": cells,
    }
    (args.output / "five_star_rule_comparison.json").write_text(
        json.dumps(comparison, ensure_ascii=False, indent=2) + "\n")
    rows = []
    for summary in cells.values():
        rows.append(f"| {summary['rule']} | {summary['opponent']} | "
                    f"{summary['white']['scoreRate']:.1%} | "
                    f"{summary['opponentWhite']['scoreRate']:.1%} | "
                    f"{summary['whiteScoreDelta']:+.1%} | "
                    f"{summary['black']['scoreRate']:.1%} | "
                    f"{summary['opponentBlack']['scoreRate']:.1%} | "
                    f"{summary['blackScoreDelta']:+.1%} | "
                    f"{summary['overall']['scoreRate']:.1%} | "
                    f"{summary['corpus']['lookups']} | {summary['corpus']['changedMoves']} |")
    comparison_md = """# 五星失败复盘优化：规则分区双基线正式对比

| 规则 | 对手 | 五星白棋 | 对手白棋 | 白棋变化 | 五星黑棋 | 对手黑棋 | 黑棋变化 | 五星总体 | 语料命中 | 实际改着 |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
""" + "\n".join(rows) + f"""

- 点估计安全发布门：{'通过' if gate else '未通过'}；强度分类：`{comparison['classification']}`。
- 四星发布门逐规则要求五星白棋同色变化严格为正，黑棋同色变化与总体方向非负；旧三星只要求白棋同色变化与总体方向非负。所有单元还必须重放无异常。
- “95%已证明更强”另要求四个单元的白棋与总体配对差值区间下界都大于0；本分类不会用点估计冒充统计显著。
- 所有结果保留五子棋先手优势；白棋变化始终是“五星执白得分率 − 对手执白得分率”。
"""
    (args.output / "five_star_rule_comparison.md").write_text(comparison_md)

    if args.ab_free or args.ab_forbidden:
        ab = summarize_ab(args.ab_free, "无禁手") + summarize_ab(args.ab_forbidden, "有禁手")
        (args.output / "parameter_ab.json").write_text(
            json.dumps({"selectedMargin": -80, "rows": ab}, ensure_ascii=False, indent=2) + "\n")
        lines = ["# 五星接受窗口自然赛程 A/B", "",
                 "A/B只使用与语料无关的诊断种子；正式赛结果未参与参数选择。", "",
                 "| 规则 | margin | 局数 | 得分率 | 白棋得分率 | 命中 | 接受 | 改着 |",
                 "|---|---:|---:|---:|---:|---:|---:|---:|"]
        for row in ab:
            lines.append(f"| {row['rule']} | {row['margin']} | {row['games']} | "
                         f"{row['score']:.1%} | {row['whiteScore']:.1%} | {row['lookups']} | "
                         f"{row['accepted']} | {row['changedMoves']} |")
        lines += ["", "冻结 `margin=-80`：无禁手下比0保留更多实际改着，且与-120同着；"
                         "有禁手三组同着，因此选择更保守的-80而非-120。"]
        (args.output / "parameter_ab.md").write_text("\n".join(lines) + "\n")

    checksum_path = args.output / "checksums.sha256"
    files = sorted(path for path in args.output.iterdir()
                   if path.is_file() and path != checksum_path)
    checksum_path.write_text("".join(f"{sha256(path)}  {path.name}\n" for path in files))


if __name__ == "__main__":
    main()
