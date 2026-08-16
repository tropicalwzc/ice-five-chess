#!/usr/bin/env python3
"""Audit the recorded overlap-aware five-star games."""

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
VERSION = "5.6.2-white-v541-black-v521-overlap-aware-parallel8-5s"
COMPONENTS = {
    -1: (2, "white-proof-engine",
         "5.4.1-transactional-deadline-root-parallel-5s"),
    1: (3, "black-v521-parallel-root-8w",
        "5.2.3-overlap-aware-deeper-root-parallel-8w-5s"),
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


def prefix_for(prefixes: list[list[list[int]]], opening_id: int) -> list[list[int]]:
    assert 0 <= opening_id < len(prefixes)
    return prefixes[opening_id]


def validate_step(step: dict, side: int) -> dict[str, int]:
    component = COMPONENTS[side]
    assert step["engine"] == "new"
    assert (step["hybridComponent"], step["hybridComponentName"],
            step["hybridComponentVersion"]) == component
    assert step["randomMode"] == 1
    assert step["randomEligibilityVerified"] is True
    assert parse_u64(step["randomEquivalenceSignature"]) != 0
    assert step["randomCandidateCount"] >= 1
    assert 0 <= step["randomSelectedRank"] < step["randomCandidateCount"]
    if step["randomSelectionUsed"]:
        assert step["randomCandidateCount"] > 1
    if step["randomCandidateCount"] == 1:
        assert step["randomSelectionUsed"] is False
    assert float(step["ms"]) <= 5000.0
    assert float(step.get("cpuMs", 0.0)) >= 0.0
    assert int(step.get("proofWorkerCap", 0)) == 8
    jobs = int(step.get("proofParallelJobs", 0))
    completed = int(step.get("proofParallelJobsCompleted", 0))
    workers = int(step.get("proofWorkersLaunched", 0))
    assert 0 <= completed <= jobs and 0 <= workers <= jobs
    diagnostics = step["diagnostics"]
    assert diagnostics["parallelOverlapGroups"] >= 0
    assert diagnostics["parallelGroupedRootJobs"] >= diagnostics["parallelRootJobs"]
    assert diagnostics["parallelRootJobsCompleted"] <= diagnostics["parallelRootJobs"]
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
        "overlapPairs": int(diagnostics["parallelOverlapPairs"]),
        "overlapGroups": int(diagnostics["parallelOverlapGroups"]),
        "largestOverlapGroup": int(diagnostics["parallelLargestOverlapGroup"]),
        "budgetTokens": int(diagnostics["parallelBudgetTokens"]),
    }


def validate_cell(library, path: Path, schedule_path: Path,
                  forbidden: bool) -> dict:
    header, games = load(path)
    prefixes = opening_prefixes(schedule_path)
    assert header["schemaVersion"] >= 5
    assert header["suite"] == "five-star-parallel-v521-final"
    assert header["newProfile"]["version"] == VERSION
    assert header["newProfile"]["parallelProofEnabled"] is True
    assert header["newProfile"]["proofWorkerCount"] == 8
    assert header["forbiddenBlack"] is forbidden
    assert header["randomMode"] == "user-softmax"
    assert header["openingStart"] == 0 and header["openingCount"] == 50
    assert header["games"] == 100
    expected_ids = {(opening, color) for opening in range(50)
                    for color in (1, -1)}
    actual_ids = {(int(game["openingId"]), int(game["newColor"]))
                  for game in games}
    assert actual_ids == expected_ids
    totals = {"games": 100, "moves": 0, "randomSelections": 0,
              "budgetExhaustions": 0, "certificates": 0, "workers": 0,
              "jobs": 0, "jobsCompleted": 0, "overlapPairs": 0,
              "overlapGroups": 0, "largestOverlapGroup": 0,
              "budgetTokens": 0}
    for game in games:
        opening_id = int(game["openingId"])
        prefix = prefix_for(prefixes, opening_id)
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
                    audited = validate_step(step, side)
                    for key, value in audited.items():
                        totals[key] += value
                else:
                    assert step["engine"] == "four-star"
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
                   "schedule": str(schedule_path),
                   "scheduleSha256": sha256(schedule_path),
                   "scheduleMasterSeed": None,
                   "resultMasterSeed": header["masterSeed"],
                   "forbiddenBlack": forbidden, "status": "pass"})
    return totals


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--free", type=Path, required=True)
    parser.add_argument("--forbidden", type=Path, required=True)
    parser.add_argument("--opening-inc", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    library = configure_library(args.library)
    cells = [validate_cell(library, args.free, args.opening_inc, False),
             validate_cell(library, args.forbidden, args.opening_inc, True)]
    result = {"schemaVersion": 1, "status": "pass", "games": 200,
              "moves": sum(cell["moves"] for cell in cells),
              "randomSelections": sum(cell["randomSelections"] for cell in cells),
              "certificateRecordsAudited": sum(cell["certificates"] for cell in cells),
              "cells": cells, "ruleReferenceLibrary": str(args.library),
              "ruleReferenceLibrarySha256": sha256(args.library),
              "note": "fresh decision seeds; opening identities are the frozen official-prefix schedule"}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
    print(f"overlap replay: pass; games={result['games']} moves={result['moves']}")


if __name__ == "__main__":
    main()
