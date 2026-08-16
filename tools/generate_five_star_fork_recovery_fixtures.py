#!/usr/bin/env python3
"""Extract the audited 5.7 fallback decisions into immutable fixtures.

The benchmark JSONL contains the complete move list and one step record for
each post-opening decision.  A fallback step therefore identifies one exact
board snapshot: all moves before the step, with the step's side to move.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


EXPECTED_VERSION = (
    "5.7.0-white-v541-black-v521-independent-root-parallel8-5s"
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_records(path: Path) -> tuple[dict, list[tuple[int, dict]]]:
    records = []
    for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), start=1):
        if line.strip():
            records.append((line_number, json.loads(line)))
    headers = [record for _, record in records if record.get("type") == "header"]
    games = [(line, record) for line, record in records
             if record.get("type") == "game"]
    if len(headers) != 1 or len(games) != 100:
        raise ValueError(f"{path}: expected one header and 100 games")
    if headers[0].get("newProfile", {}).get("version") != EXPECTED_VERSION:
        raise ValueError(f"{path}: unexpected profile version")
    return headers[0], games


def extract(path: Path, forbidden: bool, source_root: Path) -> tuple[dict, list[dict]]:
    header, games = read_records(path)
    source_hash = sha256(path)
    source_name = str(path.relative_to(source_root)) 
    entries: list[dict] = []
    for line_number, game in games:
        moves = game["moves"]
        steps = game.get("steps", [])
        prefix_length = len(moves) - len(steps)
        if prefix_length < 0:
            raise ValueError(f"{path}:{line_number}: steps exceed moves")
        for step_index, step in enumerate(steps):
            if step.get("engine") != "new" or not step.get("fallbackUsed"):
                continue
            move_index = prefix_length + step_index
            if move_index >= len(moves):
                raise ValueError(f"{path}:{line_number}: fallback move missing")
            move = moves[move_index]
            if [step["x"], step["y"], step["side"]] != move:
                raise ValueError(f"{path}:{line_number}: step/move mismatch")
            stones = [list(item) for item in moves[:move_index]]
            fixture_id = (
                f"{('forbidden' if forbidden else 'free')}-"
                f"opening-{int(game['openingId']):03d}-"
                f"color-{int(game['newColor']):+d}-ply-{move_index:03d}"
            )
            entries.append({
                "id": fixture_id,
                "version": "five-star-fork-recovery-fixtures-v1",
                "opening": int(game["openingId"]),
                "game": {
                    "openingId": int(game["openingId"]),
                    "newColor": int(game["newColor"]),
                    "winner": int(game["winner"]),
                    "termination": game["termination"],
                },
                "ply": move_index,
                "stepIndex": step_index,
                "sideToMove": int(step["side"]),
                "forbiddenBlack": forbidden,
                "seed": int(step["decisionSeed"], 0),
                "stones": stones,
                "legacyHint": [int(step["hintX"]), int(step["hintY"])],
                "historicalFallback": [int(step["x"]), int(step["y"])],
                "historicalMetadata": {
                    "fourStar": [int(step["fourStarX"]), int(step["fourStarY"])],
                    "default": [int(step["defaultX"]), int(step["defaultY"])],
                    "defaultSource": int(step["defaultSource"]),
                    "decisionStatus": int(step["decisionStatus"]),
                    "lossReason": int(step["lossReason"]),
                    "candidateCount": int(step["candidateCount"]),
                    "candidateCoverageComplete": bool(
                        step["candidateCoverageComplete"]
                    ),
                    "decisionLedgerVersion": int(
                        step["decisionLedgerVersion"]
                    ),
                    "decisionSeed": step["decisionSeed"],
                    "proofWorkerCap": int(step["proofWorkerCap"]),
                    "proofWorkersLaunched": int(step["proofWorkersLaunched"]),
                    "proofParallelJobs": int(step["proofParallelJobs"]),
                },
                "provenance": {
                    "sourceFile": source_name,
                    "sourceLogSha256": source_hash,
                    "sourceLine": line_number,
                    "sourceStep": step_index,
                },
            })
    return header, entries


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--free", type=Path, required=True)
    parser.add_argument("--forbidden", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source_root = args.free.parents[2]
    free_header, free_entries = extract(args.free, False, source_root)
    forbidden_header, forbidden_entries = extract(
        args.forbidden, True, source_root
    )
    entries = free_entries + forbidden_entries
    if len(free_entries) != 49 or len(forbidden_entries) != 48:
        raise ValueError(
            f"expected 49 free and 48 forbidden fallbacks, got "
            f"{len(free_entries)} and {len(forbidden_entries)}"
        )
    ids = [entry["id"] for entry in entries]
    if len(set(ids)) != len(ids):
        raise ValueError("fixture identifiers are not unique")
    manifest = {
        "schemaVersion": 1,
        "fixtureVersion": "five-star-fork-recovery-fixtures-v1",
        "profileVersion": EXPECTED_VERSION,
        "fixtureCount": len(entries),
        "counts": {"free": len(free_entries), "forbidden": len(forbidden_entries)},
        "sources": [
            {
                "ruleMode": False,
                "path": str(args.free),
                "sha256": sha256(args.free),
                "header": free_header,
            },
            {
                "ruleMode": True,
                "path": str(args.forbidden),
                "sha256": sha256(args.forbidden),
                "header": forbidden_header,
            },
        ],
        "entries": entries,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"wrote {len(entries)} fixtures: free={len(free_entries)} "
          f"forbidden={len(forbidden_entries)}")


if __name__ == "__main__":
    main()
