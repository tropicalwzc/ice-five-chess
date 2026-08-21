#!/usr/bin/env python3
"""Replay an early micro-VCF paired match and independently reprove losses."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path


BOARD_SIZE = 15
FC_PROOF_PROVEN_WIN = 1
FC_PROOF_SEARCH_VCF = 1
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


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def configure_library(path: Path):
    lib = ctypes.CDLL(str(path))
    lib.fc_is_legal_move.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_int,
        ctypes.c_int, ctypes.c_bool,
    ]
    lib.fc_is_legal_move.restype = ctypes.c_bool
    lib.fc_has_five.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_int, ctypes.c_int,
    ]
    lib.fc_has_five.restype = ctypes.c_bool
    lib.fc_prove_forced_win.argtypes = [
        ctypes.POINTER(BoardRow), ctypes.c_int, ctypes.c_bool,
        ctypes.c_int, ctypes.c_int, ctypes.c_uint64, ctypes.c_uint32,
        ctypes.c_size_t, ctypes.POINTER(FCProofResult),
    ]
    lib.fc_prove_forced_win.restype = ctypes.c_bool
    return lib


def board_bytes(board: Board) -> bytes:
    return ctypes.string_at(ctypes.byref(board), ctypes.sizeof(board))


def copy_board(board: Board) -> Board:
    copied = Board()
    ctypes.memmove(ctypes.byref(copied), ctypes.byref(board),
                   ctypes.sizeof(board))
    return copied


def replay_proof(lib, board: Board, attacker: int, forbidden_black: bool,
                 search_class: int, depth: int, recorded_nodes: int) -> dict:
    before = board_bytes(board)
    result = FCProofResult()
    # Runtime node telemetry can be session-local (and therefore tiny after
    # reuse). Give the independent, cold serial replay enough deterministic
    # room without a wall-clock cutoff.
    budget = max(5_000_000, recorded_nodes * 256 + 65_536)
    proven = lib.fc_prove_forced_win(
        board, attacker, forbidden_black, search_class, max(depth, 1), budget, 0,
        65_536, ctypes.byref(result))
    assert board_bytes(board) == before
    assert proven and result.status == FC_PROOF_PROVEN_WIN, {
        "attacker": attacker,
        "searchClass": search_class,
        "depth": depth,
        "status": result.status,
        "nodes": result.nodes,
        "budgetExhausted": bool(result.budgetExhausted),
    }
    assert result.certificateVerified and result.certificateNodeCount > 0
    return {
        "distance": result.distance,
        "nodes": result.nodes,
        "certificateId": f"0x{result.certificateId:016x}",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--schedule", type=Path)
    parser.add_argument("--jsonl", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    schedule = (json.loads(args.schedule.read_text())
                if args.schedule is not None else None)
    rows = [json.loads(line) for line in args.jsonl.read_text().splitlines()
            if line]
    headers = [row for row in rows if row.get("type") == "header"]
    games = [row for row in rows if row.get("type") == "game"]
    assert len(headers) == 1
    header = headers[0]
    assert len(games) == header["games"] == header["openingCount"] * 2
    assert header["schemaVersion"] == 5
    if schedule is not None:
        assert header["masterSeed"].lower() == schedule["masterSeed"].lower()
        assert header["games"] == schedule["expectedGames"]
    assert header["randomMode"] == "deterministic-best"
    assert isinstance(header["forbiddenBlack"], bool)
    forbidden_black = header["forbiddenBlack"]
    assert header["newProfile"]["version"] == \
        "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a"
    assert header["opponentProfile"] in {
        "five-star-incremental-dfpn-candidate@"
        "5.4.1-transactional-deadline-root-parallel-5s",
        "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book",
        "legacy-three-star@5224020",
    }

    if schedule is not None:
        by_opening = {item["sourceOpeningId"]: item
                      for item in schedule["openings"]}
        source_opening_ids = schedule["sourceOpeningIds"]
    else:
        by_opening = {}
        source_opening_ids = range(
            header["openingStart"],
            header["openingStart"] + header["openingCount"])
        for game in games:
            prefix_length = len(game["moves"]) - len(game["steps"])
            assert prefix_length >= 0
            prefix = game["moves"][:prefix_length]
            opening_id = game["openingId"]
            if opening_id in by_opening:
                assert by_opening[opening_id]["moves"] == prefix
            else:
                by_opening[opening_id] = {"moves": prefix}
    expected = {(opening_id, color)
                for opening_id in source_opening_ids
                for color in (1, -1)}
    assert {(game["openingId"], game["newColor"])
            for game in games} == expected

    lib = configure_library(args.library.resolve())
    early_proofs = guard_vcf_proofs = guard_vct_proofs = 0
    runtime_verified_analysis_proofs = 0
    moves_replayed = candidate_decisions = control_decisions = 0
    max_decision = {"ms": 0.0}
    over_5000 = []
    anomalies = []

    for game in games:
        prefix = by_opening[game["openingId"]]["moves"]
        assert game["moves"][:len(prefix)] == prefix
        assert len(game["steps"]) == len(game["moves"]) - len(prefix)
        if game["anomaly"] is not None:
            anomalies.append({"openingId": game["openingId"],
                              "newColor": game["newColor"],
                              "anomaly": game["anomaly"]})
        board = Board()
        winner = 0
        step_index = 0
        for ply, (x, y, side) in enumerate(game["moves"]):
            assert winner == 0
            assert side == (1 if ply % 2 == 0 else -1)
            assert lib.fc_is_legal_move(
                board, x, y, side, forbidden_black)
            if ply >= len(prefix):
                step = game["steps"][step_index]
                step_index += 1
                assert (step["x"], step["y"], step["side"]) == (x, y, side)
                is_candidate = side == game["newColor"]
                assert (step["engine"] == "new") == is_candidate
                candidate_decisions += int(is_candidate)
                control_decisions += int(not is_candidate)
                if step["ms"] > max_decision["ms"]:
                    max_decision = {
                        "openingId": game["openingId"],
                        "newColor": game["newColor"],
                        "ply": ply,
                        "side": side,
                        "engine": step["engine"],
                        "move": [x, y],
                        "ms": step["ms"],
                    }
                if step["ms"] > 5000.0:
                    over_5000.append(max_decision.copy())

                if step["proofStatus"] == FC_PROOF_PROVEN_WIN:
                    assert step["proofCertificateVerified"]
                    assert step["proofCertificateId"] != \
                        "0x0000000000000000"
                    # The engine has already independently replayed this
                    # certificate before publishing the flag. JSONL stores
                    # the ID but not the certificate nodes or a historical
                    # provisional root, so a fresh search from the final
                    # selected coordinate is not an exact certificate replay.
                    runtime_verified_analysis_proofs += 1

                if is_candidate:
                    early = step["earlyVCF"]
                    if early["status"] == FC_PROOF_PROVEN_WIN:
                        assert early["certificateVerified"]
                        after = copy_board(board)
                        px, py = early["provisionalX"], early["provisionalY"]
                        assert lib.fc_is_legal_move(
                            after, px, py, side, forbidden_black)
                        after[px][py] = side
                        replay_proof(
                            lib, after, -side, forbidden_black,
                            FC_PROOF_SEARCH_VCF,
                            early["effectiveDepth"], early["nodes"])
                        early_proofs += 1

                    guard = step["opponentGuard"]
                    if guard["vcfStatus"] == FC_PROOF_PROVEN_WIN:
                        assert guard["vcfCertificateVerified"]
                        after = copy_board(board)
                        sx, sy = guard["selectedX"], guard["selectedY"]
                        assert (sx, sy) == (x, y)
                        after[sx][sy] = side
                        replay_proof(
                            lib, after, -side, forbidden_black,
                            FC_PROOF_SEARCH_VCF, 9,
                            guard["vcfNodes"])
                        guard_vcf_proofs += 1
                    if guard["vctStatus"] == FC_PROOF_PROVEN_WIN:
                        assert guard["vctCertificateVerified"]
                        after = copy_board(board)
                        sx, sy = guard["selectedX"], guard["selectedY"]
                        assert (sx, sy) == (x, y)
                        after[sx][sy] = side
                        replay_proof(
                            lib, after, -side, forbidden_black, 2, 10,
                            guard["vctNodes"])
                        guard_vct_proofs += 1
            board[x][y] = side
            moves_replayed += 1
            if lib.fc_has_five(board, x, y, side):
                winner = side
        assert game["moveCount"] == len(game["moves"])
        if game["termination"] == "five-in-a-row":
            assert winner == game["winner"]
        elif game["termination"] == "proven-loss":
            # The runner stops before appending a move when the side to move
            # returns a verified proven-loss result.  The raw JSONL therefore
            # has no terminal step, but its winner must be the opposite of
            # the side that would move next, and no earlier move may already
            # have completed five.
            next_side = 1 if len(game["moves"]) % 2 == 0 else -1
            assert winner == 0
            assert game["winner"] == -next_side
        elif game["termination"] == "legacy-illegal-move-loss":
            # The legacy engine proposed a forbidden black move. The runner
            # rejects it before appending a step, so the board must remain
            # non-terminal and the legal opponent wins immediately.
            next_side = 1 if len(game["moves"]) % 2 == 0 else -1
            assert forbidden_black and next_side == 1
            assert winner == 0
            assert game["winner"] == -next_side
        else:
            assert game["winner"] == 0

    result = {
        "schemaVersion": 1,
        "status": "pass",
        "gamesReplayed": len(games),
        "movesReplayed": moves_replayed,
        "candidateDecisions": candidate_decisions,
        "controlDecisions": control_decisions,
        "certificatesReproved": {
            "earlyVCF": early_proofs,
            "finalGuardVCF": guard_vcf_proofs,
            "finalGuardVCT": guard_vct_proofs,
            "total": early_proofs + guard_vcf_proofs + guard_vct_proofs,
        },
        "runtimeVerifiedAnalysisCertificates":
            runtime_verified_analysis_proofs,
        "boardIntegrity": True,
        "moveLegality": True,
        "winnerReplay": True,
        "anomalies": anomalies,
        "decisionsAbove5000Ms": over_5000,
        "maxDecision": max_decision,
        "scheduleSha256": (sha256(args.schedule)
                           if args.schedule is not None else None),
        "rawJsonlSha256": sha256(args.jsonl),
        "referenceLibrarySha256": sha256(args.library),
    }
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(
        "early-vcf paired replay: pass; "
        f"games={len(games)} certificates={result['certificatesReproved']['total']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
