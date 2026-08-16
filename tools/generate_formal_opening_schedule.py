#!/usr/bin/env python3
"""Freeze short, natural, symmetry-canonical held-out Gomocup prefixes."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path


MEMBERS = {
    "freestyle": (
        "openings/openings_freestyle15.txt",
        "openings_freestyle15_piskvork.txt",
        "openings/openings_freestyle.txt",
    ),
    "forbidden": ("openings/openings_renju.txt",),
}
MOVE_RE = re.compile(r"^(\d+),(\d+),(-?\d+)$")


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


def position_identity(stones: list[tuple[int, int, int]]) -> str:
    variants = []
    for variant in range(8):
        transformed = [(*transform((x, y), variant), side)
                       for x, y, side in stones]
        min_x = min(x for x, _, _ in transformed)
        min_y = min(y for _, y, _ in transformed)
        normalized = sorted((x - min_x, y - min_y, side)
                            for x, y, side in transformed)
        variants.append(tuple(normalized))
    payload = " ".join(f"{x},{y},{side}" for x, y, side in min(variants))
    return hashlib.sha256(payload.encode()).hexdigest()[:20]


def move_position_identity(moves: list[tuple[int, int]]) -> str:
    return position_identity([(x, y, 1 if ply % 2 == 0 else -1)
                              for ply, (x, y) in enumerate(moves)])


def embed_prefix(moves: list[tuple[int, int]], source_board_size: int) -> list[tuple[int, int]] | None:
    if len(set(moves)) != len(moves):
        return None
    if source_board_size == 15:
        return moves if all(-7 <= x <= 7 and -7 <= y <= 7 for x, y in moves) else None

    # Older official Freestyle opening packages are for 20x20.  Preserve only
    # edge-independent early shapes and translate them losslessly into the
    # central 15x15 area.  No scale, rotation, or move reordering is applied.
    source = [(x + 10, y + 10) for x, y in moves]
    if any(not (2 <= x <= 17 and 2 <= y <= 17) for x, y in source):
        return None
    min_x, max_x = min(x for x, _ in source), max(x for x, _ in source)
    min_y, max_y = min(y for _, y in source), max(y for _, y in source)
    span_x, span_y = max_x - min_x, max_y - min_y
    if span_x > 10 or span_y > 10:
        return None
    target_min_x = (14 - span_x) // 2
    target_min_y = (14 - span_y) // 2
    return [(x - min_x + target_min_x - 7,
             y - min_y + target_min_y - 7) for x, y in source]


def parse_psq_prefix(payload: bytes, source_board_size: int,
                     maximum: int) -> list[tuple[int, int]]:
    lines = payload.decode("utf-8-sig", errors="strict").splitlines()
    if not lines or f"{source_board_size}x{source_board_size}" not in lines[0]:
        return []
    moves = []
    for raw in lines[1:]:
        match = MOVE_RE.match(raw.strip())
        if not match:
            break
        x, y = int(match[1]) - 1, int(match[2]) - 1
        if not (0 <= x < source_board_size and 0 <= y < source_board_size):
            return []
        center = 7 if source_board_size == 15 else 10
        moves.append((x - center, y - center))
        if len(moves) == maximum:
            break
    return moves


def parse_package(path: Path, rule: str,
                  maximum: int) -> tuple[int, str, list[dict[str, object]]]:
    match = re.search(r"(20\d\d)", path.name)
    if not match:
        raise ValueError(f"cannot determine package year from {path}")
    year = int(match[1])
    package_hash = hashlib.sha256(path.read_bytes()).hexdigest()
    records: list[dict[str, object]] = []
    with zipfile.ZipFile(path) as archive:
        available = set(archive.namelist())
        member = next((name for name in MEMBERS[rule] if name in available), None)
        if member is not None:
            source_board_size = (20 if member.endswith("openings_freestyle.txt")
                                 else 15)
            for index, raw in enumerate(
                    archive.read(member).decode("utf-8-sig").splitlines()):
                values = [int(value) for value in raw.split(",")]
                records.append({
                    "moves": list(zip(values[0::2], values[1::2])),
                    "member": member,
                    "sourceBoardSize": source_board_size,
                    "sourceIndex": index,
                    "sourceType": "official-opening-package",
                })

        division = ("Renju" if rule == "forbidden" else
                    ("Freestyle15_1" if year >= 2024 else "Freestyle1"))
        source_board_size = 15 if rule == "forbidden" or year >= 2024 else 20
        psq_members = sorted(
            name for name in available
            if name.lower().startswith(division.lower() + "/")
            and name.lower().endswith(".psq")
        )
        for index, name in enumerate(psq_members):
            moves = parse_psq_prefix(archive.read(name), source_board_size, maximum)
            if moves:
                records.append({
                    "moves": moves,
                    "member": name,
                    "sourceBoardSize": source_board_size,
                    "sourceIndex": index,
                    "sourceType": "official-top-division-game",
                })
    return year, package_hash, records


def add_move_exclusions(moves: list[tuple[int, int]], minimum: int,
                        maximum: int, ordered: set[str], positions: set[str]) -> None:
    for length in range(minimum, min(maximum, len(moves)) + 1):
        prefix = moves[:length]
        ordered.add(identity(canonical(prefix)))
        positions.add(move_position_identity(prefix))


def excluded_identities(jsonl_paths: list[Path], json_paths: list[Path],
                        minimum: int, maximum: int) -> tuple[set[str], set[str]]:
    ordered: set[str] = set()
    positions: set[str] = set()
    for path in jsonl_paths:
        for raw in path.read_text(encoding="utf-8").splitlines():
            record = json.loads(raw)
            if record.get("type") != "game":
                continue
            prefix_length = len(record["moves"]) - len(record["steps"])
            prefix = [(move[0] - 7, move[1] - 7)
                      for move in record["moves"][:prefix_length]]
            add_move_exclusions(prefix, minimum, maximum, ordered, positions)

    move_keys = {"movesRelative", "canonicalMovesRelative",
                 "scheduledMovesRelative"}

    def walk(value: object) -> None:
        if isinstance(value, dict):
            stones = value.get("stones")
            if (isinstance(stones, list) and stones and
                    all(isinstance(item, list) and len(item) == 3
                        for item in stones)):
                positions.add(position_identity([
                    (int(item[0]) - 7, int(item[1]) - 7, int(item[2]))
                    for item in stones
                ]))
            for key, child in value.items():
                if (key in move_keys and isinstance(child, list) and child and
                        all(isinstance(item, list) and len(item) >= 2
                            for item in child)):
                    add_move_exclusions(
                        [(int(item[0]), int(item[1])) for item in child],
                        minimum, maximum, ordered, positions)
                walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    for path in json_paths:
        walk(json.loads(path.read_text(encoding="utf-8")))
    return ordered, positions


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", required=True, action="append", type=Path)
    parser.add_argument("--exclude-jsonl", action="append", default=[], type=Path)
    parser.add_argument("--exclude-json", action="append", default=[], type=Path)
    parser.add_argument("--rule", required=True, choices=sorted(MEMBERS))
    parser.add_argument("--master-seed", required=True)
    parser.add_argument("--identity-count", type=int, default=50)
    parser.add_argument("--min-prefix", required=True, type=int)
    parser.add_argument("--max-prefix", required=True, type=int)
    parser.add_argument("--asset", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()
    if args.identity_count <= 0 or args.identity_count > 100:
        raise SystemExit("--identity-count must be in 1..100")
    if args.min_prefix <= 0 or args.max_prefix < args.min_prefix:
        raise SystemExit("prefix range must satisfy 0 < min-prefix <= max-prefix")
    if args.max_prefix > 8:
        raise SystemExit("--max-prefix must be no greater than 8 plies")
    try:
        seed_value = int(args.master_seed, 0)
    except ValueError as error:
        raise SystemExit("--master-seed must be an integer literal") from error
    master_seed = f"0x{seed_value:016x}"

    excluded, excluded_positions = excluded_identities(
        args.exclude_jsonl, args.exclude_json, args.min_prefix, args.max_prefix)
    candidates_by_position: dict[str, dict[str, object]] = {}
    packages: list[dict[str, object]] = []
    for package in args.package:
        year, package_hash, records = parse_package(
            package, args.rule, args.max_prefix)
        packages.append({"year": year, "sha256": package_hash,
                         "recordCount": len(records),
                         "used": bool(records)})
        for record in records:
            line = record["moves"]
            last_prefix = min(args.max_prefix, len(line))
            for prefix_length in range(args.min_prefix, last_prefix + 1):
                embedded = embed_prefix(line[:prefix_length],
                                        record["sourceBoardSize"])
                if embedded is None:
                    continue
                normalized = canonical(embedded)
                canonical_id = identity(normalized)
                canonical_position_id = move_position_identity(list(normalized))
                if (canonical_id in excluded or
                        canonical_position_id in excluded_positions):
                    continue
                entry = candidates_by_position.get(canonical_position_id)
                if entry is None or canonical_id < entry["canonicalId"]:
                    entry = {
                    "canonicalId": canonical_id,
                    "canonicalPositionId": canonical_position_id,
                    "canonicalMovesRelative": normalized,
                    "provenance": [],
                    "sourceOccurrenceCount": 0,
                    "sourceYears": set(),
                    }
                    candidates_by_position[canonical_position_id] = entry
                entry["sourceOccurrenceCount"] += 1
                entry["sourceYears"].add(year)
                if len(entry["provenance"]) < 8:
                    entry["provenance"].append({
                    "year": year, "openingIndex": record["sourceIndex"],
                    "prefixLength": prefix_length,
                    "sourceRule": args.rule,
                    "sourceBoardSize": record["sourceBoardSize"],
                    "sourceMember": record["member"],
                    "sourceType": record["sourceType"],
                })
    candidates = list(candidates_by_position.values())
    official_remaining = len(candidates)
    if len(candidates) < args.identity_count:
        raise SystemExit(
            f"only {len(candidates)} official short prefixes remain after exclusion; "
            f"need {args.identity_count}"
        )

    def selection_key(entry: dict[str, object]) -> str:
        return hashlib.sha256(
            f"{master_seed}:{entry['canonicalId']}".encode()
        ).hexdigest()

    selected = sorted(candidates, key=selection_key)[:args.identity_count]
    rendered = [
        f"/* Generated held-out schedule: {args.identity_count} unique canonical Gomocup prefixes. */",
        "typedef struct {",
        "    unsigned char length;",
        "    signed char coordinates[50];",
        "    unsigned short sourceYear;",
        "    unsigned int sourceIndex;",
        "    unsigned char prefixLength;",
        "} FCBenchmarkOpening;",
        "",
        f"static const FCBenchmarkOpening fcFormalOpenings[{args.identity_count}] = {{",
    ]
    manifest_entries: list[dict[str, object]] = []
    for opening_id, entry in enumerate(selected):
        moves = list(entry["canonicalMovesRelative"])
        variant = int(hashlib.sha256(
            f"transform:{master_seed}:{opening_id}".encode()
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
            "canonicalPositionId": entry["canonicalPositionId"],
            "scheduledTransform": variant,
            "scheduledMovesRelative": scheduled,
            "provenance": entry["provenance"],
            "sourceOccurrenceCount": entry["sourceOccurrenceCount"],
            "sourceYears": sorted(entry["sourceYears"]),
        })
    rendered.extend([
        "};",
        "static const int fcFormalOpeningCount =",
        "    (int)(sizeof(fcFormalOpenings) / sizeof(fcFormalOpenings[0]));",
        "",
    ])
    args.asset.write_text("\n".join(rendered), encoding="utf-8")
    manifest = {
        "schemaVersion": 2,
        "seedDomain": "five-star-final-v1",
        "masterSeed": master_seed,
        "rule": args.rule,
        "identityCount": len(manifest_entries),
        "prefixLengthRange": {
            "minimumPlies": args.min_prefix,
            "maximumPlies": args.max_prefix,
        },
        "uniqueCanonicalIdentityCount": len({e["canonicalId"] for e in manifest_entries}),
        "uniqueCanonicalPositionCount": len({e["canonicalPositionId"] for e in manifest_entries}),
        "excludedDiagnosticCanonicalIds": sorted(excluded),
        "excludedDiagnosticCanonicalPositionIds": sorted(excluded_positions),
        "sourcePackages": packages,
        "selection": "SHA-256 ordering of official short held-out canonical prefixes after diagnostic exclusion",
        "officialCandidateCountAfterExclusion": official_remaining,
        "syntheticNeutralCandidatesAdded": 0,
        "entries": manifest_entries,
    }
    args.manifest.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    print(f"selected {len(selected)} of {len(candidates)} held-out canonical positions; "
          f"excluded identities={len(excluded)} positions={len(excluded_positions)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
