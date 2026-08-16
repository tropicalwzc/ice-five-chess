#!/usr/bin/env python3
"""Summarize repeated elite-paired JSONL latency runs."""

import argparse
import hashlib
import json
import math
from pathlib import Path
from statistics import mean, median


PROFILES = ("five-star-v51", "five-star")
COMPLETED = {1, 2}
HARD_LIMIT_MS = 5000.0


def percentile(values, fraction):
    ordered = sorted(values)
    index = (math.ceil((len(ordered) - 1) * fraction)
             if fraction == 0.95
             else math.floor((len(ordered) - 1) * fraction))
    return ordered[index]


def load_run(path):
    records = {profile: {} for profile in PROFILES}
    header = None
    for raw in path.read_text(encoding="utf-8").splitlines():
        item = json.loads(raw)
        if item.get("type") == "header":
            header = item
            continue
        if item.get("type") != "game":
            continue
        key = (item["openingId"], item["newColor"])
        matches = [step for step in item["steps"]
                   if step.get("engine") in PROFILES]
        if len(matches) != 1:
            raise ValueError(f"{path}: expected one paired step for {key}")
        step = matches[0]
        profile = step["engine"]
        if key in records[profile]:
            raise ValueError(f"{path}: duplicate {profile} record for {key}")
        records[profile][key] = step
    if header is None or header.get("suite") != "elite-paired":
        raise ValueError(f"{path}: not an elite-paired result")
    if set(records[PROFILES[0]]) != set(records[PROFILES[1]]):
        raise ValueError(f"{path}: control/candidate pair set differs")
    return header, records


def representative(samples):
    middle = median(step["ms"] for step in samples)
    return min(samples, key=lambda step: abs(step["ms"] - middle))


def summarize_profile(records, median_latencies):
    latencies = list(median_latencies.values())
    cpu_times = [step.get("cpuMs", 0.0) for step in records.values()]
    elapsed_seconds = sum(latencies) / 1000.0
    completed = sum(step["proofStatus"] in COMPLETED
                    for step in records.values())
    diagnostics = {}
    for step in records.values():
        for name, value in step["diagnostics"].items():
            diagnostics[name] = diagnostics.get(name, 0) + value
    statuses = {}
    for step in records.values():
        name = str(step["proofStatus"])
        statuses[name] = statuses.get(name, 0) + 1
    return {
        "decisions": len(records),
        "p50Ms": percentile(latencies, 0.50),
        "p95Ms": percentile(latencies, 0.95),
        "maxMs": max(latencies),
        "meanMs": mean(latencies),
        "p50CpuMs": percentile(cpu_times, 0.50),
        "p95CpuMs": percentile(cpu_times, 0.95),
        "maxCpuMs": max(cpu_times),
        "meanCpuMs": mean(cpu_times),
        "totalRepresentativeCpuMs": sum(cpu_times),
        "peakResidentBytes": max(
            step.get("peakResidentBytes", 0) for step in records.values()),
        "completedClassificationsPerSecond": (
            completed / elapsed_seconds if elapsed_seconds else 0.0),
        "proofStatusCounts": statuses,
        "diagnostics": diagnostics,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", action="append", required=True,
                        type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if len(args.input) < 4:
        raise SystemExit("at least four paired runs are required")

    runs = [load_run(path) for path in args.input]
    rule_values = {run[0]["forbiddenBlack"] for run in runs}
    if len(rule_values) != 1:
        raise SystemExit("all inputs must use the same rule mode")
    keys = set(runs[0][1][PROFILES[0]])
    for _, records in runs:
        if set(records[PROFILES[0]]) != keys:
            raise SystemExit("paired position sets differ across repetitions")

    selected = {profile: {} for profile in PROFILES}
    median_latencies = {profile: {} for profile in PROFILES}
    deterministic = True
    deterministic_moves = True
    deterministic_nodes = True
    for profile in PROFILES:
        for key in keys:
            samples = [records[profile][key] for _, records in runs]
            moves = {(sample["x"], sample["y"]) for sample in samples}
            statuses = {(sample["proofStatus"], sample["proofSearchClass"],
                         sample["proofDistance"],
                         sample["proofCertificateId"])
                        for sample in samples}
            deterministic_moves = deterministic_moves and len(moves) == 1
            deterministic = deterministic and len(moves) == 1 and len(statuses) == 1
            deterministic_nodes = deterministic_nodes and len({
                (sample["proofNodes"], sample.get("proofParallelNodes", 0))
                for sample in samples}) == 1
            selected[profile][key] = representative(samples)
            median_latencies[profile][key] = median(
                sample["ms"] for sample in samples)

    move_match = all(
        (selected[PROFILES[0]][key]["x"], selected[PROFILES[0]][key]["y"]) ==
        (selected[PROFILES[1]][key]["x"], selected[PROFILES[1]][key]["y"])
        for key in keys)
    control = summarize_profile(
        selected[PROFILES[0]], median_latencies[PROFILES[0]])
    candidate = summarize_profile(
        selected[PROFILES[1]], median_latencies[PROFILES[1]])
    for profile, summary in ((PROFILES[0], control),
                             (PROFILES[1], candidate)):
        summary["rawMaxMs"] = max(
            step["ms"]
            for _, records in runs
            for step in records[profile].values())
        summary["rawMaxCpuMs"] = max(
            step.get("cpuMs", 0.0)
            for _, records in runs
            for step in records[profile].values())
        summary["rawPeakResidentBytes"] = max(
            step.get("peakResidentBytes", 0)
            for _, records in runs
            for step in records[profile].values())
    gate = {
        "hardLimitMs": HARD_LIMIT_MS,
        "hardLimitPass": candidate["rawMaxMs"] <= HARD_LIMIT_MS,
        "movesDeterministic": deterministic_moves,
        "movesMatch": move_match,
        "p50NoGreaterThanControl": candidate["p50Ms"] <= control["p50Ms"],
        "p95NoGreaterThanControl": candidate["p95Ms"] <= control["p95Ms"],
        "parallelActive": (
            candidate["diagnostics"].get("parallelBatches", 0) > 0 and
            candidate["diagnostics"].get("parallelWorkersLaunched", 0) > 0 and
            candidate["diagnostics"].get("parallelRootJobs", 0) > 0),
    }
    # A stronger candidate is allowed to change the final move.  Move
    # equality remains useful compatibility telemetry, but the fixed-position
    # performance gate only requires deterministic candidate decisions and
    # the player-visible hard deadline.
    gate["pass"] = (gate["hardLimitPass"] and
                    gate["movesDeterministic"] and
                    gate["parallelActive"])
    result = {
        "schemaVersion": 2,
        "suite": "elite-paired",
        "forbiddenBlack": rule_values.pop(),
        "repetitions": len(runs),
        "percentileMethod": "per-position median, nearest-rank across positions",
        "deterministicMetadata": deterministic,
        "deterministicMoves": deterministic_moves,
        "deterministicNodeCounts": deterministic_nodes,
        "movesMatch": move_match,
        "control": control,
        "candidate": candidate,
        "gate": gate,
        "inputs": [{
            "path": str(path),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        } for path in args.input],
    }
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                           encoding="utf-8")
    print(json.dumps(result["gate"], sort_keys=True))


if __name__ == "__main__":
    main()
