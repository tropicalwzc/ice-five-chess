#!/usr/bin/env python3
"""Verify deterministic benchmark decisions while allowing wall-clock jitter."""

from __future__ import annotations

import copy
import json
import sys
from pathlib import Path
from typing import Any


def normalized(path: Path) -> list[dict[str, Any]]:
    records = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line]
    result = copy.deepcopy(records)
    for record in result:
        if record.get("type") != "game":
            continue
        for step in record.get("steps", []):
            step.pop("ms", None)
    return result


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} FIRST.jsonl SECOND.jsonl", file=sys.stderr)
        return 2
    first = normalized(Path(sys.argv[1]))
    second = normalized(Path(sys.argv[2]))
    if first != second:
        print("benchmark decisions differ", file=sys.stderr)
        return 1
    games = sum(record.get("type") == "game" for record in first)
    print(f"reproducible: {games} games; moves/results/nodes/depth/cache/budgets match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
