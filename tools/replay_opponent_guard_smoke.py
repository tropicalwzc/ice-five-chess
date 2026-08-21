#!/usr/bin/env python3
"""Independently replay the paired 48-game opponent-guard smoke dataset."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path


BOARD_SIZE = 15
BoardRow = ctypes.c_int * BOARD_SIZE
Board = BoardRow * BOARD_SIZE
GUARD_FIELDS = (
    "eligible", "skipReason", "provisionalX", "provisionalY",
    "provisionalClass", "selectedX", "selectedY", "selectedClass",
    "vcfStatus", "vcfDistance", "vcfCertificateVerified", "vctStatus",
    "vctDistance", "vctEligible", "vctCertificateVerified",
    "auditedStages", "auditedCount", "completedDisproofs", "unknowns",
    "verifiedLosses", "avoidedVerifiedLoss", "rollback",
)
DECISION_FIELDS = (
    "x", "y", "side", "engine", "tacticalClass", "decisionStatus",
    "proofStatus", "proofSearchClass", "proofDistance",
    "proofCertificateId", "proofCertificateVerified",
    "opponentAfterSelectedStatus", "opponentAfterSelectedDistance",
    "fourStarX", "fourStarY", "handoffReason", "corpusAccepted",
    "corpusReason", "corpusPositionIndex",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_jsonl(path: Path) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in path.read_text().splitlines() if line]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1 and len(games) == 24, path
    return headers[0], games


def configure_library(path: Path):
    lib = ctypes.CDLL(str(path))
    lib.fc_is_legal_move.argtypes = [ctypes.POINTER(BoardRow), ctypes.c_int,
                                     ctypes.c_int, ctypes.c_int, ctypes.c_bool]
    lib.fc_is_legal_move.restype = ctypes.c_bool
    lib.fc_has_five.argtypes = [ctypes.POINTER(BoardRow), ctypes.c_int,
                                ctypes.c_int, ctypes.c_int]
    lib.fc_has_five.restype = ctypes.c_bool
    int_pointer = ctypes.POINTER(ctypes.c_int)
    bool_pointer = ctypes.POINTER(ctypes.c_bool)
    lib.fc_replay_opponent_guard_probe.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_bool,
        ctypes.c_int, ctypes.c_int, int_pointer, int_pointer,
        int_pointer, int_pointer, bool_pointer, int_pointer, int_pointer,
        bool_pointer, bool_pointer,
    ]
    lib.fc_replay_opponent_guard_probe.restype = ctypes.c_int
    return lib


def probe_guard(lib, board, side: int, x: int, y: int) -> dict:
    values = [ctypes.c_int() for _ in range(6)]
    vcf_verified = ctypes.c_bool()
    vct_verified = ctypes.c_bool()
    restored = ctypes.c_bool()
    ok = lib.fc_replay_opponent_guard_probe(
        board, side, False, x, y,
        ctypes.byref(values[0]), ctypes.byref(values[1]),
        ctypes.byref(values[2]), ctypes.byref(values[3]),
        ctypes.byref(vcf_verified), ctypes.byref(values[4]),
        ctypes.byref(values[5]), ctypes.byref(vct_verified),
        ctypes.byref(restored))
    assert ok and restored.value
    return {"class": values[0].value, "searchClass": values[1].value,
            "vcfStatus": values[2].value, "vcfDistance": values[3].value,
            "vcfVerified": vcf_verified.value,
            "vctStatus": values[4].value, "vctDistance": values[5].value,
            "vctVerified": vct_verified.value}


def validate_header(header: dict, schedule: dict, candidate: bool) -> None:
    assert header["schemaVersion"] == 5
    assert header["suite"] == "opponent-guard-smoke"
    assert header["masterSeed"].lower() == schedule["masterSeed"].lower()
    assert header["openingStart"] == 60 and header["openingCount"] == 12
    assert header["games"] == 24 and header["forbiddenBlack"] is False
    assert header["randomMode"] == "deterministic-best"
    assert header["opponentProfile"].startswith("four-star-")
    profile = header["newProfile"]
    if candidate:
        assert profile["version"] == "5.8.0-vcf-first-opponent-guard-4w"
        assert profile["opponentGuardEnabled"] is True
        assert profile["proofWorkerCount"] == 4
        assert profile["decisionHardLimitMs"] == 5000
    else:
        assert profile["version"] == "5.4.1-transactional-deadline-root-parallel-5s"
        assert profile["opponentGuardEnabled"] is False


def validate_game(lib, game: dict, schedule_by_id: dict,
                  candidate: bool, diagnose: bool = False) -> tuple[int, int, dict | None]:
    opening = schedule_by_id[game["openingId"]]
    prefix = opening["moves"]
    assert game["moves"][:len(prefix)] == prefix
    assert game["moveCount"] == len(game["moves"])
    assert len(game["steps"]) == len(game["moves"]) - len(prefix)
    assert game["anomaly"] is None
    board = Board()
    winner = 0
    accepted_guard_evidence = 0
    incident = None
    step_index = 0
    for ply, (x, y, side) in enumerate(game["moves"]):
        assert winner == 0 and side == (1 if ply % 2 == 0 else -1)
        assert lib.fc_is_legal_move(board, x, y, side, False)
        if ply >= len(prefix):
            step = game["steps"][step_index]
            step_index += 1
            assert (step["x"], step["y"], step["side"]) == (x, y, side)
            if side == game["newColor"]:
                assert step["engine"] == "new" and step["ms"] <= 5000.0
                guard = step["opponentGuard"]
                if candidate and guard["eligible"] and guard["selectedClass"] in (1, 3):
                    replayed = probe_guard(lib, board, side, x, y)
                    assert replayed["class"] == guard["selectedClass"], (
                        game["openingId"], ply, [x, y],
                        guard["selectedClass"], replayed)
                    if guard["selectedClass"] == 1:
                        assert replayed["vcfVerified"] or replayed["vctVerified"]
                    else:
                        assert replayed["vcfStatus"] == 2
                        if guard["vctStatus"] == 2:
                            assert replayed["vctStatus"] == 2
                    accepted_guard_evidence += 1
                if step["proofStatus"] == 1:
                    assert step["proofCertificateVerified"]
                    assert step["proofCertificateId"] != "0x0000000000000000"
                if (diagnose and incident is None and game["newColor"] == 1
                        and game["winner"] == -1):
                    selected_probe = probe_guard(lib, board, side, x, y)
                    if selected_probe["class"] == 1:
                        alternative = None
                        for ax in range(BOARD_SIZE):
                            for ay in range(BOARD_SIZE):
                                if (ax, ay) == (x, y) or not lib.fc_is_legal_move(
                                        board, ax, ay, side, False):
                                    continue
                                alternative_probe = probe_guard(
                                    lib, board, side, ax, ay)
                                if alternative_probe["class"] == 3:
                                    alternative = {"move": [ax, ay],
                                                   "audit": alternative_probe}
                                    break
                            if alternative is not None:
                                break
                        incident = {
                            "openingId": game["openingId"],
                            "ply": ply,
                            "boardMovesBefore": game["moves"][:ply],
                            "selectedMove": [x, y],
                            "selectedAudit": selected_probe,
                            "completedDefensiveAlternative": alternative,
                            "runtimeGuard": step["opponentGuard"],
                        }
        board[x][y] = side
        if lib.fc_has_five(board, x, y, side):
            winner = side
    if game["termination"] == "five-in-a-row":
        assert winner == game["winner"]
    elif game["termination"] in ("max-moves-draw", "board-full"):
        assert game["winner"] == 0
    else:
        assert game["winner"] in (-1, 1)
    return len(game["moves"]), accepted_guard_evidence, incident


def compare_regeneration(original: dict, regenerated: dict) -> list[dict]:
    for field in ("openingId", "newColor", "winner", "termination",
                  "moveCount", "moves", "anomaly"):
        assert original[field] == regenerated[field], field
    assert len(original["steps"]) == len(regenerated["steps"])
    mismatches = []
    for index, (first, second) in enumerate(zip(original["steps"], regenerated["steps"])):
        for field in DECISION_FIELDS:
            if first.get(field) != second.get(field):
                mismatches.append({"openingId": original["openingId"],
                                   "newColor": original["newColor"],
                                   "step": index, "move": [first["x"], first["y"]],
                                   "field": field, "original": first.get(field),
                                   "regenerated": second.get(field),
                                   "originalMs": first["ms"],
                                   "regeneratedMs": second["ms"]})
        for field in GUARD_FIELDS:
            left = first["opponentGuard"].get(field)
            right = second["opponentGuard"].get(field)
            if left != right:
                mismatches.append({"openingId": original["openingId"],
                                   "newColor": original["newColor"],
                                   "step": index, "move": [first["x"], first["y"]],
                                   "field": "guard." + field,
                                   "original": left, "regenerated": right,
                                   "originalMs": first["ms"],
                                   "regeneratedMs": second["ms"]})
    return mismatches


def replay_pair(lib, original: Path, regenerated: Path, schedule: dict,
                candidate: bool) -> dict:
    header, games = load_jsonl(original)
    regen_header, regen_games = load_jsonl(regenerated)
    validate_header(header, schedule, candidate)
    validate_header(regen_header, schedule, candidate)
    for field in ("suite", "masterSeed", "openingStart", "openingCount",
                  "forbiddenBlack", "randomMode", "strategy", "newProfile",
                  "opponentProfile", "maxMoves"):
        assert header[field] == regen_header[field], field
    by_key = {(game["openingId"], game["newColor"]): game for game in games}
    regen_by_key = {(game["openingId"], game["newColor"]): game for game in regen_games}
    expected = {(opening_id, color) for opening_id in schedule["sourceOpeningIds"]
                for color in (1, -1)}
    assert set(by_key) == set(regen_by_key) == expected
    schedule_by_id = {item["sourceOpeningId"]: item for item in schedule["openings"]}
    moves = evidence = 0
    incidents = []
    determinism_mismatches = []
    for key in sorted(expected):
        count, verified, incident = validate_game(
            lib, by_key[key], schedule_by_id, candidate, True)
        validate_game(lib, regen_by_key[key], schedule_by_id, candidate)
        determinism_mismatches.extend(
            compare_regeneration(by_key[key], regen_by_key[key]))
        moves += count
        evidence += verified
        if incident is not None:
            incidents.append(incident)
    return {"profile": "candidate" if candidate else "control",
            "games": 24, "movesReplayed": moves,
            "acceptedGuardEvidenceReplayed": evidence,
            "deterministicDiscreteResults": not determinism_mismatches,
            "determinismMismatches": determinism_mismatches,
            "lostBlackForcingIncidents": incidents,
            "avoidableLostBlackForcingIncidents": sum(
                item["completedDefensiveAlternative"] is not None
                for item in incidents),
            "originalSha256": sha256(original),
            "regeneratedSha256": sha256(regenerated)}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--schedule", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--control-regenerated", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--candidate-regenerated", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    schedule = json.loads(args.schedule.read_text())
    assert schedule["expectedGames"] == 48 and schedule["rule"] == "freestyle"
    lib = configure_library(args.library.resolve())
    cells = [replay_pair(lib, args.control, args.control_regenerated,
                         schedule, False),
             replay_pair(lib, args.candidate, args.candidate_regenerated,
                         schedule, True)]
    result = {"schemaVersion": 1, "status": "pass", "gamesReplayed": 48,
              "scheduleSha256": sha256(args.schedule),
              "referenceLibrarySha256": sha256(args.library), "cells": cells}
    result["deterministicDiscreteResults"] = all(
        cell["deterministicDiscreteResults"] for cell in cells)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print("opponent-guard smoke replay: pass; games=48")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
