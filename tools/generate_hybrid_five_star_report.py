#!/usr/bin/env python3
"""Generate color-correct reports for the stochastic five-star hybrid."""

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
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1 and len(games) == 100
    return headers[0], games


def wilson(wins: int, draws: int, losses: int) -> list[float]:
    count = wins + draws + losses
    score = (wins + 0.5 * draws) / count
    z = 1.95996398454
    denominator = 1 + z * z / count
    center = (score + z * z / (2 * count)) / denominator
    half = z * math.sqrt(score * (1 - score) / count +
                         z * z / (4 * count * count)) / denominator
    return [center - half, center + half]


def stats(games: list[dict], model: bool, color: int | None = None) -> dict:
    selected = [game for game in games
                if color is None or (game["newColor"] if model
                                     else -game["newColor"]) == color]
    wins = sum(game["winner"] == (game["newColor"] if model
                                  else -game["newColor"])
               for game in selected)
    draws = sum(game["winner"] == 0 for game in selected)
    losses = len(selected) - wins - draws
    return {"games": len(selected), "wins": wins, "draws": draws,
            "losses": losses,
            "scoreRate": (wins + 0.5 * draws) / len(selected),
            "wilson95": wilson(wins, draws, losses)}


def point(game: dict) -> float:
    return 0.5 if game["winner"] == 0 else float(
        game["winner"] == game["newColor"])


def interval(values: list[float]) -> list[float]:
    center = statistics.mean(values)
    if len(values) < 2:
        return [center, center]
    half = 1.95996398454 * statistics.stdev(values) / math.sqrt(len(values))
    return [max(-1.0, center - half), min(1.0, center + half)]


def paired_intervals(games: list[dict]) -> tuple[list[float], list[float]]:
    by_key = {(game["openingId"], game["newColor"]): game for game in games}
    openings = sorted({game["openingId"] for game in games})
    white, overall = [], []
    for opening in openings:
        model_white = point(by_key[(opening, -1)])
        model_black = point(by_key[(opening, 1)])
        white.append(model_white - (1.0 - model_black))
        overall.append((model_white + model_black) / 2.0 - 0.5)
    return interval(white), interval(overall)


def percentile(values: list[float], fraction: float) -> float:
    values = sorted(values)
    return values[min(len(values) - 1, math.ceil(fraction * len(values)) - 1)]


def component_summary(steps: list[dict]) -> dict:
    wall = [step["ms"] for step in steps]
    cpu = [step.get("cpuMs", 0.0) for step in steps]
    return {
        "decisions": len(steps),
        "latencyMs": {"p50": statistics.median(wall),
                      "p95": percentile(wall, 0.95), "max": max(wall)},
        "cpuMs": {"p50": statistics.median(cpu),
                  "p95": percentile(cpu, 0.95), "max": max(cpu)},
        "peakResidentBytes": max(step.get("peakResidentBytes", 0)
                                  for step in steps),
        "workerLaunches": sum(step.get("proofWorkersLaunched", 0)
                              for step in steps),
        "parallelJobs": sum(step.get("proofParallelJobs", 0)
                            for step in steps),
        "parallelJobsCompleted": sum(step.get("proofParallelJobsCompleted", 0)
                                     for step in steps),
    }


def summarize(path: Path, rule: str, opponent: str, forbidden: bool) -> dict:
    header, games = load(path)
    assert header["forbiddenBlack"] is forbidden
    model_white, opponent_white = stats(games, True, -1), stats(games, False, -1)
    model_black, opponent_black = stats(games, True, 1), stats(games, False, 1)
    overall = stats(games, True)
    paired_white, paired_overall = paired_intervals(games)
    steps = [step for game in games for step in game["steps"]
             if step["engine"] == "new"]
    white_steps = [step for step in steps if step["side"] == -1]
    black_steps = [step for step in steps if step["side"] == 1]
    assert all(step["hybridComponent"] == 2 for step in white_steps)
    assert all(step["hybridComponent"] == 1 for step in black_steps)
    random_steps = [step for step in steps if step["randomSelectionUsed"]]
    eligible_steps = [step for step in steps if step["randomCandidateCount"] > 1]
    return {
        "rule": rule, "forbiddenBlack": forbidden, "opponent": opponent,
        "source": str(path), "sourceSha256": sha256(path), "header": header,
        "games": len(games), "white": model_white,
        "opponentWhite": opponent_white,
        "whiteScoreDelta": model_white["scoreRate"] - opponent_white["scoreRate"],
        "pairedWhiteDelta95": paired_white,
        "black": model_black, "opponentBlack": opponent_black,
        "blackScoreDelta": model_black["scoreRate"] - opponent_black["scoreRate"],
        "overall": overall, "overallScoreDelta": overall["scoreRate"] - 0.5,
        "pairedOverallDelta95": paired_overall,
        "terminations": dict(sorted(collections.Counter(
            game["termination"] for game in games).items())),
        "routing": {"whiteProofEngine": len(white_steps),
                    "blackV51": len(black_steps), "mismatches": 0},
        "randomness": {
            "decisions": len(steps), "eligibleMultiCandidate": len(eligible_steps),
            "randomSelections": len(random_steps),
            "eligibilityFailures": sum(not step["randomEligibilityVerified"]
                                       for step in steps),
            "distinctEquivalenceSignatures": len({
                step["randomEquivalenceSignature"] for step in steps}),
            "budgetExhaustions": sum(step["budgetExhausted"] for step in steps),
        },
        "whiteComponent": component_summary(white_steps),
        "blackComponent": component_summary(black_steps),
    }


def stat_line(label: str, value: dict) -> str:
    low, high = value["wilson95"]
    return (f"- {label}：{value['wins']}/{value['draws']}/{value['losses']}，"
            f"得分率 {value['scoreRate']:.1%}，Wilson 95% {low:.1%}–{high:.1%}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--free-four", type=Path, required=True)
    parser.add_argument("--free-legacy", type=Path, required=True)
    parser.add_argument("--forbidden-four", type=Path, required=True)
    parser.add_argument("--forbidden-legacy", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    cells = {
        "free_vs_four": summarize(args.free_four, "无禁手", "冻结四星", False),
        "free_vs_legacy": summarize(args.free_legacy, "无禁手", "旧三星", False),
        "forbidden_vs_four": summarize(args.forbidden_four, "有禁手", "冻结四星", True),
        "forbidden_vs_legacy": summarize(args.forbidden_legacy, "有禁手", "旧三星", True),
    }
    replay = json.loads(args.replay.read_text(encoding="utf-8"))
    assert replay["status"] == "pass" and replay["games"] == 400
    four_gate = all(cells[name]["white"]["scoreRate"] >= 0.5 and
                    cells[name]["blackScoreDelta"] >= 0 and
                    cells[name]["overall"]["scoreRate"] >= 0.5
                    for name in ("free_vs_four", "forbidden_vs_four"))
    legacy_gate = all(cells[name]["whiteScoreDelta"] >= 0 and
                      cells[name]["overallScoreDelta"] >= 0
                      for name in ("free_vs_legacy", "forbidden_vs_legacy"))
    correctness_gate = all(
        cell["routing"]["mismatches"] == 0 and
        cell["randomness"]["eligibilityFailures"] == 0 and
        cell["whiteComponent"]["latencyMs"]["max"] <= 5000 and
        cell["blackComponent"]["latencyMs"]["max"] <= 5000
        for cell in cells.values())
    passed = four_gate and legacy_gate and correctness_gate
    demonstrated = all(cell["pairedWhiteDelta95"][0] > 0 and
                       cell["pairedOverallDelta95"][0] > 0
                       for cell in cells.values())
    decision = {
        "schemaVersion": 1, "releaseGatePassed": passed,
        "fourStarGatePassed": four_gate,
        "legacyGeneralizationGatePassed": legacy_gate,
        "correctnessLatencyGatePassed": correctness_gate,
        "classification": ("demonstrated-stronger" if passed and demonstrated
                           else "point-estimate-pass-statistically-inconclusive"
                           if passed else "not-demonstrated"),
        "productionAction": "keep-v5.1-pending-explicit-promotion",
    }
    summary = {"schemaVersion": 1, "cells": cells, "replay": replay,
               "releaseDecision": decision}
    (args.output / "hybrid_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8")
    (args.output / "release_decision.json").write_text(
        json.dumps(decision, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8")

    sections = []
    for cell in cells.values():
        sections.append(f"""## {cell['rule']} vs {cell['opponent']}

{stat_line('新五星执白（5.4.1 搜索）', cell['white'])}
{stat_line(cell['opponent'] + '执白', cell['opponentWhite'])}
- 白棋同色变化：{cell['whiteScoreDelta']:+.1%}；配对95%区间 {cell['pairedWhiteDelta95'][0]:+.1%}–{cell['pairedWhiteDelta95'][1]:+.1%}
{stat_line('新五星执黑（5.1 策略）', cell['black'])}
{stat_line(cell['opponent'] + '执黑', cell['opponentBlack'])}
- 黑棋同色变化：{cell['blackScoreDelta']:+.1%}
{stat_line('新五星总体', cell['overall'])}
- 总体相对50%：{cell['overallScoreDelta']:+.1%}；配对95%区间 {cell['pairedOverallDelta95'][0]:+.1%}–{cell['pairedOverallDelta95'][1]:+.1%}
- 随机：{cell['randomness']['decisions']} 次决策，{cell['randomness']['eligibleMultiCandidate']} 次存在多个等价候选，实际随机改选 {cell['randomness']['randomSelections']} 次，等价验证失败 {cell['randomness']['eligibilityFailures']} 次；预算耗尽 {cell['randomness']['budgetExhaustions']} 次。
- 路由：白引擎 {cell['routing']['whiteProofEngine']} 次，黑引擎 {cell['routing']['blackV51']} 次，错配 0 次。
- 白引擎延迟 p50/p95/max：{cell['whiteComponent']['latencyMs']['p50']:.1f}/{cell['whiteComponent']['latencyMs']['p95']:.1f}/{cell['whiteComponent']['latencyMs']['max']:.1f} ms；黑引擎：{cell['blackComponent']['latencyMs']['p50']:.1f}/{cell['blackComponent']['latencyMs']['p95']:.1f}/{cell['blackComponent']['latencyMs']['max']:.1f} ms。
""")
    markdown = f"""# 颜色专用新五星：随机自由对局测试报告

新模型执白使用 `5.4.1-transactional-deadline-root-parallel-5s`，执黑使用 `5.1.0-elite-rule-partitioned-local-v2`。对冻结四星和旧三星分别在无禁手、有禁手下进行100局自由对局，每个单元50次交换颜色，总计400局。

随机性按生产用户模式启用。不同种子或不同运行出现不同落子是预期行为；有效性依据是逐手合法、种子与组件来源正确、随机候选属于相同的完整证明/战术等价类，而不是要求另一次运行逐手相同。

""" + "\n".join(sections) + f"""
## 结论

- 四星门：{'通过' if four_gate else '未通过'}；旧三星泛化门：{'通过' if legacy_gate else '未通过'}；正确性与5秒门：{'通过' if correctness_gate else '未通过'}。
- 最终分类：`{decision['classification']}`。
- 当前产品动作：`{decision['productionAction']}`。本变更只验证候选，不自动替换玩家界面的5.1。
- 400局采用自然先手优势原始结果，不做颜色再加权；白棋变化始终是“新模型执白 − 对手执白”。

## 完整性说明

- 独立回放：{replay['games']} 局、{replay['moves']} 手；随机改选 {replay['randomSelections']} 次；证书记录审计 {replay['certificateRecordsAudited']} 次。
- 回放不要求独立随机运行复现同一手，但所有已记录路径均由生产C规则函数检查占用、轮次、禁手和终局；路由、派生种子、等价签名及随机接受门逐步核验。
"""
    report = args.output / "color_specialized_five_star_hybrid_report.md"
    report.write_text(markdown, encoding="utf-8")
    checksum = args.output / "checksums.sha256"
    files = sorted(path for path in args.output.iterdir()
                   if path.is_file() and path != checksum)
    checksum.write_text("".join(f"{sha256(path)}  {path.name}\n" for path in files),
                        encoding="utf-8")


if __name__ == "__main__":
    main()
