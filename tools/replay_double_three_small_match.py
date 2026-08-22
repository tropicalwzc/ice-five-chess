#!/usr/bin/env python3
"""Replay and summarize the 5.8.2 double-three paired benchmark cells."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import math
from pathlib import Path
from statistics import mean, median


BOARD_SIZE = 15
FC_PROOF_PROVEN_WIN = 1
FC_PROOF_SEARCH_VCF = 1
FC_PROOF_SEARCH_VCT = 2
FC_DOUBLE_THREE_STATUS_COMPLETE_SAFE = 2
FC_DOUBLE_THREE_STATUS_COMPLETE_UNRESOLVED = 3
FC_DOUBLE_THREE_STATUS_UNKNOWN = 4
FC_DOUBLE_THREE_STATUS_BYPASSED = 5

BoardRow = ctypes.c_int * BOARD_SIZE
Board = BoardRow * BOARD_SIZE


class FCProofNode(ctypes.Structure):
    _fields_ = [
        ("boardHash", ctypes.c_uint64),
        ("initialRelevanceMask", ctypes.c_uint64 * 4),
        ("replyRelatedZoneMask", ctypes.c_uint64 * 4),
        ("relatedZoneMask", ctypes.c_uint64 * 4),
        ("verifiedOmissionMask", ctypes.c_uint64 * 4),
        ("parent", ctypes.c_int),
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("side", ctypes.c_int),
        ("terminalWin", ctypes.c_bool),
    ]


class FCProofResult(ctypes.Structure):
    _fields_ = [
        ("status", ctypes.c_int),
        ("searchClass", ctypes.c_int),
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("distance", ctypes.c_int),
        ("nodes", ctypes.c_uint64),
        ("transpositionHits", ctypes.c_uint64),
        ("proofNumber", ctypes.c_uint64),
        ("disproofNumber", ctypes.c_uint64),
        ("completedDepth", ctypes.c_int),
        ("budgetExhausted", ctypes.c_bool),
        ("certificateVerified", ctypes.c_bool),
        ("certificateOverflow", ctypes.c_bool),
        ("certificateId", ctypes.c_uint64),
        ("certificateNodeCount", ctypes.c_int),
        ("certificate", FCProofNode * 512),
        ("elapsedMilliseconds", ctypes.c_double),
    ]


def configure_library(path: Path):
    library = ctypes.CDLL(str(path.resolve()))
    library.fc_is_legal_move.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_int,
        ctypes.c_int, ctypes.c_bool,
    ]
    library.fc_is_legal_move.restype = ctypes.c_bool
    library.fc_has_five.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_int,
        ctypes.c_int,
    ]
    library.fc_has_five.restype = ctypes.c_bool
    library.fc_prove_forced_win.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_bool,
        ctypes.c_int, ctypes.c_int, ctypes.c_uint64, ctypes.c_uint32,
        ctypes.c_size_t, ctypes.POINTER(FCProofResult),
    ]
    library.fc_prove_forced_win.restype = ctypes.c_bool
    return library


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    if not ordered:
        return 0.0
    index = (math.ceil((len(ordered) - 1) * fraction)
             if fraction == 0.95
             else math.floor((len(ordered) - 1) * fraction))
    return ordered[index]


def latency_summary(steps: list[dict]) -> dict:
    values = [step["ms"] for step in steps]
    return {
        "p50Ms": percentile(values, 0.50),
        "p95Ms": percentile(values, 0.95),
        "maxMs": max(values) if values else 0.0,
        "meanMs": mean(values) if values else 0.0,
    }


def board_bytes(board: Board) -> bytes:
    return ctypes.string_at(ctypes.byref(board), ctypes.sizeof(board))


def copy_board(board: Board) -> Board:
    copied = Board()
    ctypes.memmove(ctypes.byref(copied), ctypes.byref(board),
                   ctypes.sizeof(board))
    return copied


def replay_proof(library, board: Board, attacker: int, search_class: int,
                 depth: int, recorded_nodes: int,
                 forbidden_black: bool) -> dict:
    before = board_bytes(board)
    result = FCProofResult()
    budget = max(5_000_000, recorded_nodes * 256 + 65_536)
    proven = library.fc_prove_forced_win(
        board, attacker, forbidden_black, search_class, max(depth, 1), budget, 0,
        65_536, ctypes.byref(result))
    assert board_bytes(board) == before
    assert proven and result.status == FC_PROOF_PROVEN_WIN
    assert result.certificateVerified
    assert result.certificateNodeCount > 0
    return {
        "distance": result.distance,
        "nodes": result.nodes,
        "certificateId": f"0x{result.certificateId:016x}",
    }


def result_counts(games: list[dict], candidate_color: int) -> dict:
    counts = {"wins": 0, "draws": 0, "losses": 0, "games": 0}
    for game in games:
        counts["games"] += 1
        if game["winner"] == candidate_color:
            counts["wins"] += 1
        elif game["winner"] == 0:
            counts["draws"] += 1
        else:
            counts["losses"] += 1
    return counts


def candidate_result_counts(games: list[dict]) -> dict:
    counts = {"wins": 0, "draws": 0, "losses": 0, "games": 0}
    for game in games:
        counts["games"] += 1
        if game["winner"] == game["newColor"]:
            counts["wins"] += 1
        elif game["winner"] == 0:
            counts["draws"] += 1
        else:
            counts["losses"] += 1
    return counts


def replay_cell(path: Path, library, expected_opponent: str,
                expected_candidate_version: str) -> dict:
    rows = [json.loads(line) for line in path.read_text().splitlines()
            if line.strip()]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1, f"{path}: expected one header"
    header = headers[0]
    assert header["schemaVersion"] == 5
    assert header["suite"] in (
        "five-star-double-three-small",
        "five-star-defense-recovery-standard",
    )
    assert header["randomMode"] == "deterministic-best"
    forbidden_black = bool(header["forbiddenBlack"])
    assert header["maxMoves"] == 120
    assert header["games"] == 24
    assert len(games) == 24
    assert header["newProfile"]["version"] == expected_candidate_version
    assert header["opponentProfile"] == expected_opponent

    expected_keys = {(opening, color)
                     for opening in range(header["openingStart"],
                                          header["openingStart"] +
                                          header["openingCount"])
                     for color in (1, -1)}
    actual_keys = {(game["openingId"], game["newColor"]) for game in games}
    assert actual_keys == expected_keys

    prefixes: dict[int, list[list[int]]] = {}
    moves_replayed = 0
    candidate_steps = {1: [], -1: []}
    integrity_failures: list[str] = []
    anomalies: list[dict] = []
    terminal_failures: list[str] = []
    certificate_replays = {
        "earlyVCF": 0,
        "finalGuardVCF": 0,
        "finalGuardVCT": 0,
        "ownProof": 0,
    }
    for game in games:
        if game["anomaly"] is not None:
            anomalies.append({"openingId": game["openingId"],
                              "newColor": game["newColor"],
                              "anomaly": game["anomaly"]})
        assert game["moveCount"] == len(game["moves"])
        assert len(game["moves"]) <= header["maxMoves"]
        prefix_length = len(game["moves"]) - len(game["steps"])
        assert prefix_length >= 0
        prefix = game["moves"][:prefix_length]
        if game["openingId"] in prefixes:
            assert prefixes[game["openingId"]] == prefix
        else:
            prefixes[game["openingId"]] = prefix

        board = Board()
        winner = 0
        step_index = 0
        for ply, move in enumerate(game["moves"]):
            x, y, side = move
            assert winner == 0
            assert side == (1 if ply % 2 == 0 else -1)
            assert library.fc_is_legal_move(
                board, x, y, side, header["forbiddenBlack"])
            if ply >= prefix_length:
                step = game["steps"][step_index]
                step_index += 1
                assert (step["x"], step["y"], step["side"]) == tuple(move)
                expected_engine = ("new" if side == game["newColor"]
                                   else ("four-star"
                                         if expected_opponent.startswith(
                                             "four-star")
                                         else ("three-star"
                                               if expected_opponent.startswith(
                                                   "three-star")
                                         else "five-star-5.8.1-control"))
                                         )
                assert step["engine"] == expected_engine
                if side == game["newColor"]:
                    candidate_steps[side].append(step)
                    if step["proofStatus"] == FC_PROOF_PROVEN_WIN:
                        assert step["proofCertificateVerified"]
                        assert step["proofCertificateId"] != \
                            "0x0000000000000000"
                    guard = step["opponentGuard"]
                    if guard["vcfStatus"] == FC_PROOF_PROVEN_WIN:
                        assert guard["vcfCertificateVerified"]
                    if guard["vctStatus"] == FC_PROOF_PROVEN_WIN:
                        assert guard["vctCertificateVerified"]
                    early = step["earlyVCF"]
                    if early["status"] == FC_PROOF_PROVEN_WIN:
                        assert early["certificateVerified"]
                        after = copy_board(board)
                        px, py = early["provisionalX"], early["provisionalY"]
                        assert library.fc_is_legal_move(
                            after, px, py, side, header["forbiddenBlack"])
                        after[px][py] = side
                        replay_proof(
                            library, after, -side, FC_PROOF_SEARCH_VCF,
                            early["effectiveDepth"], early["nodes"],
                            forbidden_black)
                        certificate_replays["earlyVCF"] += 1
                    if guard["vcfStatus"] == FC_PROOF_PROVEN_WIN:
                        after = copy_board(board)
                        after[x][y] = side
                        replay_proof(
                            library, after, -side, FC_PROOF_SEARCH_VCF,
                            9, guard["vcfNodes"], forbidden_black)
                        certificate_replays["finalGuardVCF"] += 1
                    if guard["vctStatus"] == FC_PROOF_PROVEN_WIN:
                        after = copy_board(board)
                        after[x][y] = side
                        replay_proof(
                            library, after, -side, 2, 10,
                            guard["vctNodes"], forbidden_black)
                        certificate_replays["finalGuardVCT"] += 1
                    if (step["proofStatus"] == FC_PROOF_PROVEN_WIN and
                            step["tacticalClass"] == 3 and
                            early["status"] != FC_PROOF_PROVEN_WIN and
                            guard["vcfStatus"] != FC_PROOF_PROVEN_WIN and
                            guard["vctStatus"] != FC_PROOF_PROVEN_WIN):
                        replay_proof(
                            library, board, side,
                            step["proofSearchClass"],
                            max(step["depth"], step["proofDistance"], 1),
                            step["proofNodes"], forbidden_black)
                        certificate_replays["ownProof"] += 1
                    double_three = step["doubleThree"]
                    status = double_three["status"]
                    if status == FC_DOUBLE_THREE_STATUS_COMPLETE_SAFE:
                        assert double_three["scanComplete"]
                        assert double_three["selectedResidual"] == 0
                    if status == FC_DOUBLE_THREE_STATUS_COMPLETE_UNRESOLVED:
                        assert double_three["scanComplete"]
                        assert double_three["selectedResidual"] >= 1
                    if status == FC_DOUBLE_THREE_STATUS_UNKNOWN:
                        assert not double_three["structuralOverride"]
                    if double_three["structuralOverride"]:
                        assert status == FC_DOUBLE_THREE_STATUS_COMPLETE_SAFE
                        assert double_three["scanComplete"]
                        assert double_three["selectedResidual"] == 0
                    if side != 1:
                        assert status == 0
                    recovery = step.get("recoverySource")
                    if side == 1 and expected_candidate_version.startswith(
                            "5.8.2-black-defense-recovery"):
                        assert recovery is not None
                        assert step["recoveryFinalCandidateConsistent"]
                        guard_selected = (
                            step["opponentGuard"]["selectedX"],
                            step["opponentGuard"]["selectedY"],
                        )
                        assert guard_selected == (step["x"], step["y"])
                        if step["recoveryForkProbeComplete"]:
                            assert step["recoveryForkRisk"] in (1, 2, 3, 4)
                            assert step["recoveryForkRepliesExamined"] >= 0
                        if step["recoveryVCTEscalatedOnUnknown"]:
                            assert step["opponentGuard"]["vctEligible"]

            board[x][y] = side
            moves_replayed += 1
            if library.fc_has_five(board, x, y, side):
                winner = side
        assert step_index == len(game["steps"])
        if game["termination"] == "five-in-a-row":
            if winner != game["winner"] or winner == 0:
                terminal_failures.append(
                    f"{game['openingId']}/{game['newColor']}: terminal winner")
        elif game["termination"] == "max-moves-draw":
            if winner != 0 or game["winner"] != 0:
                terminal_failures.append(
                    f"{game['openingId']}/{game['newColor']}: draw mismatch")
        elif game["termination"] == "proven-loss":
            next_side = 1 if len(game["moves"]) % 2 == 0 else -1
            if winner != 0 or game["winner"] != -next_side:
                terminal_failures.append(
                    f"{game['openingId']}/{game['newColor']}: loss mismatch")
        else:
            terminal_failures.append(
                f"{game['openingId']}/{game['newColor']}: {game['termination']}")

    assert not anomalies, anomalies
    assert not terminal_failures, terminal_failures
    assert not integrity_failures, integrity_failures
    assert len(prefixes) == header["openingCount"]
    diagnostic_totals: dict[str, int] = {}
    for game in games:
        for step in game["steps"]:
            for name, value in step["diagnostics"].items():
                diagnostic_totals[name] = diagnostic_totals.get(name, 0) + value

    telemetry = {}
    latency = {}
    for color in (1, -1):
        steps = candidate_steps[color]
        black_double_three = [step["doubleThree"] for step in steps
                              if color == 1]
        telemetry[str(color)] = {
            "decisions": len(steps),
            "gainCount": sum(item["gainCount"]
                              for item in black_double_three),
            "positionsWithGains": sum(item["gainCount"] > 0
                                       for item in black_double_three),
            "safeSelections": sum(
                item["status"] == FC_DOUBLE_THREE_STATUS_COMPLETE_SAFE
                for item in black_double_three),
            "unresolvedSelections": sum(
                item["status"] == FC_DOUBLE_THREE_STATUS_COMPLETE_UNRESOLVED
                for item in black_double_three),
            "unknownScans": sum(
                item["status"] == FC_DOUBLE_THREE_STATUS_UNKNOWN
                for item in black_double_three),
            "incompleteScans": sum(
                item["status"] == FC_DOUBLE_THREE_STATUS_UNKNOWN or
                item["scanOverflow"] for item in black_double_three),
            "deadlineAnomalies": sum(item["deadlineAnomaly"]
                                      for item in black_double_three),
            "structuralOverrides": sum(item["structuralOverride"]
                                        for item in black_double_three),
            "rollbacks": sum(item["rollback"]
                              for item in black_double_three),
            "forcingCertificateClaims": sum(
                step["proofStatus"] == FC_PROOF_PROVEN_WIN and
                step["proofCertificateVerified"] for step in steps),
            "immediateBlockSelections": sum(
                step.get("recoverySource") == 5 for step in steps
                if color == 1),
            "forkRiskSelections": sum(
                step.get("recoveryForkRisk") == 3 for step in steps
                if color == 1),
            "forkNoForkCompleted": sum(
                step.get("recoveryForkProbeComplete") and
                step.get("recoveryForkRisk") in (1, 2)
                for step in steps if color == 1),
            "forkIncompleteProbes": sum(
                not step.get("recoveryForkProbeComplete", False) and
                step.get("recoveryForkRepliesExamined", 0) > 0
                for step in steps if color == 1),
            "vctEscalationsOnUnknown": sum(
                step.get("recoveryVCTEscalatedOnUnknown", False)
                for step in steps if color == 1),
            "recoveryConsistencyFailures": sum(
                not step.get("recoveryFinalCandidateConsistent", True)
                for step in steps if color == 1),
        }
        latency[str(color)] = latency_summary(steps)

    black_steps = candidate_steps[1]
    position_latency = {
        "candidateBlackOrdinary": latency_summary(
            [step for step in black_steps
             if step["doubleThree"]["gainCount"] == 0]),
        "candidateBlackDoubleThree": latency_summary(
            [step for step in black_steps
             if step["doubleThree"]["gainCount"] > 0]),
    }

    result = {
        "schemaVersion": 1,
        "status": "pass",
        "input": str(path),
        "rawJsonlSha256": sha256(path),
        "masterSeed": header["masterSeed"],
        "openingIds": sorted(prefixes),
        "openings": [{"openingId": opening, "moves": prefixes[opening]}
                     for opening in sorted(prefixes)],
        "profiles": {
            "candidate": header["newProfile"]["name"] + "@" +
                         header["newProfile"]["version"],
            "opponent": header["opponentProfile"],
        },
        "games": len(games),
        "movesReplayed": moves_replayed,
        "candidateBlack": result_counts(
            [game for game in games if game["newColor"] == 1], 1),
        "candidateWhite": result_counts(
            [game for game in games if game["newColor"] == -1], -1),
        "overall": candidate_result_counts(games),
        "telemetryByCandidateColor": telemetry,
        "latencyByCandidateColor": latency,
        "latencyByBlackPositionType": position_latency,
        "deadlineAnomalyCount": sum(
            value["deadlineAnomalies"] for value in telemetry.values()),
        "incompleteScanCount": sum(
            value["incompleteScans"] for value in telemetry.values()),
        "recoveryIncompleteForkProbeCount": sum(
            value["forkIncompleteProbes"]
            for value in telemetry.values()),
        "replay": {
            "boardIntegrity": True,
            "moveLegality": True,
            "terminalResults": True,
            "certificateReplays": certificate_replays,
            "certificateReplayTotal": sum(certificate_replays.values()),
            "certificateClaimsRuntimeVerified": sum(
                value["forcingCertificateClaims"]
                for value in telemetry.values()),
        },
        "diagnostics": diagnostic_totals,
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--jsonl", action="append", required=True,
                        type=Path)
    parser.add_argument("--opponent", action="append", required=True,
                        choices=("three-star", "four-star", "five-star-5.8.1"))
    parser.add_argument(
        "--candidate-version",
        default="5.8.2-black-double-three-soft-40-16g-32c-80ms",
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if len(args.jsonl) != len(args.opponent):
        raise SystemExit("each --jsonl needs one matching --opponent")

    library = configure_library(args.library)
    expected_profiles = {
        "three-star": "three-star-production@2.2.0-legacy-hint-safe-gate",
        "four-star": "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book",
        "five-star-5.8.1": "five-star-early-micro-vcf@"
                           "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a",
    }
    cells = [replay_cell(path, library, expected_profiles[opponent],
                         args.candidate_version)
             for path, opponent in zip(args.jsonl, args.opponent)]
    assert cells
    if len(cells) > 1:
        assert cells[0]["masterSeed"].lower() == \
            cells[1]["masterSeed"].lower()
        assert cells[0]["openingIds"] == cells[1]["openingIds"]
        assert cells[0]["openings"] == cells[1]["openings"]

    aggregate = {
        "schemaVersion": 1,
        "status": "pass",
        "suite": "five-star-defense-recovery-standard",
        "games": sum(cell["games"] for cell in cells),
        "masterSeed": cells[0]["masterSeed"],
        "openingIds": cells[0]["openingIds"],
        "scheduleMatch": len(cells) == 1 or (
            cells[0]["masterSeed"].lower() == cells[1]["masterSeed"].lower() and
            cells[0]["openings"] == cells[1]["openings"]),
        "cells": cells,
        "sourceHashes": {
            "rawJsonl": [cell["rawJsonlSha256"] for cell in cells],
            "library": sha256(args.library),
        },
    }
    args.output.write_text(json.dumps(aggregate, indent=2) + "\n")
    print(json.dumps({
        "status": aggregate["status"],
        "games": aggregate["games"],
        "scheduleMatch": aggregate["scheduleMatch"],
        "cells": [cell["profiles"]["opponent"] for cell in cells],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
