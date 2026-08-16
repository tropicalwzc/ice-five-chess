#!/usr/bin/env python3
"""Replay the 97 audited fallback fixtures under the fixed worker matrix."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import time
from pathlib import Path


WORKER_MATRIX = (1, 4, 8)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def board_encoding(stones: list[list[int]]) -> str:
    cells = ["."] * (15 * 15)
    for x, y, side in stones:
        if not (0 <= x < 15 and 0 <= y < 15 and side in (-1, 1)):
            raise ValueError(f"invalid stone {(x, y, side)}")
        index = x * 15 + y
        if cells[index] != ".":
            raise ValueError(f"duplicate stone {(x, y)}")
        cells[index] = "X" if side == 1 else "O"
    return "".join(cells)


def load_manifest(path: Path) -> dict:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("fixtureVersion") != "five-star-fork-recovery-fixtures-v1":
        raise ValueError("unexpected fixture manifest version")
    entries = manifest.get("entries", [])
    if manifest.get("fixtureCount") != len(entries) or len(entries) != 97:
        raise ValueError("fixture manifest must contain exactly 97 entries")
    ids = [entry["id"] for entry in entries]
    if len(set(ids)) != len(ids):
        raise ValueError("fixture IDs are not unique")
    for source in manifest.get("sources", []):
        source_path = Path(source["path"])
        if not source_path.is_file():
            raise FileNotFoundError(source_path)
        if sha256(source_path) != source["sha256"]:
            raise ValueError(f"source hash drift: {source_path}")
    return manifest


def input_lines(entries: list[dict]) -> str:
    lines = []
    for entry in entries:
        hint_x, hint_y = entry["legacyHint"]
        lines.append("\t".join((
            entry["id"],
            "1" if entry["forbiddenBlack"] else "0",
            str(entry["sideToMove"]),
            str(hint_x),
            str(hint_y),
            str(entry["seed"]),
            board_encoding(entry["stones"]),
        )))
    return "\n".join(lines) + "\n"


def run_cell(runner: Path, entries: list[dict], worker_count: int,
             mode: str, token_block_size: int | None) -> list[dict]:
    if worker_count not in WORKER_MATRIX:
        raise ValueError(
            f"invalid worker cell {worker_count}; expected 1, 4, or 8"
        )
    started = time.monotonic()
    command = [str(runner), "--worker-count", str(worker_count)]
    if mode == "baseline":
        command.append("--baseline")
    elif mode == "scheduler":
        command.append("--scheduler")
    elif mode == "v541-baseline":
        command.append("--v541-baseline")
    elif mode == "v541-scheduler":
        command.append("--v541-scheduler")
    elif mode == "branch-first":
        command.append("--branch-first")
    if token_block_size is not None:
        command.extend(("--token-block-size", str(token_block_size)))
    process = subprocess.run(
        command,
        input=input_lines(entries), text=True, capture_output=True, check=False,
    )
    if process.returncode != 0:
        raise RuntimeError(
            f"worker cell {worker_count} failed ({process.returncode}): "
            f"{process.stderr.strip()}"
        )
    rows = [json.loads(line) for line in process.stdout.splitlines()
            if line.strip()]
    if len(rows) != len(entries):
        raise ValueError(
            f"worker cell {worker_count}: expected {len(entries)} rows, "
            f"got {len(rows)}"
        )
    expected_ids = [entry["id"] for entry in entries]
    actual_ids = [row.get("id") for row in rows]
    if actual_ids != expected_ids:
        raise ValueError(f"worker cell {worker_count}: fixture order drift")
    for row in rows:
        row["wallClockMs"] = row.get("elapsedMs", 0.0)
        row["runnerWallClockMs"] = 1000.0 * (time.monotonic() - started)
    return rows


def aggregate(rows: list[dict]) -> dict:
    totals = {
        "fixtures": len(rows),
        "legal": sum(bool(row["legal"]) for row in rows),
        "found": sum(bool(row["found"]) for row in rows),
        "unknown": sum(row["decisionStatus"] == 3 for row in rows),
        "verifiedWins": sum(row["decisionStatus"] == 1 for row in rows),
        "verifiedLosses": sum(row["decisionStatus"] == 2 for row in rows),
        "fallbacks": sum(bool(row["fallbackUsed"]) for row in rows),
        "forkAvoided": sum(row["forkAvoidedCount"] > 0 for row in rows),
        "forkRiskySelected": sum(row["forkRisk"] == 3 for row in rows),
        "coverageComplete": sum(bool(row["candidateCoverageComplete"])
                                 for row in rows),
        "memoryLiveNonzero": sum(row["decisionMemoryReserved"] != 0
                                  for row in rows),
        "memoryPeakBytes": sum(row["decisionMemoryPeakReserved"]
                                for row in rows),
        "memoryReleasedBytes": sum(row["decisionMemoryReleased"]
                                    for row in rows),
        "ledgerExhaustions": sum(
            row["diagnostics"]["decisionLedgerExhaustions"] for row in rows
        ),
        "allocations": sum(
            row["diagnostics"].get("allocations", 0) for row in rows
        ),
        "clearedBytes": sum(
            row["diagnostics"].get("clearedBytes", 0) for row in rows
        ),
        "proofSessionQueries": sum(
            row["diagnostics"].get("proofSessionQueries", 0)
            for row in rows
        ),
        "proofSessions": sum(row["diagnostics"]["parallelBatches"]
                              for row in rows),
        "completedProofJobs": sum(
            row["diagnostics"]["parallelRootJobsCompleted"] +
            row["diagnostics"]["parallelEscapeJobsCompleted"]
            for row in rows
        ),
        "usefulParallelWork": sum(
            row["diagnostics"]["parallelIndependentDispatches"] +
            row["diagnostics"]["parallelRootJobsCompleted"] +
            row["diagnostics"]["parallelEscapeJobsCompleted"]
            for row in rows
        ),
        "maxConcurrentWorkers": max(
            (max(row["diagnostics"]["parallelMaxConcurrentWorkers"],
                 row["diagnostics"]["parallelEscapeMaxConcurrentWorkers"])
             for row in rows), default=0
        ),
        "poolDispatches": sum(
            row["diagnostics"].get("parallelPoolDispatches", 0)
            for row in rows
        ),
        "poolWorkersReused": sum(
            row["diagnostics"].get("parallelPoolWorkersReused", 0)
            for row in rows
        ),
        "poolFallbacks": sum(
            row["diagnostics"].get("parallelPoolFallbacks", 0)
            for row in rows
        ),
        "escapeWorkersLaunched": sum(
            row["diagnostics"].get("parallelEscapeWorkersLaunched", 0)
            for row in rows
        ),
        "tokenBlockClaims": sum(
            row["diagnostics"].get("parallelTokenBlockClaims", 0)
            for row in rows
        ),
        "tokenBlockTokens": sum(
            row["diagnostics"].get("parallelTokenBlockTokens", 0)
            for row in rows
        ),
        "tokenBlockReturns": sum(
            row["diagnostics"].get("parallelTokenBlockReturns", 0)
            for row in rows
        ),
        "branchFirstPreviewBranches": sum(
            row["diagnostics"].get("branchFirstPreviewBranches", 0)
            for row in rows
        ),
        "branchFirstWaves": sum(
            row["diagnostics"].get("branchFirstWaves", 0)
            for row in rows
        ),
        "branchFirstJobs": sum(
            row["diagnostics"].get("branchFirstJobs", 0)
            for row in rows
        ),
        "branchFirstJobsCompleted": sum(
            row["diagnostics"].get("branchFirstJobsCompleted", 0)
            for row in rows
        ),
        "branchFirstUsefulJobs": sum(
            row["diagnostics"].get("branchFirstUsefulJobs", 0)
            for row in rows
        ),
        "branchFirstMaxConcurrentWorkers": max(
            (row["diagnostics"].get("branchFirstMaxConcurrentWorkers", 0)
             for row in rows), default=0
        ),
        "branchFirstDepthExtensions": sum(
            row["diagnostics"].get("branchFirstDepthExtensions", 0)
            for row in rows
        ),
        "branchFirstAdvancedFourDepthExtensions": sum(
            row["diagnostics"].get(
                "branchFirstAdvancedFourDepthExtensions", 0
            )
            for row in rows
        ),
        "branchFirstAdvancedThreeDepthExtensions": sum(
            row["diagnostics"].get(
                "branchFirstAdvancedThreeDepthExtensions", 0
            )
            for row in rows
        ),
        "branchFirstMaxChildDepth": max(
            (row["diagnostics"].get("branchFirstMaxChildDepth", 0)
             for row in rows), default=0
        ),
        "cpuMs": sum(float(row["cpuMs"]) for row in rows),
        "tokenBlockSize": next(
            (int(row.get("parallelTokenBlockSize", 0)) for row in rows), 0
        ),
    }
    return totals


def gate_cells(entries: list[dict], cell_rows: dict[int, list[dict]]) -> dict:
    by_worker = {
        worker: {row["id"]: row for row in rows}
        for worker, rows in cell_rows.items()
    }
    drift = []
    illegal = []
    false_loss = []
    unreconciled = []
    ledger_drift = []
    avoidable_fork = []
    for entry in entries:
        fixture_id = entry["id"]
        rows = [by_worker[worker][fixture_id] for worker in WORKER_MATRIX]
        baseline = (rows[0]["x"], rows[0]["y"], rows[0]["decisionStatus"],
                    rows[0]["forkRisk"])
        for row in rows:
            observed = (row["x"], row["y"], row["decisionStatus"],
                        row["forkRisk"])
            if observed != baseline:
                drift.append({"id": fixture_id, "baseline": baseline,
                              "observed": observed,
                              "workerCount": row["requestedWorkers"]})
            if not row["found"] or not row["legal"]:
                illegal.append({"id": fixture_id,
                                "workerCount": row["requestedWorkers"]})
            if row["decisionStatus"] == 2 or row.get("provenLoss", False):
                false_loss.append({"id": fixture_id,
                                   "workerCount": row["requestedWorkers"]})
            if row["actualWorkerCap"] != row["requestedWorkers"]:
                ledger_drift.append({"id": fixture_id,
                                     "workerCount": row["requestedWorkers"],
                                     "actualWorkerCap": row["actualWorkerCap"]})
            if row["decisionMemoryReserved"] != 0:
                unreconciled.append({"id": fixture_id,
                                     "workerCount": row["requestedWorkers"],
                                     "liveBytes": row["decisionMemoryReserved"]})
            if (row["decisionMemoryReleased"] <
                    row["decisionMemoryPeakReserved"]):
                ledger_drift.append({
                    "id": fixture_id,
                    "workerCount": row["requestedWorkers"],
                    "peakBytes": row["decisionMemoryPeakReserved"],
                    "releasedBytes": row["decisionMemoryReleased"],
                })
            if (row["candidateCoverageComplete"] and
                    row["forkSafeCandidates"] > 0 and row["forkRisk"] == 3):
                avoidable_fork.append({"id": fixture_id,
                                       "workerCount": row["requestedWorkers"]})
    return {
        "passed": not (drift or illegal or false_loss or unreconciled or
                        avoidable_fork),
        "selectedMoveDrift": drift,
        "illegalOutputs": illegal,
        "falseVerifiedLoss": false_loss,
        "unreconciledMemory": unreconciled,
        "ledgerTelemetryDrift": ledger_drift,
        "avoidableForks": avoidable_fork,
    }


def markdown(report: dict) -> str:
    lines = [
        "# Five-star fork-recovery fixture replay",
        "",
        f"- Manifest: `{report['fixtureManifest']}`",
        f"- Fixtures: {report['fixtureCount']} "
        f"(free {report['counts']['free']}, "
        f"forbidden {report['counts']['forbidden']})",
        f"- Worker matrix: `{report['workerMatrix']}`",
        f"- Profile: `{report['profileVersion']}`",
        f"- Token block size: `{report['tokenBlockSize']}`",
        f"- Fixed-position gate: **{'PASS' if report['gate']['passed'] else 'FAIL'}**",
        "",
        "| workers | legal | unknown | fallbacks | completed proof jobs | "
        "useful work | pool dispatches | token claims | arena allocs | "
        "cleared MiB | CPU ms |",
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for worker in report["workerMatrix"]:
        totals = report["cells"][str(worker)]["aggregate"]
        lines.append(
            f"| {worker} | {totals['legal']}/{totals['fixtures']} | "
            f"{totals['unknown']} | {totals['fallbacks']} | "
            f"{totals['completedProofJobs']} | "
            f"{totals['usefulParallelWork']} | "
            f"{totals['poolDispatches']} | {totals['tokenBlockClaims']} | "
            f"{totals['allocations']} | "
            f"{totals['clearedBytes'] / (1024 * 1024):.1f} | "
            f"{totals['cpuMs']:.1f} |"
        )
    lines.extend([
        "",
        "Branch-first telemetry: preview branches and recursive waves/jobs "
        "are reported separately from root jobs; useful work counts only "
        "verified child certificates.",
        "",
        "| workers | previews | waves | branch jobs | completed | useful | "
        "max concurrent | depth extensions | adv-four +2 | adv-three +1 | "
        "max child depth |",
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ])
    for worker in report["workerMatrix"]:
        totals = report["cells"][str(worker)]["aggregate"]
        lines.append(
            f"| {worker} | {totals['branchFirstPreviewBranches']} | "
            f"{totals['branchFirstWaves']} | {totals['branchFirstJobs']} | "
            f"{totals['branchFirstJobsCompleted']} | "
            f"{totals['branchFirstUsefulJobs']} | "
            f"{totals['branchFirstMaxConcurrentWorkers']} | "
            f"{totals['branchFirstDepthExtensions']} | "
            f"{totals['branchFirstAdvancedFourDepthExtensions']} | "
            f"{totals['branchFirstAdvancedThreeDepthExtensions']} | "
            f"{totals['branchFirstMaxChildDepth']} |"
        )
    lines.extend([
        "",
        "Parallel advantage is reported as useful completed proof work and "
        "fixed-fixture outcome changes; worker count alone is not treated as "
        "an improvement.",
        "",
        f"Gate details: `{json.dumps(report['gate'], ensure_ascii=False)}`",
    ])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--workers", type=int, action="append")
    parser.add_argument("--mode", choices=("fork", "baseline", "scheduler",
                                             "v541-baseline",
                                             "v541-scheduler", "branch-first"),
                        default="fork")
    parser.add_argument("--token-block-size", type=int)
    args = parser.parse_args()
    if (args.token_block_size is not None and
            not 1 <= args.token_block_size <= 1048576):
        print("--token-block-size must be in 1..1048576", file=sys.stderr)
        return 2
    workers = tuple(args.workers or WORKER_MATRIX)
    if set(workers) != set(WORKER_MATRIX) or len(workers) != len(WORKER_MATRIX):
        print("--workers must contain exactly 1, 4, and 8", file=sys.stderr)
        return 2
    if not args.runner.is_file():
        print(f"runner not found: {args.runner}", file=sys.stderr)
        return 2
    manifest = load_manifest(args.manifest)
    entries = manifest["entries"]
    cell_rows = {worker: run_cell(
        args.runner, entries, worker, args.mode, args.token_block_size)
                 for worker in workers}
    ordered_cells = {
        str(worker): {
            "workerCount": worker,
            "aggregate": aggregate(cell_rows[worker]),
            "records": cell_rows[worker],
        }
        for worker in WORKER_MATRIX
    }
    report = {
        "schemaVersion": 1,
        "fixtureManifest": str(args.manifest),
        "fixtureManifestSha256": sha256(args.manifest),
        "fixtureCount": len(entries),
        "counts": manifest["counts"],
        "profileVersion": next(
            (row.get("profileVersion") for rows in cell_rows.values()
             for row in rows if row.get("profileVersion")),
            manifest["profileVersion"],
        ),
        "mode": args.mode,
        "tokenBlockSize": args.token_block_size,
        "workerMatrix": list(WORKER_MATRIX),
        "cells": ordered_cells,
    }
    report["gate"] = gate_cells(entries, cell_rows)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "five_star_fork_recovery_replay.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (args.output_dir / "five_star_fork_recovery_replay.md").write_text(
        markdown(report), encoding="utf-8"
    )
    print(json.dumps({
        "gate": report["gate"]["passed"],
        "fixtures": report["fixtureCount"],
        "workers": report["workerMatrix"],
    }))
    return 0 if report["gate"]["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
