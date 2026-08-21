#!/usr/bin/env python3
"""Fail closed if opponent-guard diagnostics leak into runtime or smoke."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "openspec/changes/strengthen-five-star-opponent-forcing-defense/evidence/diagnostic_manifest.json"
RUNTIME_POLICY = (
    ROOT / "ice five chess/FiveChessAI.c",
    ROOT / "ice five chess/FiveChessAI.h",
    ROOT / "ice five chess/FiveChessEliteCorpus.inc",
    ROOT / "ice five chess/doublethree.m",
)


def board_hash(moves: list[list[int]]) -> str:
    board = [[0] * 15 for _ in range(15)]
    for x, y, side in moves:
        if board[x][y] != 0:
            raise ValueError("duplicate diagnostic move")
        board[x][y] = side
    payload = bytes(value + 1 for row in board for value in row)
    return hashlib.sha256(payload).hexdigest()


def transform_point(transform: int, x: int, y: int) -> tuple[int, int]:
    if transform >= 4:
        x = 14 - x
    for _ in range(transform & 3):
        x, y = y, 14 - x
    return x, y


def transformed_hashes(moves: list[list[int]]) -> set[str]:
    values = set()
    for transform in range(8):
        transformed = [
            [*transform_point(transform, x, y), side]
            for x, y, side in moves
        ]
        values.add(board_hash(transformed))
    return values


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--schedule", type=Path)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    assert manifest["classification"] == "diagnostic-only-not-runtime-policy-or-smoke-input"
    fixtures = manifest["fixtures"]
    hashes: set[str] = set()
    fixture_ids = {fixture["id"] for fixture in fixtures}
    separating = next(f for f in fixtures if f["id"] == "separating-opening13-ply18")
    assert board_hash(separating["movesBefore"]) == separating["boardSha256"]
    hashes |= transformed_hashes(separating["movesBefore"])
    opening20 = [
        [9,10,1],[6,8,-1],[5,7,1],[8,4,-1],[9,4,1],[8,7,-1],
        [8,6,1],[7,8,-1],[9,6,1],[9,8,-1],[5,8,1],[7,6,-1],
        [10,9,1],[7,7,-1],[7,5,1],[9,7,-1],[10,7,1],[5,4,-1],
        [6,5,1],[7,9,-1],[7,10,1],[8,8,-1],[10,8,1],[6,10,-1],
    ]
    for fixture in fixtures:
        prefix_length = fixture.get("prefixLength")
        if prefix_length is None:
            continue
        moves = opening20[:prefix_length]
        assert board_hash(moves) == fixture["boardSha256"]
        hashes |= transformed_hashes(moves)

    runtime = "\n".join(path.read_text(errors="replace") for path in RUNTIME_POLICY)
    for fixture_id in fixture_ids:
        assert fixture_id not in runtime, f"diagnostic fixture leaked: {fixture_id}"
    for digest in hashes:
        assert digest not in runtime, f"diagnostic board hash leaked: {digest}"

    if args.schedule:
        schedule = json.loads(args.schedule.read_text())
        schedule_text = json.dumps(schedule, sort_keys=True, separators=(",", ":"))
        for fixture_id in fixture_ids:
            assert fixture_id not in schedule_text
        scheduled_hashes = set(schedule.get("positionHashes", []))
        overlap = hashes & scheduled_hashes
        assert not overlap, f"diagnostic/smoke overlap: {sorted(overlap)}"
        prior_ids = set(manifest["losingOpeningIds"])
        assert not prior_ids.intersection(schedule.get("sourceOpeningIds", []))

    print(json.dumps({
        "status": "pass",
        "fixtures": len(fixtures),
        "diagnosticSymmetryHashes": len(hashes),
        "runtimeFilesAudited": len(RUNTIME_POLICY),
        "scheduleAudited": bool(args.schedule),
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
