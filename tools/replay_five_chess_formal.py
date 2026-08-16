#!/usr/bin/env python3
"""Independently replay formal games and regenerated proof metadata."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path


BOARD_SIZE = 15
BoardRow = ctypes.c_int * BOARD_SIZE
Board = BoardRow * BOARD_SIZE
PROOF_FIELDS = (
    "proofStatus", "proofSearchClass", "proofDistance",
    "proofCertificateId", "proofCertificateVerified",
    "opponentAfterSelectedStatus", "opponentAfterSelectedDistance",
    "defaultSource", "defaultX", "defaultY", "overrideReason",
    "fourStarX", "fourStarY", "escapeStage",
    "escapeAlternativesExamined", "escapeScopedDisproofCount",
    "escapeUnknownCount", "escapeVerifiedLossCount", "lossReason",
    "corpusProtectionApplied", "quietThreatSelected",
    "corpusLookup", "corpusPositionIndex", "corpusCandidateCount",
    "corpusMatchType", "corpusTrustTier", "corpusRequiredStones",
    "corpusSourceBoardMask", "corpusAccepted", "corpusReason",
    "corpusSupportGames", "corpusSupportEvents", "corpusSupportSources",
    "corpusAcceptedCandidateCount",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_jsonl(path: Path) -> tuple[dict, list[dict]]:
    records = [json.loads(line) for line in path.read_text().splitlines()
               if line.strip()]
    headers = [record for record in records if record.get("type") == "header"]
    games = [record for record in records if record.get("type") == "game"]
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


def expected_prefix(schedule: dict, opening_id: int) -> list[list[int]]:
    entry = schedule["entries"][opening_id]
    return [[x + 7, y + 7, 1 if ply % 2 == 0 else -1]
            for ply, (x, y) in enumerate(entry["scheduledMovesRelative"])]


def validate_header(header: dict, schedule: dict, forbidden: bool) -> None:
    assert header["schemaVersion"] == 5
    assert header["suite"] == "five-star-natural-final"
    assert header["openingMode"] == "gomocup-held-out-prefix"
    assert header["masterSeed"].lower() == schedule["masterSeed"].lower()
    assert header["forbiddenBlack"] is forbidden
    assert header["openingCount"] == 50 and header["games"] == 100
    profile = header["newProfile"]
    assert profile["version"] == "5.4.1-transactional-deadline-root-parallel-5s"
    assert profile["proofWorkerCount"] == 8
    assert profile["decisionTimeBudgetMs"] == 4500
    assert profile["proofParallelPeakMemoryBytes"] == 85983232


def validate_game(library, game: dict, schedule: dict,
                  forbidden: bool) -> tuple[int, int]:
    opening_id = game["openingId"]
    prefix = expected_prefix(schedule, opening_id)
    assert game["moves"][:len(prefix)] == prefix
    assert game["moveCount"] == len(game["moves"])
    assert len(game["moves"]) - len(prefix) == len(game["steps"])
    assert game["anomaly"] is None
    board = Board()
    winner = 0
    for ply, move in enumerate(game["moves"]):
        x, y, side = move
        assert winner == 0
        assert side == (1 if ply % 2 == 0 else -1)
        assert library.fc_is_legal_move(board, x, y, side, forbidden)
        board[x][y] = side
        if library.fc_has_five(board, x, y, side):
            winner = side
    for offset, step in enumerate(game["steps"]):
        x, y, side = game["moves"][len(prefix) + offset]
        assert (step["x"], step["y"], step["side"]) == (x, y, side)
        expected_engine = "new" if side == game["newColor"] else step["engine"]
        if side == game["newColor"]:
            assert expected_engine == step["engine"] == "new"
            assert step["ms"] <= 5000.0
            if step["proofStatus"] == 1:
                assert step["proofCertificateVerified"]
                assert step["proofCertificateId"] != "0x0000000000000000"
            if step["proofCertificateVerified"]:
                assert step["proofCertificateId"] != "0x0000000000000000"
    if game["termination"] == "five-in-a-row":
        assert winner == game["winner"]
    elif game["termination"] in ("max-moves-draw", "board-full"):
        assert game["winner"] == 0
    else:
        assert game["winner"] in (-1, 1)
    return len(game["moves"]), sum(
        step["proofCertificateVerified"] for step in game["steps"]
        if step["engine"] == "new")


def compare_regeneration(original: dict, regenerated: dict) -> int:
    for field in ("openingId", "newColor", "winner", "termination",
                  "moveCount", "moves", "anomaly"):
        assert original[field] == regenerated[field]
    assert len(original["steps"]) == len(regenerated["steps"])
    replayed_certificates = 0
    for first, second in zip(original["steps"], regenerated["steps"]):
        for field in ("x", "y", "side", "engine"):
            assert first[field] == second[field]
        if first["engine"] != "new":
            continue
        for field in PROOF_FIELDS:
            assert first.get(field) == second.get(field), field
        if first["proofCertificateVerified"]:
            replayed_certificates += 1
    return replayed_certificates


def replay_cell(library, original_path: Path, regenerated_path: Path,
                schedule_path: Path, forbidden: bool) -> dict:
    schedule = json.loads(schedule_path.read_text())
    first_header, first_games = read_jsonl(original_path)
    second_header, second_games = read_jsonl(regenerated_path)
    validate_header(first_header, schedule, forbidden)
    validate_header(second_header, schedule, forbidden)
    for field in ("schemaVersion", "suite", "masterSeed", "forbiddenBlack",
                  "openingMode", "newProfile", "opponentProfile",
                  "randomMode", "strategy", "maxMoves"):
        assert first_header[field] == second_header[field]
    first_by_key = {(game["openingId"], game["newColor"]): game
                    for game in first_games}
    second_by_key = {(game["openingId"], game["newColor"]): game
                     for game in second_games}
    expected = {(opening, color) for opening in range(50) for color in (1, -1)}
    assert set(first_by_key) == set(second_by_key) == expected
    moves = 0
    certificates = 0
    regenerated_certificates = 0
    for key in sorted(expected):
        move_count, verified = validate_game(
            library, first_by_key[key], schedule, forbidden)
        validate_game(library, second_by_key[key], schedule, forbidden)
        moves += move_count
        certificates += verified
        regenerated_certificates += compare_regeneration(
            first_by_key[key], second_by_key[key])
    assert certificates == regenerated_certificates
    return {
        "original": str(original_path),
        "originalSha256": sha256(original_path),
        "regenerated": str(regenerated_path),
        "regeneratedSha256": sha256(regenerated_path),
        "schedule": str(schedule_path),
        "scheduleSha256": sha256(schedule_path),
        "forbiddenBlack": forbidden,
        "games": 100,
        "movesReplayedWithCReference": moves,
        "certificatesRegeneratedAndVerified": certificates,
        "status": "pass",
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--cell", action="append", nargs=4, required=True,
                        metavar=("ORIGINAL", "REGENERATED", "SCHEDULE", "FORBIDDEN"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    library = configure_library(args.library.resolve())
    cells = [replay_cell(library, Path(original), Path(regenerated),
                         Path(schedule), bool(int(forbidden)))
             for original, regenerated, schedule, forbidden in args.cell]
    result = {
        "schemaVersion": 1,
        "status": "pass",
        "ruleReferenceLibrary": str(args.library),
        "ruleReferenceLibrarySha256": sha256(args.library),
        "gamesReplayed": sum(cell["games"] for cell in cells),
        "movesReplayedWithCReference": sum(
            cell["movesReplayedWithCReference"] for cell in cells),
        "certificatesRegeneratedAndVerified": sum(
            cell["certificatesRegeneratedAndVerified"] for cell in cells),
        "cells": cells,
    }
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(f"formal replay: pass; games={result['gamesReplayed']} "
          f"certificates={result['certificatesRegeneratedAndVerified']}")


if __name__ == "__main__":
    main()
