#!/usr/bin/env python3
"""Replay seeded-stochastic hybrid games without requiring identical reruns."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path


BOARD_SIZE = 15
MASK64 = (1 << 64) - 1
BoardRow = ctypes.c_int * BOARD_SIZE
Board = BoardRow * BOARD_SIZE
HYBRID_VERSION = "5.5.1-white-proof-engine-black-v51-stochastic-5s"
BLACK_COMPONENT = (1, "black-production-v51",
                   "5.1.0-elite-rule-partitioned-local-v2")
WHITE_COMPONENT = (2, "white-proof-engine",
                   "5.4.1-transactional-deadline-root-parallel-5s")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_u64(value: str | int) -> int:
    return int(value, 0) if isinstance(value, str) else value


def mix64(value: int) -> int:
    value = (value + 0x9E3779B97F4A7C15) & MASK64
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK64
    return (value ^ (value >> 31)) & MASK64


def decision_seed(master: int, opening_id: int, ply: int,
                  side: int, new_engine: bool) -> int:
    value = master ^ ((opening_id + 1) << 32)
    value ^= ((ply + 1) * 0x9E3779B97F4A7C15) & MASK64
    value ^= 0x424C41434B if side == 1 else 0x5748495445
    value ^= 0x4E4557 if new_engine else 0x4C4547414359
    return mix64(value & MASK64)


def load_jsonl(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    if len(headers) != 1 or len(games) != 100:
        raise AssertionError(f"{path}: expected one header and 100 games")
    return headers[0], games


def configure_library(path: Path):
    library = ctypes.CDLL(str(path))
    library.fc_is_legal_move.argtypes = [ctypes.POINTER(BoardRow), ctypes.c_int,
                                         ctypes.c_int, ctypes.c_int,
                                         ctypes.c_bool]
    library.fc_is_legal_move.restype = ctypes.c_bool
    library.fc_has_five.argtypes = [ctypes.POINTER(BoardRow), ctypes.c_int,
                                    ctypes.c_int, ctypes.c_int]
    library.fc_has_five.restype = ctypes.c_bool
    return library


def schedule_entries(schedule: dict) -> dict[int, dict]:
    entries = schedule["entries"]
    return {int(entry.get("openingId", index)): entry
            for index, entry in enumerate(entries)}


def expected_prefix(entries: dict[int, dict], opening_id: int) -> list[list[int]]:
    entry = entries[opening_id]
    relative = entry.get("scheduledMovesRelative",
                         entry.get("canonicalMovesRelative"))
    if relative is None:
        raise AssertionError(f"opening {opening_id}: missing scheduled moves")
    return [[int(move[0]) + 7, int(move[1]) + 7,
             1 if ply % 2 == 0 else -1]
            for ply, move in enumerate(relative)]


def validate_header(header: dict, schedule: dict, forbidden: bool) -> int:
    assert header["schemaVersion"] >= 5
    assert header["newProfile"]["version"] == HYBRID_VERSION
    assert header["randomMode"] == "user-softmax"
    assert header["forbiddenBlack"] is forbidden
    assert header["openingCount"] == 50 and header["games"] == 100
    master = parse_u64(header["masterSeed"])
    assert master == parse_u64(schedule["masterSeed"])
    return master


def validate_new_step(step: dict, side: int, master: int,
                      opening_id: int, ply: int) -> dict[str, int]:
    assert step["engine"] == "new"
    expected = WHITE_COMPONENT if side == -1 else BLACK_COMPONENT
    assert (step["hybridComponent"], step["hybridComponentName"],
            step["hybridComponentVersion"]) == expected
    assert step["randomMode"] == 1
    assert parse_u64(step["decisionSeed"]) == decision_seed(
        master, opening_id, ply, side, True)
    assert step["randomEligibilityVerified"] is True
    assert parse_u64(step["randomEquivalenceSignature"]) != 0
    assert step["randomCandidateCount"] >= 1
    assert 0 <= step["randomSelectedRank"] < step["randomCandidateCount"]
    if step["randomSelectionUsed"]:
        assert step["randomCandidateCount"] > 1
    if step["randomCandidateCount"] == 1:
        assert step["randomSelectedRank"] == 0
        assert step["randomSelectionUsed"] is False
    assert step["ms"] <= 5000.0
    assert step.get("cpuMs", 0.0) >= 0.0
    assert step.get("peakResidentBytes", 0) >= 0
    if side == 1:
        assert step.get("proofWorkersLaunched", 0) == 0
    else:
        # This counter is cumulative across all parallel proof batches in one
        # decision, not the peak number of simultaneously active workers.
        # Each batch is capped by the frozen eight-worker profile; cumulatively
        # launched workers cannot exceed the number of scheduled root jobs.
        workers = step.get("proofWorkersLaunched", 0)
        jobs = step.get("proofParallelJobs", 0)
        completed_jobs = step.get("proofParallelJobsCompleted", 0)
        assert 0 <= completed_jobs <= jobs
        assert 0 <= workers <= jobs
    if step["proofStatus"] == 1:
        assert step["proofCertificateVerified"] is True
        assert step["proofCertificateId"] != "0x0000000000000000"
        assert step["proofNumber"] == 0
    if step["proofCertificateVerified"]:
        assert step["proofCertificateId"] != "0x0000000000000000"
    if step["proofStatus"] == 2:
        assert step["disproofNumber"] == 0
    return {
        "randomSelections": int(step["randomSelectionUsed"]),
        "budgetExhaustions": int(step["budgetExhausted"]),
        "certificates": int(step["proofCertificateVerified"]),
        "whiteSteps": int(side == -1),
        "blackSteps": int(side == 1),
    }


def validate_game(library, game: dict, entries: dict[int, dict],
                  forbidden: bool, master: int) -> dict[str, int]:
    opening_id = int(game["openingId"])
    prefix = expected_prefix(entries, opening_id)
    assert game["moves"][:len(prefix)] == prefix
    assert game["moveCount"] == len(game["moves"])
    assert len(game["moves"]) - len(prefix) == len(game["steps"])
    assert game["anomaly"] is None
    board = Board()
    winner = 0
    totals = {"randomSelections": 0, "budgetExhaustions": 0,
              "certificates": 0, "whiteSteps": 0, "blackSteps": 0}
    for ply, move in enumerate(game["moves"]):
        x, y, side = move
        assert winner == 0
        assert side == (1 if ply % 2 == 0 else -1)
        assert library.fc_is_legal_move(board, x, y, side, forbidden)
        board[x][y] = side
        if ply >= len(prefix):
            step = game["steps"][ply - len(prefix)]
            assert (step["x"], step["y"], step["side"]) == (x, y, side)
            if side == game["newColor"]:
                audited = validate_new_step(
                    step, side, master, opening_id, ply)
                for key, value in audited.items():
                    totals[key] += value
        if library.fc_has_five(board, x, y, side):
            winner = side
    if game["termination"] == "five-in-a-row":
        assert winner == game["winner"]
    elif game["termination"] in ("max-moves-draw", "board-full"):
        assert game["winner"] == 0
    else:
        assert game["winner"] in (-1, 1)
    totals["moves"] = len(game["moves"])
    return totals


def replay_cell(library, result_path: Path, schedule_path: Path,
                forbidden: bool, opponent: str) -> dict:
    header, games = load_jsonl(result_path)
    schedule = json.loads(schedule_path.read_text(encoding="utf-8"))
    entries = schedule_entries(schedule)
    master = validate_header(header, schedule, forbidden)
    expected_ids = {(opening, color)
                    for opening in sorted(entries)[:50] for color in (1, -1)}
    actual_ids = {(int(game["openingId"]), int(game["newColor"]))
                  for game in games}
    assert actual_ids == expected_ids
    totals = {"moves": 0, "randomSelections": 0, "budgetExhaustions": 0,
              "certificates": 0, "whiteSteps": 0, "blackSteps": 0}
    for game in games:
        audited = validate_game(
            library, game, entries, forbidden, master)
        for key, value in audited.items():
            totals[key] += value
    return {
        "result": str(result_path), "resultSha256": sha256(result_path),
        "schedule": str(schedule_path), "scheduleSha256": sha256(schedule_path),
        "forbiddenBlack": forbidden, "opponent": opponent,
        "games": 100, **totals, "status": "pass",
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--cell", action="append", nargs=4, required=True,
                        metavar=("RESULT", "SCHEDULE", "FORBIDDEN", "OPPONENT"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    library = configure_library(args.library.resolve())
    cells = [replay_cell(library, Path(result), Path(schedule),
                         bool(int(forbidden)), opponent)
             for result, schedule, forbidden, opponent in args.cell]
    summary = {
        "schemaVersion": 1, "status": "pass",
        "replayPolicy": "recorded-path correctness; no separately-seeded exact-move requirement",
        "ruleReferenceLibrary": str(args.library.resolve()),
        "ruleReferenceLibrarySha256": sha256(args.library),
        "games": sum(cell["games"] for cell in cells),
        "moves": sum(cell["moves"] for cell in cells),
        "randomSelections": sum(cell["randomSelections"] for cell in cells),
        "budgetExhaustions": sum(cell["budgetExhaustions"] for cell in cells),
        "certificateRecordsAudited": sum(cell["certificates"] for cell in cells),
        "cells": cells,
    }
    args.output.write_text(json.dumps(summary, indent=2) + "\n",
                           encoding="utf-8")
    print(f"stochastic replay: pass; games={summary['games']} "
          f"moves={summary['moves']} random={summary['randomSelections']}")


if __name__ == "__main__":
    main()
