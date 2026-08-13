#!/usr/bin/env python3
"""Freeze 100 unique symmetry-canonical held-out Gomocup opening prefixes."""

from __future__ import annotations

import argparse
import hashlib
import json
import zipfile
from pathlib import Path


MEMBER = "openings/openings_freestyle15.txt"
MASTER_SEED = "0xc0dec0de20260813"


def transform(point: tuple[int, int], variant: int) -> tuple[int, int]:
    x, y = point[0] + 7, point[1] + 7
    n = 14
    tx, ty = (
        (x, y), (y, n - x), (n - x, n - y), (n - y, x),
        (n - x, y), (x, n - y), (y, x), (n - y, n - x),
    )[variant]
    return tx - 7, ty - 7


def canonical(moves: list[tuple[int, int]]) -> tuple[tuple[int, int], ...]:
    return min(tuple(transform(move, variant) for move in moves)
               for variant in range(8))


def identity(moves: tuple[tuple[int, int], ...]) -> str:
    payload = " ".join(f"{x},{y}" for x, y in moves)
    return hashlib.sha256(payload.encode()).hexdigest()[:20]


def parse_package(path: Path) -> tuple[int, str, list[list[tuple[int, int]]]]:
    year = 2024 if "2024" in path.name else 2025
    package_hash = hashlib.sha256(path.read_bytes()).hexdigest()
    with zipfile.ZipFile(path) as archive:
        lines = []
        for raw in archive.read(MEMBER).decode("utf-8-sig").splitlines():
            values = [int(value) for value in raw.split(",")]
            lines.append(list(zip(values[0::2], values[1::2])))
    return year, package_hash, lines


def excluded_identities(paths: list[Path]) -> set[str]:
    excluded: set[str] = set()
    for path in paths:
        for raw in path.read_text(encoding="utf-8").splitlines():
            record = json.loads(raw)
            if record.get("type") != "game":
                continue
            prefix_length = len(record["moves"]) - len(record["steps"])
            prefix = [(move[0] - 7, move[1] - 7)
                      for move in record["moves"][:prefix_length]]
            excluded.add(identity(canonical(prefix)))
    return excluded


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", required=True, action="append", type=Path)
    parser.add_argument("--exclude-jsonl", action="append", default=[], type=Path)
    parser.add_argument("--asset", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()

    excluded = excluded_identities(args.exclude_jsonl)
    candidates: dict[str, dict[str, object]] = {}
    packages: list[dict[str, object]] = []
    for package in args.package:
        year, package_hash, lines = parse_package(package)
        packages.append({"year": year, "sha256": package_hash,
                         "member": MEMBER})
        for line_index, line in enumerate(lines):
            for prefix_length in range(1, len(line)):
                normalized = canonical(line[:prefix_length])
                canonical_id = identity(normalized)
                if canonical_id in excluded:
                    continue
                entry = candidates.setdefault(canonical_id, {
                    "canonicalId": canonical_id,
                    "canonicalMovesRelative": normalized,
                    "provenance": [],
                })
                entry["provenance"].append({
                    "year": year, "openingIndex": line_index,
                    "prefixLength": prefix_length,
                })
    if len(candidates) < 100:
        raise ValueError(f"only {len(candidates)} held-out canonical prefixes remain")

    def selection_key(entry: dict[str, object]) -> str:
        return hashlib.sha256(
            f"{MASTER_SEED}:{entry['canonicalId']}".encode()
        ).hexdigest()

    selected = sorted(candidates.values(), key=selection_key)[:100]
    rendered = [
        "/* Generated held-out schedule: 100 unique canonical Gomocup prefixes. */",
        "typedef struct {",
        "    unsigned char length;",
        "    signed char coordinates[50];",
        "    unsigned short sourceYear;",
        "    unsigned char sourceIndex;",
        "    unsigned char prefixLength;",
        "} FCBenchmarkOpening;",
        "",
        "static const FCBenchmarkOpening fcFormalOpenings[100] = {",
    ]
    manifest_entries: list[dict[str, object]] = []
    for opening_id, entry in enumerate(selected):
        moves = list(entry["canonicalMovesRelative"])
        variant = int(hashlib.sha256(
            f"transform:{MASTER_SEED}:{opening_id}".encode()
        ).hexdigest()[:2], 16) & 7
        scheduled = [transform(move, variant) for move in moves]
        provenance = entry["provenance"][0]
        values = ", ".join(f"{value:2d}" for move in scheduled for value in move)
        rendered.append(
            f"    {{{len(scheduled)}, {{{values}}}, {provenance['year']}, "
            f"{provenance['openingIndex']}, {provenance['prefixLength']}}},"
        )
        manifest_entries.append({
            "openingId": opening_id,
            "canonicalId": entry["canonicalId"],
            "scheduledTransform": variant,
            "scheduledMovesRelative": scheduled,
            "provenance": entry["provenance"],
        })
    rendered.extend(["};", ""])
    args.asset.write_text("\n".join(rendered), encoding="utf-8")
    manifest = {
        "schemaVersion": 1,
        "seedDomain": "proof-final-proof-v2",
        "masterSeed": MASTER_SEED,
        "identityCount": len(manifest_entries),
        "uniqueCanonicalIdentityCount": len({e["canonicalId"] for e in manifest_entries}),
        "excludedDiagnosticCanonicalIds": sorted(excluded),
        "sourcePackages": packages,
        "selection": "SHA-256 ordering of held-out canonical prefixes after diagnostic exclusion",
        "entries": manifest_entries,
    }
    args.manifest.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    print(f"selected {len(selected)} of {len(candidates)} held-out canonical prefixes; excluded={len(excluded)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
