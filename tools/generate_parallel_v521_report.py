#!/usr/bin/env python3
"""Report the same-schedule serial/parallel v5.2.1-black evaluation."""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
import statistics
from pathlib import Path


PROFILE_VERSIONS = {
    "serial": "5.6.0-white-v541-black-v521-serial1-control-5s",
    "parallel": "5.6.1-white-v541-black-v521-parallel8-5s",
}
BLACK_COMPONENTS = {
    "serial": (4, "5.2.1-certificate-dependency-widening-1w-5s-control"),
    "parallel": (3, "5.2.2-certificate-dependency-root-parallel-8w-5s"),
}
WHITE_COMPONENT = (2, "5.4.1-transactional-deadline-root-parallel-5s")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path: Path, forbidden: bool, mode: str) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1 and len(games) == 100
    header = headers[0]
    assert header["suite"] == "five-star-parallel-v521-final"
    assert header["forbiddenBlack"] is forbidden
    assert header["newProfile"]["version"] == PROFILE_VERSIONS[mode]
    assert header["newProfile"]["proofWorkerCount"] == (8 if mode == "parallel" else 1)
    assert header["newProfile"]["parallelProofEnabled"] is (mode == "parallel")
    assert header["opponentProfile"] == "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book"
    assert sum(game["newColor"] == 1 for game in games) == 50
    assert sum(game["newColor"] == -1 for game in games) == 50
    assert all(game["anomaly"] is None for game in games)
    return header, games


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
    return {
        "games": len(selected), "wins": wins, "draws": draws, "losses": losses,
        "scoreRate": (wins + 0.5 * draws) / len(selected),
        "wilson95": wilson(wins, draws, losses),
    }


def model_point(game: dict) -> float:
    if game["winner"] == 0:
        return 0.5
    return float(game["winner"] == game["newColor"])


def mean_interval(values: list[float]) -> list[float]:
    center = statistics.mean(values)
    if len(values) < 2:
        return [center, center]
    half = 1.95996398454 * statistics.stdev(values) / math.sqrt(len(values))
    return [max(-1.0, center - half), min(1.0, center + half)]


def paired_effect(serial_games: list[dict], parallel_games: list[dict],
                  color: int | None) -> dict:
    serial = {(game["openingId"], game["newColor"]): game for game in serial_games}
    parallel = {(game["openingId"], game["newColor"]): game for game in parallel_games}
    assert serial.keys() == parallel.keys()
    openings = sorted({key[0] for key in serial})
    values = []
    for opening in openings:
        if color is None:
            value = statistics.mean(
                model_point(parallel[(opening, side)]) -
                model_point(serial[(opening, side)]) for side in (1, -1))
        else:
            value = (model_point(parallel[(opening, color)]) -
                     model_point(serial[(opening, color)]))
        values.append(value)
    return {"openingPairs": len(values), "delta": statistics.mean(values),
            "paired95": mean_interval(values)}


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, math.ceil(fraction * len(ordered)) - 1)]


def component_summary(steps: list[dict]) -> dict:
    wall = [float(step["ms"]) for step in steps]
    cpu = [float(step.get("cpuMs", 0.0)) for step in steps]
    caps = sorted({int(step.get("proofWorkerCap", 0)) for step in steps})
    return {
        "decisions": len(steps),
        "latencyMs": {"p50": statistics.median(wall),
                      "p95": percentile(wall, 0.95), "max": max(wall)},
        "cpuMs": {"p50": statistics.median(cpu),
                  "p95": percentile(cpu, 0.95), "max": max(cpu),
                  "total": sum(cpu)},
        "peakResidentBytes": max(int(step.get("peakResidentBytes", 0))
                                  for step in steps),
        "workerCapsObserved": caps,
        "workerLaunches": sum(int(step.get("proofWorkersLaunched", 0))
                              for step in steps),
        "parallelJobs": sum(int(step.get("proofParallelJobs", 0))
                            for step in steps),
        "parallelJobsCompleted": sum(
            int(step.get("proofParallelJobsCompleted", 0)) for step in steps),
        "parallelDecisions": sum(int(step.get("proofWorkersLaunched", 0)) > 0
                                 for step in steps),
        "budgetExhaustions": sum(bool(step["budgetExhausted"])
                                 for step in steps),
    }


def summarize(path: Path, rule: str, forbidden: bool, mode: str) -> dict:
    header, games = load(path, forbidden, mode)
    new_steps = [step for game in games for step in game["steps"]
                 if step["engine"] == "new"]
    opponent_steps = [step for game in games for step in game["steps"]
                      if step["engine"] != "new"]
    white_steps = [step for step in new_steps if step["side"] == -1]
    black_steps = [step for step in new_steps if step["side"] == 1]
    assert all((step["hybridComponent"], step["hybridComponentVersion"]) ==
               WHITE_COMPONENT for step in white_steps)
    assert all((step["hybridComponent"], step["hybridComponentVersion"]) ==
               BLACK_COMPONENTS[mode] for step in black_steps)
    assert all(int(step.get("proofWorkersLaunched", 0)) == 0 and
               int(step.get("proofParallelJobs", 0)) == 0
               for step in opponent_steps)
    if mode == "serial":
        assert all(int(step.get("proofWorkerCap", 0)) == 1 and
                   int(step.get("proofWorkersLaunched", 0)) == 0
                   for step in black_steps)
    else:
        assert all(int(step.get("proofWorkerCap", 0)) == 8 for step in black_steps)
        assert any(int(step.get("proofWorkersLaunched", 0)) > 0
                   for step in black_steps)
    white = stats(games, True, -1)
    black = stats(games, True, 1)
    opponent_white = stats(games, False, -1)
    opponent_black = stats(games, False, 1)
    overall = stats(games, True)
    return {
        "rule": rule, "forbiddenBlack": forbidden, "mode": mode,
        "source": str(path), "sourceSha256": sha256(path), "header": header,
        "games": len(games), "white": white, "black": black,
        "opponentWhite": opponent_white, "opponentBlack": opponent_black,
        "blackSameColorDelta": black["scoreRate"] - opponent_black["scoreRate"],
        "overall": overall,
        "terminations": dict(sorted(collections.Counter(
            game["termination"] for game in games).items())),
        "randomness": {
            "decisions": len(new_steps),
            "eligibleMultiCandidate": sum(step["randomCandidateCount"] > 1
                                          for step in new_steps),
            "randomSelections": sum(bool(step["randomSelectionUsed"])
                                    for step in new_steps),
            "eligibilityFailures": sum(not step["randomEligibilityVerified"]
                                       for step in new_steps),
            "distinctEquivalenceSignatures": len({
                step["randomEquivalenceSignature"] for step in new_steps}),
        },
        "whiteComponent": component_summary(white_steps),
        "blackComponent": component_summary(black_steps),
        "fourStarComponent": component_summary(opponent_steps),
    }


def stat_line(label: str, value: dict) -> str:
    low, high = value["wilson95"]
    return (f"- {label}：{value['wins']}/{value['draws']}/{value['losses']}，"
            f"得分率 {value['scoreRate']:.1%}，Wilson 95% {low:.1%}–{high:.1%}")


def effect_line(label: str, effect: dict) -> str:
    low, high = effect["paired95"]
    return (f"- {label}：{effect['delta']:+.1%}，配对95%区间 "
            f"{low:+.1%}–{high:+.1%}（{effect['openingPairs']}组开局）")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--free-serial", type=Path, required=True)
    parser.add_argument("--free-parallel", type=Path, required=True)
    parser.add_argument("--forbidden-serial", type=Path, required=True)
    parser.add_argument("--forbidden-parallel", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    cells = {
        "free_serial": summarize(args.free_serial, "无禁手", False, "serial"),
        "free_parallel": summarize(args.free_parallel, "无禁手", False, "parallel"),
        "forbidden_serial": summarize(args.forbidden_serial, "有禁手", True, "serial"),
        "forbidden_parallel": summarize(args.forbidden_parallel, "有禁手", True, "parallel"),
    }
    replay = json.loads(args.replay.read_text(encoding="utf-8"))
    assert replay["status"] == "pass" and replay["games"] == 400
    effects = {}
    gates = {}
    for prefix in ("free", "forbidden"):
        serial = cells[f"{prefix}_serial"]
        parallel = cells[f"{prefix}_parallel"]
        _, serial_games = load(Path(serial["source"]), serial["forbiddenBlack"], "serial")
        _, parallel_games = load(Path(parallel["source"]), parallel["forbiddenBlack"], "parallel")
        black_effect = paired_effect(serial_games, parallel_games, 1)
        overall_effect = paired_effect(serial_games, parallel_games, None)
        effects[prefix] = {"blackEightMinusOne": black_effect,
                           "overallEightMinusOne": overall_effect}
        strength = (parallel["white"]["scoreRate"] >= 0.5 and
                    parallel["blackSameColorDelta"] >= 0 and
                    parallel["overall"]["scoreRate"] >= 0.5)
        thread_points = (black_effect["delta"] >= 0 and
                         overall_effect["delta"] >= 0 and
                         (black_effect["delta"] > 0 or overall_effect["delta"] > 0))
        thread_demonstrated = thread_points and (
            (black_effect["delta"] > 0 and black_effect["paired95"][0] > 0) or
            (overall_effect["delta"] > 0 and overall_effect["paired95"][0] > 0))
        correctness = (
            parallel["randomness"]["eligibilityFailures"] == 0 and
            serial["randomness"]["eligibilityFailures"] == 0 and
            max(parallel["whiteComponent"]["latencyMs"]["max"],
                parallel["blackComponent"]["latencyMs"]["max"],
                serial["whiteComponent"]["latencyMs"]["max"],
                serial["blackComponent"]["latencyMs"]["max"]) <= 5000)
        gates[prefix] = {
            "candidateStrengthPointGatePassed": strength,
            "threadingPointGatePassed": thread_points,
            "threadingBenefitDemonstrated": thread_demonstrated,
            "correctnessLatencyGatePassed": correctness,
            "classification": ("demonstrated-threading-improvement"
                               if strength and correctness and thread_demonstrated
                               else "point-estimate-pass-statistically-inconclusive"
                               if strength and correctness and thread_points
                               else "not-demonstrated"),
        }
    release_pass = all(gate["candidateStrengthPointGatePassed"] and
                       gate["threadingPointGatePassed"] and
                       gate["correctnessLatencyGatePassed"]
                       for gate in gates.values())
    decision = {
        "schemaVersion": 1, "ruleGates": gates,
        "allRulePointEstimateGatesPassed": release_pass,
        "classification": ("demonstrated-threading-improvement"
                           if release_pass and all(g["threadingBenefitDemonstrated"]
                                                   for g in gates.values())
                           else "point-estimate-pass-statistically-inconclusive"
                           if release_pass else "not-demonstrated"),
        "productionAction": "keep-v5.1-pending-explicit-promotion",
    }
    summary = {"schemaVersion": 1, "cells": cells, "pairedEffects": effects,
               "replay": replay, "releaseDecision": decision}
    (args.output / "parallel_v521_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (args.output / "release_decision.json").write_text(
        json.dumps(decision, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    sections = []
    for prefix in ("free", "forbidden"):
        serial, parallel = cells[f"{prefix}_serial"], cells[f"{prefix}_parallel"]
        effect, gate = effects[prefix], gates[prefix]
        sections.append(f"""## {parallel['rule']}

### 8线程候选 vs 冻结四星

{stat_line('新五星执白（5.4.1）', parallel['white'])}
{stat_line('冻结四星执白', parallel['opponentWhite'])}
{stat_line('新五星执黑（并行 v5.2.1）', parallel['black'])}
{stat_line('冻结四星执黑', parallel['opponentBlack'])}
- 黑棋同色变化（新五星执黑 − 四星执黑）：{parallel['blackSameColorDelta']:+.1%}
{stat_line('新五星总体', parallel['overall'])}

### 同赛程线程对照

{stat_line('单线程混合模型执黑', serial['black'])}
{stat_line('8线程混合模型执黑', parallel['black'])}
{effect_line('8线程 − 单线程，黑棋', effect['blackEightMinusOne'])}
{stat_line('单线程混合模型总体', serial['overall'])}
{stat_line('8线程混合模型总体', parallel['overall'])}
{effect_line('8线程 − 单线程，总体', effect['overallEightMinusOne'])}
- 规则结论：`{gate['classification']}`；候选强度点估计门 {'通过' if gate['candidateStrengthPointGatePassed'] else '未通过'}，线程收益点估计门 {'通过' if gate['threadingPointGatePassed'] else '未通过'}，正确性/5秒门 {'通过' if gate['correctnessLatencyGatePassed'] else '未通过'}。
- 并行黑棋：{parallel['blackComponent']['decisions']} 次决策，实际并行 {parallel['blackComponent']['parallelDecisions']} 次，worker cap={parallel['blackComponent']['workerCapsObserved']}，累计启动 {parallel['blackComponent']['workerLaunches']} 个worker，完成 {parallel['blackComponent']['parallelJobsCompleted']}/{parallel['blackComponent']['parallelJobs']} 个root job。
- 并行黑棋 wall p50/p95/max：{parallel['blackComponent']['latencyMs']['p50']:.1f}/{parallel['blackComponent']['latencyMs']['p95']:.1f}/{parallel['blackComponent']['latencyMs']['max']:.1f} ms；CPU p50/p95/max：{parallel['blackComponent']['cpuMs']['p50']:.1f}/{parallel['blackComponent']['cpuMs']['p95']:.1f}/{parallel['blackComponent']['cpuMs']['max']:.1f} ms；峰值RSS {parallel['blackComponent']['peakResidentBytes'] / 1048576:.1f} MiB。
- 单线程黑棋 wall p50/p95/max：{serial['blackComponent']['latencyMs']['p50']:.1f}/{serial['blackComponent']['latencyMs']['p95']:.1f}/{serial['blackComponent']['latencyMs']['max']:.1f} ms；worker启动 {serial['blackComponent']['workerLaunches']}；四星worker启动 {parallel['fourStarComponent']['workerLaunches']}。
- 随机：8线程单元 {parallel['randomness']['decisions']} 次五星决策，{parallel['randomness']['eligibleMultiCandidate']} 次多候选，实际随机改选 {parallel['randomness']['randomSelections']} 次，资格验证失败 {parallel['randomness']['eligibilityFailures']} 次。
""")
    markdown = f"""# v5.2.1 黑棋8线程五星 vs 冻结四星测试报告

候选模型执白使用 `5.4.1-transactional-deadline-root-parallel-5s`，执黑使用 `5.2.2-certificate-dependency-root-parallel-8w-5s`；线程对照的黑棋使用相同 v5.2.1 策略与截止时间，但只允许单线程。冻结四星保持原始单线程。无禁手和有禁手各运行100局单线程对照与100局8线程候选，每个单元50黑/50白，共400局；所有对局单进程串行运行，每步上限5秒。

统计严格按同色比较：新五星执黑只与交换颜色后的冻结四星执黑比较；线程收益只比较同一开局、同一颜色下的8线程与单线程结果。

""" + "\n".join(sections) + f"""
## 总结

- 综合分类：`{decision['classification']}`。
- 无禁手：`{gates['free']['classification']}`；有禁手：`{gates['forbidden']['classification']}`。两种规则独立判定，不用合并结果掩盖单项失败。
- 独立复放通过：{replay['games']}局、{replay['moves']}手；并行黑棋实际启动worker的决策 {replay['blackParallelSteps']} 次，累计worker启动 {replay['blackWorkerLaunches']}。
- 产品动作：`{decision['productionAction']}`。本轮候选仅用于benchmark，未改变UI中的生产五星v5.1。
- 随机用户模式允许独立运行选择不同的等价合法位置；复放验证记录路径的规则合法性、组件路由、种子、随机资格、证书元数据和终局，不要求另一次运行逐手相同。
"""
    (args.output / "parallel_v521_vs_four_star_report.md").write_text(
        markdown, encoding="utf-8")
    checksum = args.output / "checksums.sha256"
    files = sorted(path for path in args.output.iterdir()
                   if path.is_file() and path != checksum)
    checksum.write_text("".join(f"{sha256(path)}  {path.name}\n" for path in files),
                        encoding="utf-8")


if __name__ == "__main__":
    main()
