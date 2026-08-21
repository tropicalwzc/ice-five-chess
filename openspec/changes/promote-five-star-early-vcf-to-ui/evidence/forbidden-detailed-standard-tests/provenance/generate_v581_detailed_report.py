#!/usr/bin/env python3
"""Generate the detailed 5.8.1 three/four/5.4.1 standard-test report."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import random
from collections import Counter
from datetime import datetime
from pathlib import Path
from zoneinfo import ZoneInfo


STATUS_NAMES = {
    0: "unknown",
    1: "proven-win",
    2: "no-forced-win-in-scope",
}


def read_jsonl(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text().splitlines() if line]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1
    header = headers[0]
    assert len(games) == header["games"] == 100
    assert header["openingCount"] == 50
    assert Counter(game["newColor"] for game in games) == {1: 50, -1: 50}
    return header, games


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def percentile(values: list[float], proportion: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    index = max(0, math.ceil(len(ordered) * proportion) - 1)
    return ordered[index]


def distribution(values: list[float]) -> dict:
    if not values:
        return {"count": 0, "p50": None, "p95": None, "max": None,
                "total": 0.0}
    return {
        "count": len(values),
        "p50": percentile(values, 0.50),
        "p95": percentile(values, 0.95),
        "max": max(values),
        "total": sum(values),
    }


def wilson(score: float, games: int) -> list[float]:
    if games == 0:
        return [0.0, 0.0]
    z = 1.959963984540054
    p = score / games
    denominator = 1.0 + z * z / games
    center = (p + z * z / (2.0 * games)) / denominator
    half = z * math.sqrt(
        p * (1.0 - p) / games + z * z / (4.0 * games * games)
    ) / denominator
    return [center - half, center + half]


def result_block(games: list[dict], color: int | None = None) -> dict:
    selected = games if color is None else [
        game for game in games if game["newColor"] == color]
    wins = sum(game["winner"] == game["newColor"] for game in selected)
    draws = sum(game["winner"] == 0 for game in selected)
    losses = sum(game["winner"] == -game["newColor"] for game in selected)
    score = wins + 0.5 * draws
    return {
        "games": len(selected), "wins": wins, "draws": draws,
        "losses": losses, "scoreRate": score / len(selected),
        "wilson95": wilson(score, len(selected)),
    }


def paired_block(games: list[dict], seed: int) -> tuple[dict, list[dict]]:
    rows = []
    for opening_id in sorted({game["openingId"] for game in games}):
        pair = [game for game in games if game["openingId"] == opening_id]
        assert len(pair) == 2
        by_color = {game["newColor"]: game for game in pair}

        def points(game: dict) -> float:
            if game["winner"] == game["newColor"]:
                return 1.0
            if game["winner"] == 0:
                return 0.5
            return 0.0

        black_points = points(by_color[1])
        white_points = points(by_color[-1])
        total = black_points + white_points
        rows.append({
            "openingId": opening_id,
            "candidateBlackPoints": black_points,
            "candidateWhitePoints": white_points,
            "candidatePairPoints": total,
            "candidateBlackWinner": by_color[1]["winner"],
            "candidateWhiteWinner": by_color[-1]["winner"],
        })

    pair_scores = [row["candidatePairPoints"] / 2.0 for row in rows]
    rng = random.Random(seed)
    bootstrap = []
    for _ in range(20_000):
        sample = [pair_scores[rng.randrange(len(pair_scores))]
                  for _ in pair_scores]
        bootstrap.append(sum(sample) / len(sample))
    involving_draw = sum(
        row["candidateBlackPoints"] == 0.5 or
        row["candidateWhitePoints"] == 0.5 for row in rows)
    result = {
        "openings": len(rows),
        "candidateSweeps": sum(row["candidatePairPoints"] == 2.0
                               for row in rows),
        "opponentSweeps": sum(row["candidatePairPoints"] == 0.0
                              for row in rows),
        "splitDecisive": sum(
            sorted((row["candidateBlackPoints"],
                    row["candidateWhitePoints"])) == [0.0, 1.0]
            for row in rows),
        "involvingDraw": involving_draw,
        "candidatePairedScoreRate": sum(pair_scores) / len(pair_scores),
        "pairedBootstrap95": [percentile(bootstrap, 0.025),
                              percentile(bootstrap, 0.975)],
    }
    return result, rows


def format_counter(counter: Counter) -> dict[str, int]:
    return {str(key): counter[key] for key in sorted(counter, key=str)}


def analyze_cell(label: str, path: Path, replay_path: Path,
                 bootstrap_seed: int) -> tuple[dict, list[dict]]:
    header, games = read_jsonl(path)
    replay = json.loads(replay_path.read_text())
    assert replay["status"] == "pass" and replay["gamesReplayed"] == 100
    steps = [dict(step, openingId=game["openingId"],
                  newColor=game["newColor"], ply=index +
                  len(game["moves"]) - len(game["steps"]))
             for game in games for index, step in enumerate(game["steps"])]
    candidate = [step for step in steps if step["engine"] == "new"]
    opponent = [step for step in steps if step["engine"] != "new"]
    eligible = [step for step in candidate if step["earlyVCF"]["eligible"]]
    guard_eligible = [step for step in candidate
                      if step["opponentGuard"]["eligible"]]
    vct_eligible = [step for step in guard_eligible
                    if step["opponentGuard"]["vctEligible"]]
    black = result_block(games, 1)
    white = result_block(games, -1)
    overall = result_block(games)
    paired, paired_rows = paired_block(games, bootstrap_seed)

    opponent_black_rate = white["losses"] / white["games"]
    opponent_white_rate = black["losses"] / black["games"]
    hard_violations = [{
        key: step[key] for key in
        ("openingId", "newColor", "ply", "engine", "side", "x", "y", "ms")
    } for step in steps if step["ms"] > 5000.0]

    def max_step(selected: list[dict]) -> dict:
        step = max(selected, key=lambda item: item["ms"])
        return {key: step[key] for key in
                ("openingId", "newColor", "ply", "engine", "side", "x", "y", "ms")}

    early_statuses = Counter(
        STATUS_NAMES.get(step["earlyVCF"]["status"],
                         str(step["earlyVCF"]["status"]))
        for step in eligible)
    summary = {
        "label": label,
        "rawPath": f"raw/{path.name}",
        "replayPath": f"replay/{replay_path.name}",
        "rawSha256": sha256(path),
        "replaySha256": sha256(replay_path),
        "header": header,
        "results": {"black": black, "white": white, "overall": overall},
        "sameColor": {
            "candidateBlackScoreRate": black["scoreRate"],
            "opponentBlackScoreRate": opponent_black_rate,
            "blackDelta": black["scoreRate"] - opponent_black_rate,
            "candidateWhiteScoreRate": white["scoreRate"],
            "opponentWhiteScoreRate": opponent_white_rate,
            "whiteDelta": white["scoreRate"] - opponent_white_rate,
        },
        "paired": paired,
        "games": {
            "moves": distribution([game["moveCount"] for game in games]),
            "terminations": format_counter(Counter(
                game["termination"] for game in games)),
            "anomalies": [
                {key: game[key] for key in ("openingId", "newColor", "anomaly")}
                for game in games if game["anomaly"] is not None],
        },
        "decisions": {
            "candidate": len(candidate), "opponent": len(opponent),
            "candidateByColor": {
                "black": sum(step["newColor"] == 1 for step in candidate),
                "white": sum(step["newColor"] == -1 for step in candidate),
            },
            "candidateWallMs": distribution([step["ms"] for step in candidate]),
            "opponentWallMs": distribution([step["ms"] for step in opponent]),
            "candidateCpuMs": distribution([step["cpuMs"] for step in candidate]),
            "opponentCpuMs": distribution([step["cpuMs"] for step in opponent]),
            "candidateNodes": distribution([step["nodes"] for step in candidate]),
            "opponentNodes": distribution([step["nodes"] for step in opponent]),
            "candidatePeakResidentBytes": max(
                step["peakResidentBytes"] for step in candidate),
            "opponentPeakResidentBytes": max(
                step["peakResidentBytes"] for step in opponent),
            "candidateBudgetExhausted": sum(
                step["budgetExhausted"] for step in candidate),
            "opponentBudgetExhausted": sum(
                step["budgetExhausted"] for step in opponent),
            "candidateFallbacks": sum(step["fallbackUsed"] for step in candidate),
            "opponentFallbacks": sum(step["fallbackUsed"] for step in opponent),
            "candidateDecisionStatuses": format_counter(Counter(
                step["decisionStatus"] for step in candidate)),
            "opponentDecisionStatuses": format_counter(Counter(
                step["decisionStatus"] for step in opponent)),
            "hardLimitViolations": hard_violations,
            "candidateMax": max_step(candidate),
            "opponentMax": max_step(opponent),
        },
        "sentinel": {
            "eligible": len(eligible),
            "skipped": len(candidate) - len(eligible),
            "skipReasons": format_counter(Counter(
                step["earlyVCF"]["skipReason"] for step in candidate
                if not step["earlyVCF"]["eligible"])),
            "effectiveDepths": format_counter(Counter(
                step["earlyVCF"]["effectiveDepth"] for step in eligible)),
            "statuses": format_counter(early_statuses),
            "adaptiveEscalations": sum(
                step["earlyVCF"]["adaptiveEscalated"] for step in eligible),
            "auditedAlternatives": sum(
                step["earlyVCF"]["auditedCount"] for step in eligible),
            "verifiedLossAlternatives": sum(
                step["earlyVCF"]["verifiedLosses"] for step in eligible),
            "verifiedLossesAvoided": sum(
                step["earlyVCF"]["avoidedVerifiedLoss"] for step in eligible),
            "moveChanges": sum(
                step["earlyVCF"]["provisionalX"] != step["earlyVCF"]["selectedX"] or
                step["earlyVCF"]["provisionalY"] != step["earlyVCF"]["selectedY"]
                for step in eligible),
            "latencyMs": distribution([
                step["earlyVCF"]["ms"] for step in eligible]),
            "nodes": distribution([
                step["earlyVCF"]["nodes"] for step in eligible]),
            "cacheReuseDecisions": sum(
                step["earlyVCF"]["cacheReused"] for step in eligible),
            "cacheHits": sum(step["earlyVCF"]["cacheHits"] for step in eligible),
            "rollbacks": sum(step["earlyVCF"]["rollback"] for step in candidate),
            "evidenceMismatches": sum(
                step["earlyVCF"]["evidenceMismatch"] for step in candidate),
            "finalGuardOnlyCatches": sum(
                step["earlyVCF"]["finalGuardOnlyLoss"] for step in candidate),
        },
        "finalGuard": {
            "eligible": len(guard_eligible),
            "auditedCandidates": sum(
                step["opponentGuard"]["auditedCount"]
                for step in guard_eligible),
            "verifiedLossAlternatives": sum(
                step["opponentGuard"]["verifiedLosses"]
                for step in guard_eligible),
            "verifiedLossesAvoided": sum(
                step["opponentGuard"]["avoidedVerifiedLoss"]
                for step in guard_eligible),
            "vcfLatencyMs": distribution([
                step["opponentGuard"]["vcfMs"] for step in guard_eligible]),
            "vctEligible": len(vct_eligible),
            "vctLatencyMs": distribution([
                step["opponentGuard"]["vctMs"] for step in vct_eligible]),
            "rollbacks": sum(
                step["opponentGuard"]["rollback"] for step in candidate),
        },
        "replay": replay,
    }
    for row in paired_rows:
        row["opponent"] = label
    return summary, paired_rows


def pct(value: float) -> str:
    return f"{value * 100:.1f}%"


def interval(values: list[float]) -> str:
    return f"{pct(values[0])}–{pct(values[1])}"


def wdl(block: dict) -> str:
    return f"{block['wins']}/{block['draws']}/{block['losses']}"


def ms(stats: dict) -> str:
    return f"{stats['p50']:.3f}/{stats['p95']:.3f}/{stats['max']:.3f}"


def nodes(stats: dict) -> str:
    return f"{stats['p50']:.0f}/{stats['p95']:.0f}/{stats['max']:.0f}"


def render_markdown(cells: list[dict], aggregate: dict) -> str:
    forbidden_black = cells[0]["header"]["forbiddenBlack"]
    assert all(cell["header"]["forbiddenBlack"] == forbidden_black
               for cell in cells)
    rule_label = "黑方禁手开启" if forbidden_black else "Freestyle 无禁手"
    seed_domain = cells[0]["header"]["seedDomain"]
    lines = [
        f"# 五星 5.8.1 对三星、四星和 exact 5.4.1（{rule_label}）详细标准测试报告",
        "",
        "## 测试结论",
        "",
        "本报告比较 UI 已推广的五星模型 "
        "`5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` 与冻结三星、四星和 "
        f"exact 5.4.1。规则为{rule_label}；每个对手使用同一批 50 个自然开局并交换颜色，共 100 局；"
        "5.8.1 执黑 50 局、执白 50 局。三组共 300 局。",
        "",
        "| 对手 | 5.8.1 执黑 W/D/L | 5.8.1 执白 W/D/L | 总体 W/D/L | 得分率 | Wilson 95% | 配对 bootstrap 95% |",
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    for cell in cells:
        result = cell["results"]
        lines.append(
            f"| {cell['label']} | {wdl(result['black'])} | "
            f"{wdl(result['white'])} | {wdl(result['overall'])} | "
            f"{pct(result['overall']['scoreRate'])} | "
            f"{interval(result['overall']['wilson95'])} | "
            f"{interval(cell['paired']['pairedBootstrap95'])} |")
    point_estimates = "、".join(
        f"对{cell['label']} {pct(cell['results']['overall']['scoreRate'])}"
        for cell in cells)
    all_candidate_violations = [
        item for cell in cells
        for item in cell["decisions"]["hardLimitViolations"]
        if item["engine"] == "new"]
    all_opponent_violations = [
        item for cell in cells
        for item in cell["decisions"]["hardLimitViolations"]
        if item["engine"] != "new"]
    candidate_violation_locations = {
        (item["openingId"], item["newColor"], item["ply"],
         item["side"], item["x"], item["y"])
        for item in all_candidate_violations}
    if (len(all_candidate_violations) > 1 and
            len(candidate_violation_locations) == 1):
        item = all_candidate_violations[0]
        candidate_violation_note = (
            f"候选侧的 {len(all_candidate_violations)} 次超时均为同一固定局面："
            f"opening {item['openingId']}、5.8.1 执"
            f"{'黑' if item['newColor'] == 1 else '白'}、ply {item['ply']}、"
            f"走 ({item['x']},{item['y']})，属于跨对手重复出现的尾延迟。")
    else:
        candidate_violation_note = ""
    lines += [
        "",
        f"三组汇总为 **{wdl(aggregate['results'])}**，得分率 "
        f"**{pct(aggregate['results']['scoreRate'])}**。该合计只描述这三个不同对手的"
        "测试总量，不能用来代替逐对手结论。",
        "",
        f"逐对手点估计为：{point_estimates}。显著性应同时参考 Wilson 和按 50 个开局"
        "成对 bootstrap 的区间；颜色交换结果仍需按执黑、执白和配对开局分别解释。",
        "",
        "## 统一测试口径",
        "",
        "- Suite：`five-star-natural-final`；seed domain："
        f"`{seed_domain}`。",
        "- Master seed：`0x9ee4d91480ac5889`；开局 ID 0–49。",
        f"- 规则：{rule_label}；每局最多 120 手。",
        "- 选择：deterministic-best；策略：hybrid-deep-verified。",
        "- 每个开局交换颜色；每个对手 100 局，黑白各 50。",
        "- 5.8.1 sentinel：adaptive depth 5/7、16,000 nodes、80 ms、最多 2 个 early alternatives。",
        "- 决策内部预算 4,500 ms，玩家可见硬门槛 5,000 ms。",
        ("- 三组均为本轮新跑、单进程串行运行，并由同一当前 replay 工具重新验证。"
         if forbidden_black else
         "- 三组均单进程串行运行；四星和 5.4.1 使用此前同口径标准原始日志，"
         "三星为本轮补跑；三组均由同一当前 replay 工具重新验证。"),
        "",
        "## 逐对手详细结果",
        "",
    ]

    for cell in cells:
        result = cell["results"]
        same = cell["sameColor"]
        pair = cell["paired"]
        decisions = cell["decisions"]
        sentinel = cell["sentinel"]
        guard = cell["finalGuard"]
        replay = cell["replay"]
        lines += [
            f"### 对{cell['label']}",
            "",
            f"- 执黑：{wdl(result['black'])}，得分率 {pct(result['black']['scoreRate'])}，"
            f"Wilson 95% {interval(result['black']['wilson95'])}。",
            f"- 执白：{wdl(result['white'])}，得分率 {pct(result['white']['scoreRate'])}，"
            f"Wilson 95% {interval(result['white']['wilson95'])}。",
            f"- 总体：{wdl(result['overall'])}，得分率 {pct(result['overall']['scoreRate'])}，"
            f"Wilson 95% {interval(result['overall']['wilson95'])}。",
            f"- 同色比较：5.8.1 黑棋 {pct(same['candidateBlackScoreRate'])} vs "
            f"对手黑棋 {pct(same['opponentBlackScoreRate'])}，变化 "
            f"{same['blackDelta'] * 100:+.1f} 个百分点；5.8.1 白棋 "
            f"{pct(same['candidateWhiteScoreRate'])} vs 对手白棋 "
            f"{pct(same['opponentWhiteScoreRate'])}，变化 "
            f"{same['whiteDelta'] * 100:+.1f} 个百分点。",
            f"- 50 个配对开局：5.8.1 双杀 {pair['candidateSweeps']}，对手双杀 "
            f"{pair['opponentSweeps']}，各胜一盘 {pair['splitDecisive']}，含和棋配对 "
            f"{pair['involvingDraw']}；配对得分率 {pct(pair['candidatePairedScoreRate'])}，"
            f"bootstrap 95% {interval(pair['pairedBootstrap95'])}。",
            f"- 终局类型：{json.dumps(cell['games']['terminations'], ensure_ascii=False)}；"
            f"棋局手数 p50/p95/max = {cell['games']['moves']['p50']:.0f}/"
            f"{cell['games']['moves']['p95']:.0f}/{cell['games']['moves']['max']:.0f}。",
            "",
            "决策和运行时：",
            "",
            f"- 5.8.1 决策 {decisions['candidate']} 次（执黑局 "
            f"{decisions['candidateByColor']['black']}、执白局 "
            f"{decisions['candidateByColor']['white']}）；对手决策 "
            f"{decisions['opponent']} 次。",
            f"- 5.8.1 wall p50/p95/max = {ms(decisions['candidateWallMs'])} ms；"
            f"CPU = {ms(decisions['candidateCpuMs'])} ms；nodes = "
            f"{nodes(decisions['candidateNodes'])}。",
            f"- 对手 wall p50/p95/max = {ms(decisions['opponentWallMs'])} ms；"
            f"CPU = {ms(decisions['opponentCpuMs'])} ms；nodes = "
            f"{nodes(decisions['opponentNodes'])}。",
            f"- 峰值 RSS：5.8.1 {decisions['candidatePeakResidentBytes'] / 1048576:.1f} MiB；"
            f"对手 {decisions['opponentPeakResidentBytes'] / 1048576:.1f} MiB。",
            f"- 最大 5.8.1 决策：opening {decisions['candidateMax']['openingId']}、"
            f"ply {decisions['candidateMax']['ply']}、"
            f"({decisions['candidateMax']['x']},{decisions['candidateMax']['y']})、"
            f"{decisions['candidateMax']['ms']:.3f} ms。",
            f"- 最大对手决策：opening {decisions['opponentMax']['openingId']}、"
            f"ply {decisions['opponentMax']['ply']}、"
            f"({decisions['opponentMax']['x']},{decisions['opponentMax']['y']})、"
            f"{decisions['opponentMax']['ms']:.3f} ms。",
            f"- >5,000 ms 决策 {len(decisions['hardLimitViolations'])}；runner anomaly "
            f"{len(cell['games']['anomalies'])}。",
            "",
            "Early micro-VCF 与最终 guard：",
            "",
            f"- Sentinel eligible {sentinel['eligible']}，skip {sentinel['skipped']}；"
            f"effective depth {json.dumps(sentinel['effectiveDepths'], ensure_ascii=False)}；"
            f"adaptive escalation {sentinel['adaptiveEscalations']}。",
            f"- Sentinel 状态 {json.dumps(sentinel['statuses'], ensure_ascii=False)}；"
            f"审计 alternatives {sentinel['auditedAlternatives']}，其中 verified-loss "
            f"{sentinel['verifiedLossAlternatives']}。",
            f"- 提前避免 verified loss {sentinel['verifiedLossesAvoided']} 次；"
            f"early move change {sentinel['moveChanges']} 次；final-guard-only catch "
            f"{sentinel['finalGuardOnlyCatches']} 次。",
            f"- Sentinel latency p50/p95/max = {ms(sentinel['latencyMs'])} ms；"
            f"nodes = {nodes(sentinel['nodes'])}；cache reuse decisions/hits = "
            f"{sentinel['cacheReuseDecisions']}/{sentinel['cacheHits']}。",
            f"- Final guard eligible {guard['eligible']}；审计 candidates "
            f"{guard['auditedCandidates']}；verified-loss alternatives "
            f"{guard['verifiedLossAlternatives']}；避免 verified loss "
            f"{guard['verifiedLossesAvoided']}。",
            f"- Final guard VCF latency p50/p95/max = {ms(guard['vcfLatencyMs'])} ms；"
            f"VCT eligible {guard['vctEligible']}，VCT latency = "
            f"{ms(guard['vctLatencyMs']) if guard['vctLatencyMs']['count'] else 'n/a'} ms。",
            f"- Sentinel rollback {sentinel['rollbacks']}，final guard rollback "
            f"{guard['rollbacks']}，evidence mismatch {sentinel['evidenceMismatches']}。",
            "",
            "独立重放：",
            "",
            f"- 100 局、{replay['movesReplayed']} 手全部重放；合法性、胜负、棋盘完整性通过。",
            f"- 冷启动重新证明 early VCF "
            f"{replay['certificatesReproved']['earlyVCF']}、final VCF "
            f"{replay['certificatesReproved']['finalGuardVCF']}、final VCT "
            f"{replay['certificatesReproved']['finalGuardVCT']}，合计 "
            f"{replay['certificatesReproved']['total']} 个证书。",
            f"- Runtime 已独立验证的一般分析证书 "
            f"{replay['runtimeVerifiedAnalysisCertificates']}；replay anomaly "
            f"{len(replay['anomalies'])}。",
            "",
        ]

    lines += [
        "## 运行时门槛汇总",
        "",
        "| 对手 | 5.8.1 最大决策 | 对手最大决策 | >5秒来源 | Runner anomaly | Replay |",
        "| --- | ---: | ---: | --- | ---: | --- |",
    ]
    for cell in cells:
        violations = cell["decisions"]["hardLimitViolations"]
        source = "无" if not violations else "; ".join(
            f"{item['engine']} opening {item['openingId']} ply {item['ply']} "
            f"{item['ms']:.3f}ms" for item in violations)
        lines.append(
            f"| {cell['label']} | {cell['decisions']['candidateMax']['ms']:.3f} ms | "
            f"{cell['decisions']['opponentMax']['ms']:.3f} ms | {source} | "
            f"{len(cell['games']['anomalies'])} | pass |")
    lines += [
        "",
        ("5.8.1 在三组共 300 局中没有任何决策超过 5,000 ms。"
         if not all_candidate_violations else
         f"5.8.1 在三组共 300 局中有 {len(all_candidate_violations)} 次决策超过 5,000 ms。")
        + ("对手侧没有决策超过 5,000 ms。"
           if not all_opponent_violations else
           f"对手侧共有 {len(all_opponent_violations)} 次决策超过 5,000 ms，"
           "具体位置见上表。"),
        candidate_violation_note,
        "",
        "## 综合解释",
        "",
    ]
    for cell in cells:
        score = cell["results"]["overall"]["scoreRate"]
        wilson95 = cell["results"]["overall"]["wilson95"]
        paired95 = cell["paired"]["pairedBootstrap95"]
        if wilson95[0] > 0.5 and paired95[0] > 0.5:
            inference = "两种 95% 区间均高于 50%"
        elif wilson95[1] < 0.5 and paired95[1] < 0.5:
            inference = "两种 95% 区间均低于 50%"
        else:
            inference = "两种 95% 区间未同时排除 50%"
        lines.append(
            f"- 对{cell['label']}：总体 {pct(score)}，{inference}；配对双杀 "
            f"{cell['paired']['candidateSweeps']} 比 "
            f"{cell['paired']['opponentSweeps']}。")
    avoided = sum(cell["sentinel"]["verifiedLossesAvoided"]
                  for cell in cells)
    final_catches = sum(cell["sentinel"]["finalGuardOnlyCatches"]
                        for cell in cells)
    lines += [
        f"- Early sentinel 三组共提前避免 verified loss {avoided} 次；final-guard-only "
        f"catch {final_catches} 次。两层机制的具体负载和收益应结合逐对手遥测解释。",
        "- 黑白表现和换色配对结果见逐对手部分；不同规则下不可直接套用自由规则结论。",
        ("- 三星单元有 1 局 `legacy-illegal-move-loss`：旧三星执黑尝试禁手，"
         "非法着未落盘并按规则判负；独立重放已验证棋盘和胜方。"
         if any("legacy-illegal-move-loss" in cell["games"]["terminations"]
                for cell in cells) else ""),
        "",
        "## 限制",
        "",
        ("- 本报告只覆盖黑方禁手开启规则；不代表 Freestyle 表现。"
         if forbidden_black else
         "- 本报告只有 Freestyle；不代表有禁手规则表现。"),
        "- 每个对手仅 50 个配对开局；是否显著以逐对手 Wilson 与配对区间为准。",
        "- 三组复用了同一批开局以增强横向可比性，因此三组结果不是相互独立样本。",
        "- JSONL 保存一般分析证书 ID 和验证标志，但未保存所有证书节点；冷重证范围是"
        "能够从最终/临时根精确重建的 early/final guard 证书。",
        "- 计时会受机器负载影响；硬门槛判断以原始每步 wall `ms` 为准。",
        "",
        "## 证据文件",
        "",
        "最终目录按 `raw/`、`replay/`、`report/`、`provenance/` 组织。"
        "`summary.json` 保存本报告全部结构化统计，`paired_openings.csv` 保存每个对手"
        "每个开局的换色配对结果，`checksums.sha256` 固定交付文件哈希。",
        "",
    ]
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--three", type=Path, required=True)
    parser.add_argument("--four", type=Path, required=True)
    parser.add_argument("--v541", type=Path, required=True)
    parser.add_argument("--three-replay", type=Path, required=True)
    parser.add_argument("--four-replay", type=Path, required=True)
    parser.add_argument("--v541-replay", type=Path, required=True)
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-md", type=Path, required=True)
    parser.add_argument("--paired-csv", type=Path, required=True)
    parser.add_argument("--profiles-json", type=Path, required=True)
    args = parser.parse_args()

    specs = [
        ("冻结三星", args.three, args.three_replay, 0x581300),
        ("四星", args.four, args.four_replay, 0x581400),
        ("exact 5.4.1", args.v541, args.v541_replay, 0x581541),
    ]
    cells = []
    paired_rows = []
    for label, raw, replay, seed in specs:
        cell, rows = analyze_cell(label, raw, replay, seed)
        cells.append(cell)
        paired_rows.extend(rows)

    wins = sum(cell["results"]["overall"]["wins"] for cell in cells)
    draws = sum(cell["results"]["overall"]["draws"] for cell in cells)
    losses = sum(cell["results"]["overall"]["losses"] for cell in cells)
    aggregate_score = wins + 0.5 * draws
    aggregate = {
        "generatedAt": datetime.now(ZoneInfo("Asia/Shanghai")).isoformat(),
        "model": "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a",
        "forbiddenBlack": cells[0]["header"]["forbiddenBlack"],
        "cells": 3,
        "games": wins + draws + losses,
        "results": {
            "games": wins + draws + losses,
            "wins": wins, "draws": draws, "losses": losses,
            "scoreRate": aggregate_score / (wins + draws + losses),
            "wilson95": wilson(aggregate_score, wins + draws + losses),
        },
        "candidateHardLimitViolations": sum(
            item["engine"] == "new" for cell in cells
            for item in cell["decisions"]["hardLimitViolations"]),
        "allHardLimitViolations": sum(
            len(cell["decisions"]["hardLimitViolations"]) for cell in cells),
        "runnerAnomalies": sum(
            len(cell["games"]["anomalies"]) for cell in cells),
        "certificatesReproved": sum(
            cell["replay"]["certificatesReproved"]["total"] for cell in cells),
    }
    document = {"schemaVersion": 1, "aggregate": aggregate, "cells": cells}
    args.output_json.write_text(
        json.dumps(document, ensure_ascii=False, indent=2) + "\n")
    args.output_md.write_text(render_markdown(cells, aggregate))
    args.profiles_json.write_text(json.dumps({
        "schemaVersion": 1,
        "source": "raw JSONL headers",
        "headers": [cell["header"] for cell in cells],
    }, ensure_ascii=False, indent=2) + "\n")
    with args.paired_csv.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "opponent", "openingId", "candidateBlackPoints",
            "candidateWhitePoints", "candidatePairPoints",
            "candidateBlackWinner", "candidateWhiteWinner",
        ])
        writer.writeheader()
        writer.writerows(paired_rows)
    print(
        f"v5.8.1 detailed report: {aggregate['games']} games; "
        f"W/D/L={wins}/{draws}/{losses}; "
        f"certificates={aggregate['certificatesReproved']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
