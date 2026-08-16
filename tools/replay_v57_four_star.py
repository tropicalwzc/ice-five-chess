#!/usr/bin/env python3
"""Replay and audit the five-star natural-prefix strength cells."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import re
from pathlib import Path


BOARD_SIZE = 15
BoardRow = ctypes.c_int * BOARD_SIZE
Board = BoardRow * BOARD_SIZE
VERSION = "5.7.0-white-v541-black-v521-independent-root-parallel8-5s"
BRANCH_FIRST_VERSION = "5.7.2-branch-first-preview-recursive-pool-14d-5s"
V541_SCHEDULER_VERSION = "5.4.2-v541-persistent-pool-token-blocks-8w-5s"
COMPONENTS = {
    -1: (2, "white-proof-engine",
         "5.4.1-transactional-deadline-root-parallel-5s"),
    1: (5, "black-v521-independent-root-parallel-8w-5s", VERSION),
}
BRANCH_FIRST_COMPONENTS = {
    -1: (6, "v57-branch-first-recursive-8w-5s", BRANCH_FIRST_VERSION),
    1: (6, "v57-branch-first-recursive-8w-5s", BRANCH_FIRST_VERSION),
}
V541_SCHEDULER_COMPONENTS = {
    -1: (7, "v541-persistent-pool-token-blocks-8w-5s",
         V541_SCHEDULER_VERSION),
    1: (7, "v541-persistent-pool-token-blocks-8w-5s",
         V541_SCHEDULER_VERSION),
}


def profile_config(version: str) -> dict:
    if version == V541_SCHEDULER_VERSION:
        return {
            "suite": "five-star-v541-final",
            "components": V541_SCHEDULER_COMPONENTS,
            "branchFirst": False,
            "schedulerOnly": True,
            "decisionTimeBudgetMs": 4200,
            "proofEmergencyTimeBudgetMs": 3800,
        }
    if version == BRANCH_FIRST_VERSION:
        return {
            "suite": "five-star-v57-final",
            "components": BRANCH_FIRST_COMPONENTS,
            "branchFirst": True,
            "schedulerOnly": False,
        }
    return {
        "suite": "five-star-v57-final",
        "components": COMPONENTS,
        "branchFirst": False,
        "schedulerOnly": False,
    }


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_u64(value: str | int) -> int:
    return int(value, 0) if isinstance(value, str) else int(value)


def load(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1 and len(games) == 100, path
    return headers[0], games


def configure_library(path: Path):
    library = ctypes.CDLL(str(path.resolve()))
    library.fc_is_legal_move.argtypes = [ctypes.POINTER(BoardRow), ctypes.c_int,
                                         ctypes.c_int, ctypes.c_int,
                                         ctypes.c_bool]
    library.fc_is_legal_move.restype = ctypes.c_bool
    library.fc_has_five.argtypes = [ctypes.POINTER(BoardRow), ctypes.c_int,
                                    ctypes.c_int, ctypes.c_int]
    library.fc_has_five.restype = ctypes.c_bool
    return library


def opening_prefixes(path: Path) -> list[list[list[int]]]:
    text = path.read_text(encoding="utf-8")
    entries = []
    for match in re.finditer(r"\{\s*(\d+),\s*\{([^}]*)\},", text):
        length = int(match.group(1))
        coordinates = [int(value) for value in
                       re.findall(r"[-+]?\d+", match.group(2))]
        assert len(coordinates) >= length * 2
        entries.append([[coordinates[2 * ply] + 7,
                         coordinates[2 * ply + 1] + 7,
                         1 if ply % 2 == 0 else -1]
                        for ply in range(length)])
    assert len(entries) == 50
    return entries


def validate_step(step: dict, side: int, expected_components: dict,
                  branch_first: bool, scheduler_only: bool) -> dict[str, int]:
    assert step["engine"] == "new"
    assert (step["hybridComponent"], step["hybridComponentName"],
            step["hybridComponentVersion"]) == expected_components[side]
    assert step["randomMode"] == 1
    assert step["randomEligibilityVerified"] is True
    assert parse_u64(step["randomEquivalenceSignature"]) != 0
    # A legal deterministic fallback is intentionally not a random choice.
    # It may therefore publish an empty random candidate set while retaining
    # the normal eligibility/equivalence audit fields.  The fork-first
    # recovery path has the same contract when an invalid four-star handoff
    # is replaced by a generated, fully probed candidate pool: those moves
    # are selected deterministically, but are not random alternatives.
    random_candidate_count = int(step["randomCandidateCount"])
    generated_recovery = (
        random_candidate_count == 0 and
        int(step.get("handoffReason", 0)) == 3 and
        bool(step.get("forkProbeComplete", False)) and
        int(step.get("forkCandidatesExamined", 0)) > 0 and
        int(step.get("candidateCount", 0)) >= 1
    )
    if step.get("fallbackUsed", False) or generated_recovery:
        assert step["randomCandidateCount"] == 0
        assert step["randomSelectionUsed"] is False
        assert step["randomSelectedRank"] == 0
        assert step["randomEligibilityVerified"] is True
    else:
        assert random_candidate_count >= 1
        assert 0 <= step["randomSelectedRank"] < step["randomCandidateCount"]
    if step["randomSelectionUsed"]:
        assert step["randomCandidateCount"] > 1
    if step["randomCandidateCount"] == 1:
        assert step["randomSelectedRank"] == 0
        assert step["randomSelectionUsed"] is False
    assert float(step["ms"]) <= 5000.0
    assert float(step.get("cpuMs", 0.0)) >= 0.0
    assert int(step.get("decisionStatus", 3)) in (0, 1, 2, 3)
    assert bool(step.get("candidateCoverageComplete", False)) in (True, False)
    assert int(step.get("proofWorkerCap", 0)) == 8
    jobs = int(step.get("proofParallelJobs", 0))
    completed = int(step.get("proofParallelJobsCompleted", 0))
    workers = int(step.get("proofWorkersLaunched", 0))
    assert 0 <= completed <= jobs and 0 <= workers <= jobs
    diagnostics = step["diagnostics"]
    assert diagnostics["parallelRootJobsCompleted"] <= diagnostics["parallelRootJobs"]
    assert diagnostics["parallelMaxConcurrentWorkers"] <= 8
    assert diagnostics["parallelEscapeJobsCompleted"] <= diagnostics["parallelEscapeJobs"]
    assert diagnostics["parallelEscapeMaxConcurrentWorkers"] <= 8
    if branch_first:
        assert diagnostics["branchFirstJobsCompleted"] <= diagnostics["branchFirstJobs"]
        assert diagnostics["branchFirstWorkersLaunched"] <= diagnostics["branchFirstJobs"]
        assert diagnostics["branchFirstMaxConcurrentWorkers"] <= 8
        assert diagnostics["branchFirstUsefulJobs"] <= diagnostics["branchFirstVerifiedJobs"]
    if scheduler_only:
        assert diagnostics.get("parallelPoolFallbacks", 0) == 0
        assert diagnostics.get("parallelTokenBlockReturns", 0) <= diagnostics.get(
            "parallelTokenBlockTokens", 0)
        assert diagnostics.get("parallelBudgetTokens", 0) <= 288000
    if step["proofStatus"] == 1:
        assert step["proofCertificateVerified"] is True
        assert step["proofNumber"] == 0
    if step["proofStatus"] == 2:
        assert step["disproofNumber"] == 0
    return {
        "moves": 1,
        "randomSelections": int(step["randomSelectionUsed"]),
        "budgetExhaustions": int(step["budgetExhausted"]),
        "certificates": int(step["proofCertificateVerified"]),
        "workers": workers,
        "jobs": jobs,
        "jobsCompleted": completed,
        "rootJobs": int(diagnostics["parallelRootJobs"]),
        "rootJobsCompleted": int(diagnostics["parallelRootJobsCompleted"]),
        "maxConcurrentWorkers": int(diagnostics["parallelMaxConcurrentWorkers"]),
        "independentDispatches": int(diagnostics["parallelIndependentDispatches"]),
        "escapeBatches": int(diagnostics["parallelEscapeBatches"]),
        "escapeJobs": int(diagnostics["parallelEscapeJobs"]),
        "escapeJobsCompleted": int(diagnostics["parallelEscapeJobsCompleted"]),
        "escapeMaxConcurrentWorkers": int(
            diagnostics["parallelEscapeMaxConcurrentWorkers"]),
        "escapeBudgetExhausted": int(
            diagnostics["parallelEscapeBudgetExhausted"]),
        "cachedLegalityChecks": int(
            diagnostics["forbiddenLegalityCachedChecks"]),
        "cacheMismatches": int(
            diagnostics["forbiddenLegalityCacheMismatches"]),
        "cacheValidationSamples": int(
            diagnostics.get("forbiddenLegalityCacheValidationSamples", 0)),
        "decisionUnknowns": int(diagnostics.get("decisionUnknowns", 0)),
        "decisionFallbacks": int(diagnostics.get("decisionFallbacks", 0)),
        "decisionLedgerExhaustions": int(
            diagnostics.get("decisionLedgerExhaustions", 0)),
        "duplicateRootEnumerations": int(
            diagnostics.get("parallelDuplicateRootEnumerations", 0)),
        "earlyStops": int(diagnostics.get("parallelEarlyStops", 0)),
        "postponedSiblings": int(diagnostics.get("dfpnPostponedSiblings", 0)),
        "dovetailRequeues": int(diagnostics.get("dfpnDovetailRequeues", 0)),
    }


def validate_cell(library, path: Path, opening_inc: Path,
                  forbidden: bool, opponent: str,
                  expected_version: str) -> dict:
    header, games = load(path)
    prefixes = opening_prefixes(opening_inc)
    config = profile_config(expected_version)
    branch_first = config["branchFirst"]
    expected_components = config["components"]
    assert header["schemaVersion"] >= 5
    assert header["suite"] == config["suite"]
    assert header["newProfile"]["version"] == expected_version
    assert header["newProfile"]["parallelProofEnabled"] is (not branch_first)
    assert header["newProfile"].get("branchFirstSearchEnabled", False) is branch_first
    assert header["newProfile"]["proofWorkerCount"] == 8
    if config["schedulerOnly"]:
        assert header["newProfile"]["persistentWorkerPoolEnabled"] is True
        assert header["newProfile"]["parallelTokenBlockEnabled"] is True
        assert header["newProfile"]["parallelTokenBlockSize"] == 64
        assert header["newProfile"]["decisionTimeBudgetMs"] == 4200
        assert header["newProfile"]["proofEmergencyTimeBudgetMs"] == 3800
        assert header["newProfile"]["decisionLedgerVersion"] == 0
        assert header["newProfile"]["decisionNodeBudget"] == 0
        assert header["newProfile"]["decisionHardLimitMs"] == 0
    assert header["forbiddenBlack"] is forbidden
    expected_opponent_engine = {
        "four-star": "four-star",
        "legacy": "legacy",
    }[opponent]
    expected_opponent_profile = {
        "four-star": "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book",
        "legacy": "legacy-three-star@5224020",
    }[opponent]
    assert header["opponentProfile"] == expected_opponent_profile
    assert header["randomMode"] == "user-softmax"
    assert header["openingStart"] == 0 and header["openingCount"] == 50
    assert header["games"] == 100
    expected_ids = {(opening, color) for opening in range(50)
                    for color in (1, -1)}
    actual_ids = {(int(game["openingId"]), int(game["newColor"]))
                  for game in games}
    assert actual_ids == expected_ids
    totals = {
        "games": 100, "moves": 0, "randomSelections": 0,
        "budgetExhaustions": 0, "certificates": 0, "workers": 0,
        "jobs": 0, "jobsCompleted": 0, "rootJobs": 0,
        "rootJobsCompleted": 0, "maxConcurrentWorkers": 0,
        "independentDispatches": 0, "escapeBatches": 0,
        "escapeJobs": 0, "escapeJobsCompleted": 0,
        "escapeMaxConcurrentWorkers": 0, "escapeBudgetExhausted": 0,
        "cachedLegalityChecks": 0, "cacheMismatches": 0,
        "cacheValidationSamples": 0, "decisionUnknowns": 0,
        "decisionFallbacks": 0, "decisionLedgerExhaustions": 0,
        "duplicateRootEnumerations": 0, "earlyStops": 0,
        "postponedSiblings": 0, "dovetailRequeues": 0,
    }
    for game in games:
        opening_id = int(game["openingId"])
        prefix = prefixes[opening_id]
        assert game["moves"][:len(prefix)] == prefix
        assert game["moveCount"] == len(game["moves"])
        assert len(game["moves"]) - len(prefix) == len(game["steps"])
        assert game["anomaly"] is None
        board = Board()
        winner = 0
        for ply, move in enumerate(game["moves"]):
            x, y, side = map(int, move)
            assert winner == 0
            assert side == (1 if ply % 2 == 0 else -1)
            assert library.fc_is_legal_move(board, x, y, side, forbidden)
            board[x][y] = side
            if ply >= len(prefix):
                step = game["steps"][ply - len(prefix)]
                assert (step["x"], step["y"], step["side"]) == (x, y, side)
                if side == int(game["newColor"]):
                    audited = validate_step(
                        step, side, expected_components, branch_first,
                        config["schedulerOnly"])
                    for key, value in audited.items():
                        totals[key] += value
                else:
                    assert step["engine"] == expected_opponent_engine
                    assert step.get("proofWorkersLaunched", 0) == 0
                    assert step.get("proofParallelJobs", 0) == 0
            if library.fc_has_five(board, x, y, side):
                winner = side
        if game["termination"] in ("max-moves-draw", "board-full"):
            assert game["winner"] == 0
        else:
            assert game["winner"] in (-1, 1)
            if winner != 0:
                assert winner == game["winner"]
    totals.update({"result": str(path), "resultSha256": sha256(path),
                   "openingInc": str(opening_inc),
                   "openingIncSha256": sha256(opening_inc),
                   "forbiddenBlack": forbidden, "status": "pass"})
    return totals


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--free", type=Path, required=True)
    parser.add_argument("--forbidden", type=Path, required=True)
    parser.add_argument("--opening-inc", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--opponent", choices=("four-star", "legacy"),
                        default="four-star",
                        help="opponent engine encoded in the two input cells")
    parser.add_argument("--profile-version", default=VERSION,
                        help="expected candidate profile version")
    args = parser.parse_args()
    library = configure_library(args.library)
    cells = [validate_cell(library, args.free, args.opening_inc, False,
                           args.opponent, args.profile_version),
             validate_cell(library, args.forbidden, args.opening_inc, True,
                           args.opponent, args.profile_version)]
    result = {"schemaVersion": 1, "status": "pass", "games": 200,
              "moves": sum(cell["moves"] for cell in cells),
              "randomSelections": sum(cell["randomSelections"] for cell in cells),
              "certificateRecordsAudited": sum(cell["certificates"] for cell in cells),
              "cells": cells, "ruleReferenceLibrary": str(args.library),
              "ruleReferenceLibrarySha256": sha256(args.library),
        "note": "natural-prefix/no-swap formal-opening diagnostic; no RIF exchange metadata"}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
    print(f"replay: pass; profile={args.profile_version}; "
          f"games={result['games']} moves={result['moves']}")


if __name__ == "__main__":
    main()
