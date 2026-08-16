#!/usr/bin/env python3
"""Summarize the overlap-aware hybrid against the frozen four-star control."""

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


def load(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()]
    return next(row for row in rows if row.get("type") == "header"), \
        [row for row in rows if row.get("type") == "game"]


def wilson(wins: int, draws: int, losses: int) -> list[float]:
    n = wins + draws + losses
    score = (wins + 0.5 * draws) / n
    z = 1.95996398454
    den = 1.0 + z * z / n
    center = (score + z * z / (2.0 * n)) / den
    half = z * math.sqrt(score * (1.0 - score) / n + z * z /
                         (4.0 * n * n)) / den
    return [center - half, center + half]


def point(game: dict, side: int) -> float:
    return 0.5 if game["winner"] == 0 else float(game["winner"] == side)


def stats(games: list[dict], side: int, model: bool) -> dict:
    selected = [game for game in games
                if (game["newColor"] == side if model
                    else game["newColor"] == -side)]
    wins = sum(point(game, side) == 1.0 for game in selected)
    draws = sum(game["winner"] == 0 for game in selected)
    losses = len(selected) - wins - draws
    return {"games": len(selected), "wins": wins, "draws": draws,
            "losses": losses,
            "scoreRate": (wins + 0.5 * draws) / len(selected),
            "wilson95": wilson(wins, draws, losses)}


def mean_interval(values: list[float]) -> list[float]:
    center = statistics.mean(values)
    if len(values) < 2:
        return [center, center]
    half = 1.95996398454 * statistics.stdev(values) / math.sqrt(len(values))
    return [max(-1.0, center - half), min(1.0, center + half)]


def paired(games: list[dict], side: int | None) -> dict:
    by_key = {(int(game["openingId"]), int(game["newColor"])): game
              for game in games}
    values = []
    for opening in range(50):
        if side is None:
            values.append(sum(
                point(by_key[(opening, color)], color) -
                point(by_key[(opening, -color)], color)
                for color in (1, -1)) / 2.0)
        else:
            values.append(point(by_key[(opening, side)], side) -
                         point(by_key[(opening, -side)], side))
    return {"openingPairs": len(values), "delta": statistics.mean(values),
            "paired95": mean_interval(values)}


def component_summary(steps: list[dict]) -> dict:
    wall = [float(step["ms"]) for step in steps]
    cpu = [float(step.get("cpuMs", 0.0)) for step in steps]
    diagnostics = [step["diagnostics"] for step in steps]
    percentile = lambda values, p: sorted(values)[min(
        len(values) - 1, math.ceil(p * len(values)) - 1)]
    return {
        "decisions": len(steps),
        "latencyMs": {"p50": statistics.median(wall),
                      "p95": percentile(wall, 0.95), "max": max(wall)},
        "cpuMs": {"p50": statistics.median(cpu),
                  "p95": percentile(cpu, 0.95), "max": max(cpu),
                  "total": sum(cpu)},
        "budgetExhaustions": sum(bool(step["budgetExhausted"]) for step in steps),
        "workerLaunches": sum(int(step.get("proofWorkersLaunched", 0))
                               for step in steps),
        "parallelJobs": sum(int(step.get("proofParallelJobs", 0))
                             for step in steps),
        "parallelJobsCompleted": sum(
            int(step.get("proofParallelJobsCompleted", 0)) for step in steps),
        "parallelDecisions": sum(
            int(step.get("proofWorkersLaunched", 0)) > 0 for step in steps),
        "overlapPairs": sum(int(d["parallelOverlapPairs"]) for d in diagnostics),
        "overlapGroups": sum(int(d["parallelOverlapGroups"]) for d in diagnostics),
        "largestOverlapGroup": max(
            int(d["parallelLargestOverlapGroup"]) for d in diagnostics),
        "rootJobs": sum(int(d["parallelRootJobs"]) for d in diagnostics),
        "rootJobsCompleted": sum(
            int(d["parallelRootJobsCompleted"]) for d in diagnostics),
        "budgetTokens": sum(int(d["parallelBudgetTokens"]) for d in diagnostics),
        "randomSelections": sum(bool(step["randomSelectionUsed"])
                                 for step in steps),
        "eligibleMultiCandidate": sum(
            int(step["randomCandidateCount"]) > 1 for step in steps),
        "certificates": sum(bool(step["proofCertificateVerified"])
                             for step in steps),
    }


def summarize(path: Path, rule: str, forbidden: bool) -> dict:
    header, games = load(path)
    assert len(games) == 100 and header["forbiddenBlack"] is forbidden
    new_white = stats(games, -1, True)
    new_black = stats(games, 1, True)
    control_white = stats(games, -1, False)
    control_black = stats(games, 1, False)
    new_overall = {
        "games": 100,
        "wins": sum(game["winner"] == game["newColor"] for game in games),
        "draws": sum(game["winner"] == 0 for game in games),
        "losses": sum(game["winner"] not in (0, game["newColor"])
                       for game in games),
    }
    new_overall["scoreRate"] = (new_overall["wins"] +
                                 0.5 * new_overall["draws"]) / 100.0
    new_overall["wilson95"] = wilson(new_overall["wins"],
                                      new_overall["draws"],
                                      new_overall["losses"])
    control_overall = {
        "games": 100,
        "wins": sum(game["winner"] == -game["newColor"] for game in games),
        "draws": new_overall["draws"],
        "losses": sum(game["winner"] not in (0, -game["newColor"])
                       for game in games),
    }
    control_overall["scoreRate"] = (control_overall["wins"] +
                                     0.5 * control_overall["draws"]) / 100.0
    control_overall["wilson95"] = wilson(control_overall["wins"],
                                          control_overall["draws"],
                                          control_overall["losses"])
    new_steps = [step for game in games for step in game["steps"]
                 if step["engine"] == "new"]
    return {
        "rule": rule, "forbiddenBlack": forbidden, "source": str(path),
        "sourceSha256": sha256(path), "header": header,
        "games": len(games), "newWhite": new_white,
        "controlWhite": control_white,
        "whiteDelta": new_white["scoreRate"] - control_white["scoreRate"],
        "newBlack": new_black, "controlBlack": control_black,
        "blackDelta": new_black["scoreRate"] - control_black["scoreRate"],
        "newOverall": new_overall, "controlOverall": control_overall,
        "overallDelta": new_overall["scoreRate"] - control_overall["scoreRate"],
        "pairedWhite": paired(games, -1),
        "pairedBlack": paired(games, 1),
        "pairedOverall": paired(games, None),
        "terminations": dict(collections.Counter(
            game["termination"] for game in games)),
        "newComponent": component_summary(new_steps),
        "anomalies": sum(game["anomaly"] is not None for game in games),
    }


def line(label: str, value: dict) -> str:
    low, high = value["wilson95"]
    return (f"- {label}：{value['wins']}/{value['draws']}/{value['losses']}，"
            f"得分率 {value['scoreRate']:.1%}，Wilson 95% "
            f"{low:.1%}–{high:.1%}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--free", type=Path, required=True)
    parser.add_argument("--forbidden", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--old-summary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    cells = {
        "free": summarize(args.free, "无禁手", False),
        "forbidden": summarize(args.forbidden, "有禁手", True),
    }
    replay = json.loads(args.replay.read_text(encoding="utf-8"))
    old = json.loads(args.old_summary.read_text(encoding="utf-8"))["cells"]
    correctness = (replay["status"] == "pass" and replay["games"] == 200 and
                   all(cell["anomalies"] == 0 and
                       cell["newComponent"]["latencyMs"]["max"] <= 5000.0
                       for cell in cells.values()))
    white_gate = all(cell["newWhite"]["scoreRate"] >= 0.5
                     for cell in cells.values())
    black_gate = all(cell["blackDelta"] >= 0.0 for cell in cells.values())
    overall_gate = all(cell["overallDelta"] >= 0.0 for cell in cells.values())
    point_pass = correctness and white_gate and black_gate and overall_gate
    demonstrated = point_pass and all(
        cell["pairedWhite"]["paired95"][0] > 0 and
        cell["pairedOverall"]["paired95"][0] > 0
        for cell in cells.values())
    decision = {
        "correctnessLatencyGate": correctness,
        "whitePointGate": white_gate,
        "blackSameColorPointGate": black_gate,
        "overallPointGate": overall_gate,
        "classification": "demonstrated" if demonstrated else
            "point-estimate-pass-statistically-inconclusive"
            if point_pass else "not-demonstrated",
        "productionAction": "keep-existing-five-star-until-explicit-promotion",
    }
    summary = {"schemaVersion": 1, "candidate": "5.6.2-overlap-aware-grouped",
               "cells": cells, "oldParallelReference": {
                   "free": old["free_parallel"],
                   "forbidden": old["forbidden_parallel"]},
               "replay": replay, "releaseDecision": decision}
    (args.output / "overlap_grouped_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    sections = []
    for key in ("free", "forbidden"):
        cell = cells[key]
        activity = cell["newComponent"]
        old_cell = old[f"{'free' if key == 'free' else 'forbidden'}_parallel"]
        sections.append(f"""## {cell['rule']} vs 冻结四星

{line('新五星执白', cell['newWhite'])}
{line('冻结四星执白', cell['controlWhite'])}
- 白棋同色变化：{cell['whiteDelta']:+.1%}；配对95%区间 {cell['pairedWhite']['paired95'][0]:+.1%}–{cell['pairedWhite']['paired95'][1]:+.1%}
{line('新五星执黑', cell['newBlack'])}
{line('冻结四星执黑', cell['controlBlack'])}
- 黑棋同色变化：{cell['blackDelta']:+.1%}；配对95%区间 {cell['pairedBlack']['paired95'][0]:+.1%}–{cell['pairedBlack']['paired95'][1]:+.1%}
{line('新五星总体', cell['newOverall'])}
{line('冻结四星总体', cell['controlOverall'])}
- 总体变化：{cell['overallDelta']:+.1%}；配对95%区间 {cell['pairedOverall']['paired95'][0]:+.1%}–{cell['pairedOverall']['paired95'][1]:+.1%}
- 旧并行 v5.2.1 同色参考（非同种子配对）：白 {old_cell['white']['scoreRate']:.1%}，黑 {old_cell['black']['scoreRate']:.1%}，总体 {old_cell['overall']['scoreRate']:.1%}。
- 搜索活动：{activity['decisions']} 次新模型决策；p50/p95/max {activity['latencyMs']['p50']:.1f}/{activity['latencyMs']['p95']:.1f}/{activity['latencyMs']['max']:.1f}ms；worker {activity['workerLaunches']}，根任务 {activity['rootJobs']}（完成 {activity['rootJobsCompleted']}），重叠对 {activity['overlapPairs']}，重叠组 {activity['overlapGroups']}，最大组 {activity['largestOverlapGroup']}，预算耗尽 {activity['budgetExhaustions']} 次。
- 随机：{activity['eligibleMultiCandidate']} 次多候选，实际改选 {activity['randomSelections']} 次；证书 {activity['certificates']} 条；终局 {cell['terminations']}。
""")
    report = f"""# 重叠感知并行混合五星 vs 冻结四星测试报告

候选版本：`5.6.2-white-v541-black-v521-overlap-aware-parallel8-5s`。执白使用冻结的 5.4.1 证明引擎，执黑使用启用主动证明、候选依赖重叠分组和共享 DFPN/TT 会话的 v5.2.1 路径；并行上限 8 worker，单步硬上限 5 秒（内部搜索门禁 4.5 秒）。

正式赛程分无禁手、有禁手各 100 局，每个 opening 黑白各一局；决策种子分别为 `0xC7A3E51D20260815` 与 `0xD8B42F1C20260815`。棋谱前缀来自当前冻结的 `FiveChessFormalOpenings.inc`，未把对局结果反馈到参数。报告保留自然先手优势，不做颜色再加权；白棋结果是判断改进的重要指标。

""" + "\n".join(sections) + f"""
## 结论

- 正确性/5秒门：{'通过' if correctness else '未通过'}；白棋点估计门：{'通过' if white_gate else '未通过'}；黑棋同色点估计门：{'通过' if black_gate else '未通过'}；总体点估计门：{'通过' if overall_gate else '未通过'}。
- 分类：`{decision['classification']}`。配对区间仍跨过 0，因此不能只凭 100 局声称统计显著提升。
- 重叠处理实际生效：不同 gain 仍分别验证；共享组只复用后继 DFPN/TT 状态，未把重叠候选当成同一着法。
- 产品动作：`{decision['productionAction']}`；旧四星及低星路径未修改。

## 完整性

- 独立回放：{replay['games']} 局、{replay['moves']} 手、{replay['certificateRecordsAudited']} 条证书记录，状态 `{replay['status']}`。
- 原始记录：`{args.free}`、`{args.forbidden}`；回放摘要：`{args.replay}`。
- 旧并行参考来自 `reports/five_chess/five_star_parallel_v521_20260815/report/parallel_v521_summary.json`，仅作非同种子描述性对照。
"""
    (args.output / "overlap_grouped_vs_four_star_report.md").write_text(
        report, encoding="utf-8")
    files = sorted(path for path in args.output.iterdir() if path.is_file())
    (args.output / "checksums.sha256").write_text(
        "".join(f"{sha256(path)}  {path.name}\n" for path in files
                if path.name != "checksums.sha256"), encoding="utf-8")


if __name__ == "__main__":
    main()
