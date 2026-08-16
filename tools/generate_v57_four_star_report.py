#!/usr/bin/env python3
"""Generate the five-star versus frozen-control report."""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
import statistics
from pathlib import Path


VERSION = "5.7.0-white-v541-black-v521-independent-root-parallel8-5s"
BRANCH_FIRST_VERSION = "5.7.2-branch-first-preview-recursive-pool-14d-5s"
V541_SCHEDULER_VERSION = "5.4.2-v541-persistent-pool-token-blocks-8w-5s"
OPPONENTS = {
    "four-star": {
        "engine": "four-star",
        "label": "稳定四星",
        "profile": "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book",
        "slug": "four_star",
    },
    "legacy": {
        "engine": "legacy",
        "label": "旧三星",
        "profile": "legacy-three-star@5224020",
        "slug": "legacy_three_star",
    },
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()]
    return (next(row for row in rows if row.get("type") == "header"),
            [row for row in rows if row.get("type") == "game"])


def wilson(wins: int, draws: int, losses: int) -> list[float]:
    n = wins + draws + losses
    score = (wins + 0.5 * draws) / n
    z = 1.95996398454
    den = 1.0 + z * z / n
    center = (score + z * z / (2.0 * n)) / den
    half = z * math.sqrt(score * (1.0 - score) / n + z * z /
                         (4.0 * n * n)) / den
    return [center - half, center + half]


def point(game: dict, stone_color: int, candidate: bool) -> float:
    model_color = game["newColor"] if candidate else -game["newColor"]
    if game["winner"] == 0:
        return 0.5
    return float(game["winner"] == model_color == stone_color)


def stats(games: list[dict], stone_color: int, candidate: bool) -> dict:
    selected = [game for game in games
                if (game["newColor"] if candidate else -game["newColor"])
                == stone_color]
    wins = sum(game["winner"] ==
               (game["newColor"] if candidate else -game["newColor"])
               for game in selected)
    draws = sum(game["winner"] == 0 for game in selected)
    losses = len(selected) - wins - draws
    return {"games": len(selected), "wins": wins, "draws": draws,
            "losses": losses,
            "scoreRate": (wins + 0.5 * draws) / len(selected),
            "wilson95": wilson(wins, draws, losses)}


def overall(games: list[dict], candidate: bool) -> dict:
    wins = sum(game["winner"] ==
               (game["newColor"] if candidate else -game["newColor"])
               for game in games)
    draws = sum(game["winner"] == 0 for game in games)
    losses = len(games) - wins - draws
    return {"games": len(games), "wins": wins, "draws": draws,
            "losses": losses,
            "scoreRate": (wins + 0.5 * draws) / len(games),
            "wilson95": wilson(wins, draws, losses)}


def paired(games: list[dict], stone_color: int | None) -> dict:
    by_key = {(int(game["openingId"]), int(game["newColor"])): game
              for game in games}
    values = []
    for opening in range(50):
        if stone_color is None:
            values.append(statistics.mean(
                point(by_key[(opening, color)], color, True) -
                point(by_key[(opening, -color)], color, False)
                for color in (1, -1)))
        else:
            values.append(
                point(by_key[(opening, stone_color)], stone_color, True) -
                point(by_key[(opening, -stone_color)], stone_color, False))
    center = statistics.mean(values)
    half = (0.0 if len(values) < 2 else
            1.95996398454 * statistics.stdev(values) /
            math.sqrt(len(values)))
    return {"openingPairs": len(values), "delta": center,
            "paired95": [max(-1.0, center - half), min(1.0, center + half)]}


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1,
                       math.ceil(fraction * len(ordered)) - 1)]


def component_summary(steps: list[dict]) -> dict:
    if not steps:
        return {"decisions": 0}
    diagnostics = [step["diagnostics"] for step in steps]
    status_counts = collections.Counter(str(step.get("decisionStatus", 3))
                                        for step in steps)
    return {
        "decisions": len(steps),
        "latencyMs": {"p50": statistics.median(float(s["ms"]) for s in steps),
                      "p95": percentile([float(s["ms"]) for s in steps], 0.95),
                      "max": max(float(s["ms"]) for s in steps)},
        "cpuMs": {"p50": statistics.median(float(s.get("cpuMs", 0.0)) for s in steps),
                  "p95": percentile([float(s.get("cpuMs", 0.0)) for s in steps], 0.95),
                  "max": max(float(s.get("cpuMs", 0.0)) for s in steps),
                  "total": sum(float(s.get("cpuMs", 0.0)) for s in steps)},
        "budgetExhaustions": sum(bool(s["budgetExhausted"]) for s in steps),
        "workerLaunches": sum(int(s.get("proofWorkersLaunched", 0)) for s in steps),
        "parallelJobs": sum(int(s.get("proofParallelJobs", 0)) for s in steps),
        "parallelJobsCompleted": sum(int(s.get("proofParallelJobsCompleted", 0)) for s in steps),
        "parallelDecisions": sum(int(s.get("proofWorkersLaunched", 0)) > 0 for s in steps),
        "poolDispatches": sum(int(d.get("parallelPoolDispatches", 0)) for d in diagnostics),
        "poolWorkersReused": sum(int(d.get("parallelPoolWorkersReused", 0)) for d in diagnostics),
        "poolFallbacks": sum(int(d.get("parallelPoolFallbacks", 0)) for d in diagnostics),
        "tokenBlockClaims": sum(int(d.get("parallelTokenBlockClaims", 0)) for d in diagnostics),
        "tokenBlockTokens": sum(int(d.get("parallelTokenBlockTokens", 0)) for d in diagnostics),
        "tokenBlockReturns": sum(int(d.get("parallelTokenBlockReturns", 0)) for d in diagnostics),
        "branchFirstPreviewBranches": sum(int(d.get("branchFirstPreviewBranches", 0)) for d in diagnostics),
        "branchFirstAdvancedFourPreviews": sum(int(d.get("branchFirstAdvancedFourPreviews", 0)) for d in diagnostics),
        "branchFirstAdvancedThreePreviews": sum(int(d.get("branchFirstAdvancedThreePreviews", 0)) for d in diagnostics),
        "branchFirstPreviewIncomplete": sum(int(d.get("branchFirstPreviewIncomplete", 0)) for d in diagnostics),
        "branchFirstWaves": sum(int(d.get("branchFirstWaves", 0)) for d in diagnostics),
        "branchFirstWorkersLaunched": sum(int(d.get("branchFirstWorkersLaunched", 0)) for d in diagnostics),
        "branchFirstJobs": sum(int(d.get("branchFirstJobs", 0)) for d in diagnostics),
        "branchFirstJobsCompleted": sum(int(d.get("branchFirstJobsCompleted", 0)) for d in diagnostics),
        "branchFirstVerifiedJobs": sum(int(d.get("branchFirstVerifiedJobs", 0)) for d in diagnostics),
        "branchFirstUsefulJobs": sum(int(d.get("branchFirstUsefulJobs", 0)) for d in diagnostics),
        "branchFirstMergeFailures": sum(int(d.get("branchFirstMergeFailures", 0)) for d in diagnostics),
        "branchFirstUnknownJobs": sum(int(d.get("branchFirstUnknownJobs", 0)) for d in diagnostics),
        "branchFirstMaxConcurrentWorkers": max(int(d.get("branchFirstMaxConcurrentWorkers", 0)) for d in diagnostics),
        "branchFirstDispatchFallbacks": sum(int(d.get("branchFirstDispatchFallbacks", 0)) for d in diagnostics),
        "branchFirstSerialFallbacks": sum(int(d.get("branchFirstSerialFallbacks", 0)) for d in diagnostics),
        "branchFirstDepthExtensions": sum(int(d.get("branchFirstDepthExtensions", 0)) for d in diagnostics),
        "branchFirstAdvancedFourDepthExtensions": sum(int(d.get("branchFirstAdvancedFourDepthExtensions", 0)) for d in diagnostics),
        "branchFirstAdvancedThreeDepthExtensions": sum(int(d.get("branchFirstAdvancedThreeDepthExtensions", 0)) for d in diagnostics),
        "branchFirstMaxChildDepth": max(int(d.get("branchFirstMaxChildDepth", 0)) for d in diagnostics),
        "branchFirstDeadlineStops": sum(int(d.get("branchFirstDeadlineStops", 0)) for d in diagnostics),
        "arenaAllocations": sum(int(d.get("allocations", 0)) for d in diagnostics),
        "arenaClearedBytes": sum(int(d.get("clearedBytes", 0)) for d in diagnostics),
        "proofSessionQueries": sum(int(d.get("proofSessionQueries", 0)) for d in diagnostics),
        "rootJobs": sum(int(d.get("parallelRootJobs", 0)) for d in diagnostics),
        "rootJobsCompleted": sum(int(d.get("parallelRootJobsCompleted", 0)) for d in diagnostics),
        "independentDispatches": sum(int(d.get("parallelIndependentDispatches", 0)) for d in diagnostics),
        "singleWorkerFallbacks": sum(int(d.get("parallelMultiRootSingleWorkerFallbacks", 0)) for d in diagnostics),
        "maxConcurrentWorkers": max(int(d.get("parallelMaxConcurrentWorkers", 0)) for d in diagnostics),
        "escapeBatches": sum(int(d.get("parallelEscapeBatches", 0)) for d in diagnostics),
        "escapeWorkers": sum(int(d.get("parallelEscapeWorkersLaunched", 0)) for d in diagnostics),
        "escapeJobs": sum(int(d.get("parallelEscapeJobs", 0)) for d in diagnostics),
        "escapeJobsCompleted": sum(int(d.get("parallelEscapeJobsCompleted", 0)) for d in diagnostics),
        "escapeMaxConcurrentWorkers": max(int(d.get("parallelEscapeMaxConcurrentWorkers", 0)) for d in diagnostics),
        "escapeBudgetExhausted": sum(int(d.get("parallelEscapeBudgetExhausted", 0)) for d in diagnostics),
        "cachedLegalityChecks": sum(int(d.get("forbiddenLegalityCachedChecks", 0)) for d in diagnostics),
        "cacheMismatches": sum(int(d.get("forbiddenLegalityCacheMismatches", 0)) for d in diagnostics),
        "randomSelections": sum(bool(s["randomSelectionUsed"]) for s in steps),
        "eligibleMultiCandidate": sum(int(s["randomCandidateCount"]) > 1 for s in steps),
        "certificates": sum(bool(s["proofCertificateVerified"]) for s in steps),
        "decisionStatusCounts": dict(sorted(status_counts.items())),
        "decisionCount": sum(int(d.get("decisionCount", 0)) for d in diagnostics),
        "decisionUnknowns": sum(int(d.get("decisionUnknowns", 0)) for d in diagnostics),
        "decisionFallbacks": sum(int(d.get("decisionFallbacks", 0)) for d in diagnostics),
        "decisionNoLegalMoves": sum(int(d.get("decisionNoLegalMoves", 0)) for d in diagnostics),
        "decisionVerifiedWins": sum(int(d.get("decisionVerifiedWins", 0)) for d in diagnostics),
        "decisionVerifiedLosses": sum(int(d.get("decisionVerifiedLosses", 0)) for d in diagnostics),
        "decisionLedgerExhaustions": sum(int(d.get("decisionLedgerExhaustions", 0)) for d in diagnostics),
        "stageRequests": sum(int(d.get("decisionStageRequests", 0)) for d in diagnostics),
        "stageReservations": sum(int(d.get("decisionStageReservations", 0)) for d in diagnostics),
        "stageAbandons": sum(int(d.get("decisionStageAbandons", 0)) for d in diagnostics),
        "duplicateRootEnumerations": sum(int(d.get("parallelDuplicateRootEnumerations", 0)) for d in diagnostics),
        "earlyStops": sum(int(d.get("parallelEarlyStops", 0)) for d in diagnostics),
        "postponedSiblings": sum(int(d.get("dfpnPostponedSiblings", 0)) for d in diagnostics),
        "dovetailRequeues": sum(int(d.get("dfpnDovetailRequeues", 0)) for d in diagnostics),
        "legalityValidationSamples": sum(int(d.get("forbiddenLegalityCacheValidationSamples", 0)) for d in diagnostics),
        "cacheMismatchCoordinates": sorted({
            (int(d.get("forbiddenLegalityCacheMismatchX", 0)),
             int(d.get("forbiddenLegalityCacheMismatchY", 0)),
             int(d.get("forbiddenLegalityCacheMismatchSide", 0)))
            for d in diagnostics
            if int(d.get("forbiddenLegalityCacheMismatches", 0)) > 0
        }),
    }


def summarize(path: Path, rule: str, forbidden: bool, opponent: dict,
              expected_version: str) -> dict:
    header, games = load(path)
    assert len(games) == 100 and header["forbiddenBlack"] is forbidden
    assert header["newProfile"]["version"] == expected_version
    branch_first = expected_version == BRANCH_FIRST_VERSION
    assert header["newProfile"].get("branchFirstSearchEnabled", False) is branch_first
    assert header["opponentProfile"] == opponent["profile"]
    new_steps = [step for game in games for step in game["steps"]
                 if step["engine"] == "new"]
    control_steps = [step for game in games for step in game["steps"]
                     if step["engine"] == opponent["engine"]]
    cells = {
        "rule": rule, "forbiddenBlack": forbidden, "source": str(path),
        "sourceSha256": sha256(path), "header": header, "games": len(games),
        "newWhite": stats(games, -1, True),
        "controlWhite": stats(games, -1, False),
        "newBlack": stats(games, 1, True),
        "controlBlack": stats(games, 1, False),
        "newOverall": overall(games, True),
        "controlOverall": overall(games, False),
        "terminations": dict(collections.Counter(game["termination"] for game in games)),
        "newComponent": component_summary(new_steps),
        "controlComponent": component_summary(control_steps),
        "pairedWhite": paired(games, -1),
        "pairedBlack": paired(games, 1),
        "pairedOverall": paired(games, None),
        "anomalies": sum(game["anomaly"] is not None for game in games),
    }
    for prefix in ("White", "Black", "Overall"):
        cells[f"{prefix.lower()}Delta"] = (
            cells[f"new{prefix}"]["scoreRate"] -
            cells[f"control{prefix}"]["scoreRate"])
    return cells


def candidate_label(version: str) -> str:
    if version == V541_SCHEDULER_VERSION:
        return "5.4.2 scheduler"
    if version == BRANCH_FIRST_VERSION:
        return "5.7 branch-first"
    return "5.7"


def candidate_description(version: str) -> str:
    if version == V541_SCHEDULER_VERSION:
        return ("候选直接继承 5.4.1 证明引擎的战术、深度和节点预算，仅启用持久 worker "
                "pool、私有 worker session 和 64-token block；decision ledger、合法性缓存和恢复层保持关闭，"
                "并保留 4.2 秒内部 reserve 给 pool 收尾，用于隔离调度开销。")
    if version == BRANCH_FIRST_VERSION:
        return ("候选 profile 对两种颜色使用浅层 tactical preview、根节点串行排序和递归 branch wave；"
                "基础证明深度 14，adv-four/adv-three 分别允许 +2/+1，战术上限 16。")
    return "执白使用 5.4.1 证明引擎，执黑使用修复后的 v5.2.1 loss-aware 路径、独立 root job 并行。"


def line(label: str, value: dict) -> str:
    low, high = value["wilson95"]
    return (f"- {label}：{value['wins']}/{value['draws']}/{value['losses']}，"
            f"得分率 {value['scoreRate']:.1%}，Wilson 95% {low:.1%}–{high:.1%}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--free", type=Path, required=True)
    parser.add_argument("--forbidden", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--opponent", choices=tuple(OPPONENTS),
                        default="four-star")
    parser.add_argument("--profile-version", default=VERSION,
                        help="expected candidate profile version")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    opponent = OPPONENTS[args.opponent]
    branch_first = args.profile_version == BRANCH_FIRST_VERSION
    label = candidate_label(args.profile_version)
    prefix = "v541" if args.profile_version == V541_SCHEDULER_VERSION else "v57"
    cells = {
        "free": summarize(args.free, "无禁手", False, opponent,
                           args.profile_version),
        "forbidden": summarize(args.forbidden, "有禁手", True, opponent,
                                args.profile_version),
    }
    replay = json.loads(args.replay.read_text(encoding="utf-8"))
    assert replay["status"] == "pass" and replay["games"] == 200
    correctness = all(cell["anomalies"] == 0 and
                      cell["newComponent"]["latencyMs"]["max"] <= 5000.0
                      for cell in cells.values())
    white_gate = all(cell["newWhite"]["scoreRate"] >= 0.5
                     for cell in cells.values())
    black_gate = all(cell["blackDelta"] >= 0.0 for cell in cells.values())
    overall_gate = all(cell["overallDelta"] >= 0.0 for cell in cells.values())
    point_pass = correctness and white_gate and black_gate and overall_gate
    demonstrated = point_pass and all(
        cell["pairedWhite"]["paired95"][0] > 0 and
        cell["pairedOverall"]["paired95"][0] > 0 for cell in cells.values())
    decision = {
        "correctnessLatencyGate": correctness,
        "whitePointGate": white_gate,
        "blackSameColorPointGate": black_gate,
        "overallPointGate": overall_gate,
        "classification": "demonstrated" if demonstrated else
            "point-estimate-pass-statistically-inconclusive"
            if point_pass else "not-demonstrated",
        "productionAction": "keep-existing-four-star-and-production-routing",
    }
    summary = {"schemaVersion": 1, "candidate": args.profile_version,
               "cells": cells,
               "replay": replay, "releaseDecision": decision}
    report_slug = opponent["slug"]
    (args.output / f"{prefix}_vs_{report_slug}_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    sections = []
    for key in ("free", "forbidden"):
        cell = cells[key]
        activity = cell["newComponent"]
        sections.append(f"""## {cell['rule']} vs {opponent['label']}

{line(label + '执白', cell['newWhite'])}
{line(opponent['label'] + '执白', cell['controlWhite'])}
- 白棋同色变化：{cell['whiteDelta']:+.1%}；配对 95% 区间 {cell['pairedWhite']['paired95'][0]:+.1%}–{cell['pairedWhite']['paired95'][1]:+.1%}
{line(label + '执黑', cell['newBlack'])}
{line(opponent['label'] + '执黑', cell['controlBlack'])}
- 黑棋同色变化：{cell['blackDelta']:+.1%}；配对 95% 区间 {cell['pairedBlack']['paired95'][0]:+.1%}–{cell['pairedBlack']['paired95'][1]:+.1%}
{line(label + '总体', cell['newOverall'])}
{line(opponent['label'] + '总体', cell['controlOverall'])}
- 总体变化：{cell['overallDelta']:+.1%}；配对 95% 区间 {cell['pairedOverall']['paired95'][0]:+.1%}–{cell['pairedOverall']['paired95'][1]:+.1%}
- 搜索活动：{activity['decisions']} 次；p50/p95/max {activity['latencyMs']['p50']:.1f}/{activity['latencyMs']['p95']:.1f}/{activity['latencyMs']['max']:.1f} ms；root jobs {activity['rootJobs']}（完成 {activity['rootJobsCompleted']}），独立并行 dispatch {activity['independentDispatches']}，最大并发 worker {activity['maxConcurrentWorkers']}，单 worker fallback {activity['singleWorkerFallbacks']}。
- 调度器：pool dispatch {activity['poolDispatches']}，复用 worker {activity['poolWorkersReused']}，pool fallback {activity['poolFallbacks']}；token block claims/tokens/returns {activity['tokenBlockClaims']}/{activity['tokenBlockTokens']}/{activity['tokenBlockReturns']}；proof session queries {activity['proofSessionQueries']}；arena allocations/cleared bytes {activity['arenaAllocations']}/{activity['arenaClearedBytes']}。
- Branch-first：preview {activity['branchFirstPreviewBranches']}（adv-four {activity['branchFirstAdvancedFourPreviews']}，adv-three {activity['branchFirstAdvancedThreePreviews']}），waves/jobs/completed/useful {activity['branchFirstWaves']}/{activity['branchFirstJobs']}/{activity['branchFirstJobsCompleted']}/{activity['branchFirstUsefulJobs']}，max concurrent {activity['branchFirstMaxConcurrentWorkers']}，depth extensions {activity['branchFirstDepthExtensions']}（adv-four +2: {activity['branchFirstAdvancedFourDepthExtensions']}，adv-three +1: {activity['branchFirstAdvancedThreeDepthExtensions']}），max child depth {activity['branchFirstMaxChildDepth']}，fallback/deadline {activity['branchFirstSerialFallbacks']}/{activity['branchFirstDeadlineStops']}。
- 逃生并行：{activity['escapeBatches']} 批、{activity['escapeJobs']} jobs（完成 {activity['escapeJobsCompleted']}），最大并发 {activity['escapeMaxConcurrentWorkers']}，预算耗尽 {activity['escapeBudgetExhausted']} 次。
- 决策账本：状态 {activity['decisionStatusCounts']}；unknown {activity['decisionUnknowns']}，fallback {activity['decisionFallbacks']}，无合法着 {activity['decisionNoLegalMoves']}，verified win/loss {activity['decisionVerifiedWins']}/{activity['decisionVerifiedLosses']}；ledger exhaustion {activity['decisionLedgerExhaustions']}；stage requests/reserved/abandoned {activity['stageRequests']}/{activity['stageReservations']}/{activity['stageAbandons']}。
- 并行/DFPN 安全：duplicate root enumeration {activity['duplicateRootEnumerations']}，early-stop {activity['earlyStops']}，postponed siblings {activity['postponedSiblings']}，dovetail requeue {activity['dovetailRequeues']}。
- 禁手缓存：{activity['cachedLegalityChecks']} 次命中检查，validation samples {activity['legalityValidationSamples']}，oracle mismatch {activity['cacheMismatches']} 次，坐标 {activity['cacheMismatchCoordinates']}；随机改选 {activity['randomSelections']}/{activity['eligibleMultiCandidate']}。
- 终局：{cell['terminations']}。
""")
    report = f"""# {label} vs {opponent['label']}测试报告

候选版本：`{args.profile_version}`。{candidate_description(args.profile_version)} 保留玩家可见 5 秒硬上限；{opponent['label']}保持冻结控制实现与原有单线程语义。

无禁手、有禁手分别 100 局（50 个固定开局前缀各执黑/执白一局），保留自然先手优势，不做颜色再加权。这里的有禁手赛程是 `natural-prefix/no-swap` 规则诊断，不包含 RIF exchange/Swap2/Taraguchi，不能据此宣称正式平衡开局的理论结论。

""" + "\n".join(sections) + f"""
## 结论

- 正确性/5 秒门：{'通过' if correctness else '未通过'}；白棋点估计门：{'通过' if white_gate else '未通过'}；黑棋同色点估计门：{'通过' if black_gate else '未通过'}；总体点估计门：{'通过' if overall_gate else '未通过'}。
- 分类：`{decision['classification']}`。100 局配对区间跨 0 时，只能视为点估计改善，不能称为统计显著提升。
- 生产/UI：保持现有三星、稳定四星和生产路由不变；该 profile 仅为研究候选。

## 完整性

- 独立回放：{replay['games']} 局、{replay['moves']} 手，状态 `{replay['status']}`。
- 原始记录：`{args.free}`、`{args.forbidden}`；回放摘要：`{args.replay}`。
- 原始记录 SHA-256：无禁手 `{sha256(args.free)}`；有禁手 `{sha256(args.forbidden)}`。
"""
    (args.output / f"{prefix}_vs_{report_slug}_report.md").write_text(report, encoding="utf-8")
    files = sorted(path for path in args.output.iterdir() if path.is_file())
    (args.output / "checksums.sha256").write_text(
        "".join(f"{sha256(path)}  {path.name}\n" for path in files
                if path.name != "checksums.sha256"), encoding="utf-8")


if __name__ == "__main__":
    main()
