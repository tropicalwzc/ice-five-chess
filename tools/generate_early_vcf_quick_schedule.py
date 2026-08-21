#!/usr/bin/env python3
"""Freeze six fresh paired openings for the early micro-VCF quick test."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


MASK = (1 << 64) - 1
MASTER_SEED = 0x4755415244534D4B
OPENING_IDS = tuple(range(66, 72))


def mix64(value: int) -> int:
    value = (value + 0x9E3779B97F4A7C15) & MASK
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK
    return value ^ (value >> 31)


def random_next(state: int) -> tuple[int, int]:
    state ^= state >> 12
    state ^= (state << 25) & MASK
    state ^= state >> 27
    state &= MASK
    return state, (state * 2685821657736338717) & MASK


def board_hash(moves: list[list[int]]) -> str:
    board = [[0] * 15 for _ in range(15)]
    for x, y, side in moves:
        board[x][y] = side
    return hashlib.sha256(
        bytes(value + 1 for row in board for value in row)
    ).hexdigest()


def generate(opening_id: int) -> list[list[int]]:
    state = mix64(MASTER_SEED ^ (opening_id + 1))
    occupied = {(7, 7)}
    moves = [[7, 7, 1]]
    for ply in range(1, 6):
        side = 1 if ply % 2 == 0 else -1
        legal = []
        for x in range(4, 11):
            for y in range(4, 11):
                if abs(x - 7) + abs(y - 7) > 5 or (x, y) in occupied:
                    continue
                legal.append((x, y))
        state, value = random_next(state)
        x, y = legal[value % len(legal)]
        occupied.add((x, y))
        moves.append([x, y, side])
    return moves


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    openings = []
    for opening_id in OPENING_IDS:
        moves = generate(opening_id)
        openings.append({
            "identity": f"early-vcf-quick-{MASTER_SEED:016x}-{opening_id}",
            "sourceOpeningId": opening_id,
            "moves": moves,
            "positionHash": board_hash(moves),
        })
    schedule = {
        "schemaVersion": 1,
        "classification": "fresh-natural-freestyle-directional-quick-test",
        "masterSeed": f"0x{MASTER_SEED:016x}",
        "sourceOpeningIds": list(OPENING_IDS),
        "excludedOpeningIds": list(range(60, 66)),
        "excludedClasses": [
            "targeted-vcf-fixtures",
            "late-correction-diagnostics",
            "corpus-derived-diagnostics",
            "prior-result-selected-schedules",
        ],
        "positionHashes": [item["positionHash"] for item in openings],
        "openings": openings,
        "candidateProfile":
            "five-star-early-micro-vcf-candidate@"
            "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a",
        "controlProfile":
            "five-star-incremental-dfpn-candidate@"
            "5.4.1-transactional-deadline-root-parallel-5s",
        "colors": [1, -1],
        "expectedGames": 12,
        "randomMode": "deterministic-best",
        "rule": "freestyle",
        "maxDecisionMilliseconds": 5000,
    }
    args.output.write_text(json.dumps(schedule, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
