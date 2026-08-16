#!/usr/bin/env python3
"""Merge non-overlapping benchmark JSONL shards into one validated schedule."""

import argparse
import collections
import json
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, action="append", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--opening-count", type=int, required=True)
    args = parser.parse_args()
    headers = []
    games = []
    for path in args.input:
        for line in path.read_text().splitlines():
            row = json.loads(line)
            (headers if row["type"] == "header" else games).append(row)
    assert len(headers) == len(args.input)
    comparable = ("schemaVersion", "suite", "masterSeed", "forbiddenBlack",
                  "openingMode", "eliteCorpusVersion", "newProfile",
                  "opponentProfile", "randomMode", "strategy", "maxMoves")
    reference = headers[0]
    for header in headers[1:]:
        assert all(header[key] == reference[key] for key in comparable)
    deduplicated = {}
    for game in games:
        key = (game["openingId"], game["newColor"])
        if key in deduplicated:
            retained = deduplicated[key]
            assert (game["winner"], game["termination"], game["moves"], game["anomaly"]) == (
                retained["winner"], retained["termination"], retained["moves"], retained["anomaly"])
            continue
        deduplicated[key] = game
    games = list(deduplicated.values())
    expected = collections.Counter((opening, color)
                                   for opening in range(args.opening_count)
                                   for color in (1, -1))
    actual = collections.Counter((game["openingId"], game["newColor"])
                                 for game in games)
    assert actual == expected
    assert all(game["anomaly"] is None for game in games)
    merged_header = dict(reference)
    merged_header["openingStart"] = 0
    merged_header["openingCount"] = args.opening_count
    merged_header["games"] = args.opening_count * 2
    merged_header["mergedShardCount"] = len(headers)
    games.sort(key=lambda game: (game["openingId"], 0 if game["newColor"] == 1 else 1))
    with args.output.open("w") as output:
        output.write(json.dumps(merged_header, ensure_ascii=False, separators=(",", ":")) + "\n")
        for game in games:
            output.write(json.dumps(game, ensure_ascii=False, separators=(",", ":")) + "\n")
    print(f"merged {len(games)} games from {len(headers)} shards")


if __name__ == "__main__":
    main()
