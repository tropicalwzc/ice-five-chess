#!/usr/bin/env python3
"""Generate black-first directional A/B reporting for the opponent guard."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
from statistics import median


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_jsonl(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text().splitlines() if line]
    header = next(row for row in rows if row.get("type") == "header")
    games = [row for row in rows if row.get("type") == "game"]
    return header, games


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = min(len(ordered) - 1, math.ceil(fraction * len(ordered)) - 1)
    return ordered[index]


def wilson(score: float, games: int) -> list[float]:
    if games == 0:
        return [0.0, 0.0]
    z = 1.959963984540054
    center = (score + z * z / (2 * games)) / (1 + z * z / games)
    radius = z * math.sqrt(score * (1 - score) / games + z * z / (4 * games * games)) / (1 + z * z / games)
    return [max(0.0, center - radius), min(1.0, center + radius)]


def color_summary(games: list[dict], color: int) -> dict:
    selected = [game for game in games if game["newColor"] == color]
    wins = sum(game["winner"] == color for game in selected)
    draws = sum(game["winner"] == 0 for game in selected)
    losses = sum(game["winner"] == -color for game in selected)
    score = (wins + 0.5 * draws) / len(selected) if selected else 0.0
    return {"games": len(selected), "wins": wins, "draws": draws,
            "losses": losses, "scoreRate": score,
            "scoreRate95Wilson": wilson(score, len(selected))}


def summarize(label: str, path: Path) -> dict:
    header, games = read_jsonl(path)
    steps = [step for game in games for step in game["steps"]
             if step.get("engine") == "new"]
    guard = [step.get("opponentGuard", {}) for step in steps]
    latencies = [float(step["ms"]) for step in steps]
    black = color_summary(games, 1)
    white = color_summary(games, -1)
    overall_wins = black["wins"] + white["wins"]
    overall_draws = black["draws"] + white["draws"]
    overall_losses = black["losses"] + white["losses"]
    overall_score = (overall_wins + 0.5 * overall_draws) / len(games) if games else 0.0
    return {
        "label": label,
        "source": str(path),
        "sha256": sha256(path),
        "profile": header["newProfile"],
        "black": black,
        "white": white,
        "overall": {"games": len(games), "wins": overall_wins,
                    "draws": overall_draws, "losses": overall_losses,
                    "scoreRate": overall_score,
                    "scoreRate95Wilson": wilson(overall_score, len(games))},
        "guard": {
            "eligible": sum(bool(item.get("eligible")) for item in guard),
            "vcfCompleted": sum(item.get("vcfStatus") in (1, 2) for item in guard),
            "vctEligible": sum(bool(item.get("vctEligible")) for item in guard),
            "vctCompleted": sum(item.get("vctStatus") in (1, 2) for item in guard),
            "candidatesAudited": sum(int(item.get("auditedCount", 0)) for item in guard),
            "completedDisproofs": sum(int(item.get("completedDisproofs", 0)) for item in guard),
            "unknowns": sum(int(item.get("unknowns", 0)) for item in guard),
            "verifiedLosses": sum(int(item.get("verifiedLosses", 0)) for item in guard),
            "avoidedVerifiedLosses": sum(bool(item.get("avoidedVerifiedLoss")) for item in guard),
            "rollbacks": sum(bool(item.get("rollback")) for item in guard),
            "reservedNodes": sum(int(item.get("reservedNodes", 0)) for item in guard),
            "consumedNodes": sum(int(item.get("consumedNodes", 0)) for item in guard),
        },
        "latencyMs": {
            "p50": median(latencies) if latencies else 0.0,
            "p95": percentile(latencies, 0.95),
            "max": max(latencies, default=0.0),
            "over5000": sum(value > 5000.0 for value in latencies),
        },
        "budgetExhaustions": sum(bool(step.get("budgetExhausted")) for step in steps),
        "anomalies": sum(game.get("anomaly") is not None for game in games),
    }


def wdl(cell: dict) -> str:
    return f'{cell["wins"]}/{cell["draws"]}/{cell["losses"]} ({cell["scoreRate"]:.1%})'


def render_markdown(report: dict) -> str:
    control = report["control"]
    candidate = report["candidate"]
    lines = [
        "# Five-star opponent-guard directional smoke A/B",
        "",
        "> This 48-game paired smoke is directional only; it is not a formal strength claim.",
        "",
        "## Five-star as black (primary)",
        "",
        "| Profile | W/D/L | Score | 95% Wilson |",
        "|---|---:|---:|---:|",
    ]
    for cell in (control, candidate):
        black = cell["black"]
        interval = black["scoreRate95Wilson"]
        lines.append(f'| {cell["label"]} | {black["wins"]}/{black["draws"]}/{black["losses"]} | {black["scoreRate"]:.1%} | {interval[0]:.1%}–{interval[1]:.1%} |')
    lines += ["", "## White and overall", "",
              "| Profile | White W/D/L (score) | Overall W/D/L (score) |",
              "|---|---:|---:|"]
    for cell in (control, candidate):
        lines.append(f'| {cell["label"]} | {wdl(cell["white"])} | {wdl(cell["overall"])} |')
    lines += ["", "## Guard and resource evidence", "",
              "| Profile | Eligible | Audited | Disproofs | Unknown | Verified loss | Avoided | p50/p95/max ms | >5s |",
              "|---|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for cell in (control, candidate):
        guard = cell["guard"]
        latency = cell["latencyMs"]
        lines.append(f'| {cell["label"]} | {guard["eligible"]} | {guard["candidatesAudited"]} | {guard["completedDisproofs"]} | {guard["unknowns"]} | {guard["verifiedLosses"]} | {guard["avoidedVerifiedLosses"]} | {latency["p50"]:.1f}/{latency["p95"]:.1f}/{latency["max"]:.1f} | {latency["over5000"]} |')
    lines += ["", "## Provenance and limitations", "",
              f'- Control raw SHA-256: `{control["sha256"]}`',
              f'- Candidate raw SHA-256: `{candidate["sha256"]}`',
              f'- Schedule SHA-256: `{report["provenance"]["scheduleSha256"]}`',
              f'- Replay SHA-256: `{report["provenance"]["replaySha256"]}`',
              f'- Control command: `{report["commands"]["control"]}`',
              f'- Candidate command: `{report["commands"]["candidate"]}`',
              f'- Regeneration/replay command: `{report["commands"]["replay"]}`',
              "- Avoidable VCF/VCT incidents require independent replay output and are not inferred solely from runtime telemetry.",
              "- Any formal strength or forbidden-mode promotion claim requires a separately proposed, larger, freshly frozen dual-rule evaluation.",
              "", "## Predeclared gates", ""]
    for name, passed in report["gates"].items():
        if name == "allPassed":
            continue
        lines.append(f'- {name}: **{"PASS" if passed else "FAIL"}**')
    lines += ["", f'Playable binding decision: **{report["bindingDecision"]}**', ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-md", type=Path, required=True)
    parser.add_argument("--schedule", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--control-command", required=True)
    parser.add_argument("--candidate-command", required=True)
    parser.add_argument("--replay-command", required=True)
    args = parser.parse_args()
    control = summarize("UI-bound 5.4.1 control", args.control)
    candidate = summarize("opponent-guard candidate", args.candidate)
    replay = json.loads(args.replay.read_text())
    replay_cells = {cell["profile"]: cell for cell in replay["cells"]}
    control_incidents = replay_cells["control"][
        "avoidableLostBlackForcingIncidents"]
    candidate_incidents = replay_cells["candidate"][
        "avoidableLostBlackForcingIncidents"]
    gates = {
        "correctnessAndReplay": (
            replay["status"] == "pass" and control["anomalies"] == 0
            and candidate["anomalies"] == 0
            and replay["deterministicDiscreteResults"]),
        "hardDeadline": (control["latencyMs"]["over5000"] == 0
                         and candidate["latencyMs"]["over5000"] == 0),
        "noNewAvoidableForcingIncident": candidate_incidents <= control_incidents,
        "blackDirectionNonNegative": (
            candidate["black"]["scoreRate"] >= control["black"]["scoreRate"]),
        # "Obvious" is frozen before smoke as worse by more than one paired
        # game out of 24 for white or overall.
        "whiteNoObviousRegression": (
            candidate["white"]["scoreRate"] + 1 / 24
            >= control["white"]["scoreRate"]),
        "overallNoObviousRegression": (
            candidate["overall"]["scoreRate"] + 1 / 24
            >= control["overall"]["scoreRate"]),
    }
    gates["allPassed"] = all(gates.values())
    report = {
        "schemaVersion": 1,
        "classification": "directional-48-game-smoke-not-formal-strength",
        "control": control,
        "candidate": candidate,
        "forcingIncidents": {
            "control": replay_cells["control"]["lostBlackForcingIncidents"],
            "candidate": replay_cells["candidate"]["lostBlackForcingIncidents"],
        },
        "gates": gates,
        "bindingDecision": ("promote-candidate" if gates["allPassed"]
                            else "retain-ui-bound-5.4.1"),
        "commands": {"control": args.control_command,
                     "candidate": args.candidate_command,
                     "replay": args.replay_command},
        "provenance": {"schedule": str(args.schedule),
                       "scheduleSha256": sha256(args.schedule),
                       "replay": str(args.replay),
                       "replaySha256": sha256(args.replay)},
    }
    args.output_json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    args.output_md.write_text(render_markdown(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
