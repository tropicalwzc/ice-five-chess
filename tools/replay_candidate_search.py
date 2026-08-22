#!/usr/bin/env python3
"""Replay and validate black-only random-candidate search cells."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from replay_double_three_small_match import (  # noqa: E402
    Board,
    FC_PROOF_PROVEN_WIN,
    FC_PROOF_SEARCH_VCF,
    configure_library,
    copy_board,
    replay_proof,
)


def _assert(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def _certificate_checks(step: dict) -> int:
    claims = 0
    if step.get("proofStatus") == FC_PROOF_PROVEN_WIN:
        _assert(step.get("proofCertificateVerified") is True,
                "unverified own proof certificate")
        _assert(step.get("proofCertificateId") != "0x0000000000000000",
                "empty own proof certificate")
        claims += 1
    guard = step.get("opponentGuard", {})
    for status_key, verified_key in (
        ("vcfStatus", "vcfCertificateVerified"),
        ("vctStatus", "vctCertificateVerified"),
    ):
        if guard.get(status_key) == FC_PROOF_PROVEN_WIN:
            _assert(guard.get(verified_key) is True,
                    f"unverified guard {status_key} certificate")
            claims += 1
    early = step.get("earlyVCF", {})
    if early.get("status") == FC_PROOF_PROVEN_WIN:
        _assert(early.get("certificateVerified") is True,
                "unverified early VCF certificate")
        claims += 1
    return claims


def replay_search_cell(path: Path,
                       library,
                       expected_opening_ids: list[int],
                       expected_mutation: str,
                       expected_node_id: str,
                       expected_parent_node_id: str | None,
                       expected_forbidden_black: bool,
                       replay_certificates: bool = False,
                       expected_suite: str =
                       "five-star-candidate-search") -> dict:
    rows = [json.loads(line) for line in path.read_text().splitlines()
            if line.strip()]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    _assert(len(headers) == 1, f"{path}: expected one header")
    header = headers[0]
    _assert(header.get("schemaVersion") == 6,
            f"{path}: expected search schema 6")
    _assert(header.get("suite") == expected_suite,
            f"{path}: wrong suite")
    _assert(header.get("randomMode") == "deterministic-best",
            f"{path}: wrong random mode")
    _assert(header.get("blackOnly") is True, f"{path}: not black-only")
    _assert(bool(header.get("forbiddenBlack")) == expected_forbidden_black,
            f"{path}: rule mode mismatch")
    actual_ids = header.get("openingIds")
    _assert(actual_ids == expected_opening_ids,
            f"{path}: opening sample mismatch")
    _assert(header.get("games") == len(expected_opening_ids),
            f"{path}: game count header mismatch")
    _assert(len(games) == len(expected_opening_ids),
            f"{path}: game count mismatch")
    _assert(header.get("mutation") == expected_mutation,
            f"{path}: mutation mismatch")
    _assert(header.get("nodeId") == expected_node_id,
            f"{path}: node id mismatch")
    if expected_parent_node_id is not None:
        _assert(header.get("parentNodeId") == expected_parent_node_id,
                f"{path}: parent node mismatch")
    _assert(header.get("opponentProfile") ==
            "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book",
            f"{path}: opponent is not frozen four-star")
    opponent_snapshot = header.get("opponentProfileSnapshot")
    _assert(isinstance(opponent_snapshot, dict),
            f"{path}: missing frozen opponent snapshot")
    _assert(opponent_snapshot.get("name") == "four-star-frozen-control" and
            opponent_snapshot.get("version") ==
            "4.0.0-frozen-vcf-vct-control",
            f"{path}: frozen opponent snapshot mismatch")

    expected_id_set = set(expected_opening_ids)
    actual_id_set = {game.get("openingId") for game in games}
    _assert([game.get("openingId") for game in games] == expected_opening_ids,
            f"{path}: game opening order mismatch")
    _assert(actual_id_set == expected_id_set,
            f"{path}: game opening IDs are not unique")

    profile = header.get("newProfile", {})
    if expected_mutation == "none":
        _assert(profile.get("version") ==
                "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a",
                f"{path}: incumbent profile is not exact 5.8.1")
    else:
        _assert(profile.get("name") == "five-star-random-candidate",
                f"{path}: candidate profile name mismatch")
        _assert(profile.get("version") == expected_mutation,
                f"{path}: candidate profile mutation mismatch")

    profile_hard_limit = int(
        header.get("newProfile", {}).get("decisionHardLimitMs", 5000))
    anomalies = []
    hard_limit_violations = 0
    certificate_claims = 0
    moves_replayed = 0
    outcome = {"wins": 0, "draws": 0, "losses": 0, "games": len(games)}
    latencies: list[float] = []
    for game in games:
        _assert(game.get("newColor") == -1,
                f"{path}: game is not candidate-black")
        if game.get("anomaly") is not None:
            anomalies.append({"openingId": game.get("openingId"),
                              "anomaly": game.get("anomaly")})
        moves = game.get("moves", [])
        steps = game.get("steps", [])
        _assert(game.get("moveCount") == len(moves),
                f"{path}: move count mismatch")
        _assert(len(moves) <= int(header.get("maxMoves", 120)),
                f"{path}: move limit exceeded")
        prefix_length = len(moves) - len(steps)
        _assert(prefix_length >= 0, f"{path}: invalid opening prefix")
        board = Board()
        winner = 0
        step_index = 0
        for ply, move in enumerate(moves):
            x, y, side = move
            _assert(winner == 0, f"{path}: moves continue after winner")
            _assert(side == (1 if ply % 2 == 0 else -1),
                    f"{path}: side alternation mismatch")
            _assert(library.fc_is_legal_move(
                board, x, y, side, expected_forbidden_black),
                    f"{path}: illegal move {x},{y},{side}")
            if ply >= prefix_length:
                _assert(step_index < len(steps),
                        f"{path}: missing step telemetry")
                step = steps[step_index]
                step_index += 1
                _assert((step.get("x"), step.get("y"), step.get("side")) ==
                        (x, y, side), f"{path}: step coordinate mismatch")
                expected_engine = "new" if side == -1 else "four-star"
                _assert(step.get("engine") == expected_engine,
                        f"{path}: wrong engine for side {side}")
                elapsed = float(step.get("ms", 0.0))
                latencies.append(elapsed)
                if elapsed > profile_hard_limit:
                    hard_limit_violations += 1
                certificate_claims += _certificate_checks(step)
                if replay_certificates and side == -1:
                    guard = step.get("opponentGuard", {})
                    if guard.get("vcfStatus") == FC_PROOF_PROVEN_WIN:
                        after = copy_board(board)
                        after[x][y] = side
                        replay_proof(
                            library, after, -side, FC_PROOF_SEARCH_VCF, 9,
                            int(guard.get("vcfNodes", 0)),
                            expected_forbidden_black)
                    early = step.get("earlyVCF", {})
                    if early.get("status") == FC_PROOF_PROVEN_WIN:
                        after = copy_board(board)
                        px, py = early["provisionalX"], early["provisionalY"]
                        _assert(library.fc_is_legal_move(
                            after, px, py, side, expected_forbidden_black),
                                f"{path}: early VCF move is illegal")
                        after[px][py] = side
                        replay_proof(
                            library, after, -side, FC_PROOF_SEARCH_VCF,
                            int(early.get("effectiveDepth", 1)),
                            int(early.get("nodes", 0)),
                            expected_forbidden_black)
            board[x][y] = side
            moves_replayed += 1
            if library.fc_has_five(board, x, y, side):
                winner = side
        _assert(step_index == len(steps),
                f"{path}: step count does not match moves")
        replay_winner = winner
        _assert(game.get("winner") == replay_winner,
                f"{path}: replay winner differs from recorded winner")
        if replay_winner == -1:
            outcome["wins"] += 1
        elif replay_winner == 0:
            outcome["draws"] += 1
        else:
            outcome["losses"] += 1

    _assert(not anomalies, f"{path}: anomalies {anomalies}")
    ordered_latencies = sorted(latencies)
    p95_index = (min(len(ordered_latencies) - 1,
                     max(0, math.ceil(len(ordered_latencies) * 0.95) - 1))
                 if ordered_latencies else 0)
    snapshot_digest = hashlib.sha256(
        json.dumps(opponent_snapshot, sort_keys=True,
                   separators=(",", ":")).encode("utf-8")).hexdigest()
    return {
        "path": str(path),
        "status": "pass" if not anomalies and hard_limit_violations == 0
        else "invalid",
        "openingIds": expected_opening_ids,
        "games": outcome,
        "score": outcome["wins"] + 0.5 * outcome["draws"],
        "scoreRate": (outcome["wins"] + 0.5 * outcome["draws"])
        / max(outcome["games"], 1),
        "latency": {
            "p50Ms": sorted(latencies)[len(latencies) // 2]
            if latencies else 0.0,
            "p95Ms": ordered_latencies[p95_index]
            if ordered_latencies else 0.0,
            "maxMs": max(latencies) if latencies else 0.0,
            "meanMs": sum(latencies) / len(latencies)
            if latencies else 0.0,
        },
        "anomalyCount": len(anomalies),
        "hardLimitViolations": hard_limit_violations,
        "certificateClaims": certificate_claims,
        "movesReplayed": moves_replayed,
        "rawSchemaVersion": header["schemaVersion"],
        "opponentProfileSnapshot": opponent_snapshot,
        "opponentSnapshotSha256": snapshot_digest,
        "openingPoolVersion": header.get("openingPoolVersion"),
        "benchmarkIdentity": header.get("benchmarkIdentity"),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--jsonl", type=Path, required=True)
    parser.add_argument("--opening-ids", required=True)
    parser.add_argument("--mutation", default="none")
    parser.add_argument("--node-id", required=True)
    parser.add_argument("--parent-node-id")
    parser.add_argument("--suite", default="five-star-candidate-search")
    parser.add_argument("--forbidden-black", type=int, choices=(0, 1),
                        required=True)
    parser.add_argument("--replay-certificates", action="store_true")
    args = parser.parse_args()
    ids = [int(value) for value in args.opening_ids.split(",") if value]
    library = configure_library(args.library)
    result = replay_search_cell(
        args.jsonl, library, ids, args.mutation, args.node_id,
        args.parent_node_id, bool(args.forbidden_black),
        args.replay_certificates, expected_suite=args.suite)
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "pass" else 3


if __name__ == "__main__":
    raise SystemExit(main())
