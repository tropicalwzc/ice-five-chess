#!/usr/bin/env python3
"""Audit final schedules against every predeclared diagnostic domain."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import generate_formal_opening_schedule as schedule


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--schedule", action="append", required=True, type=Path)
    parser.add_argument("--exclude-jsonl", action="append", default=[], type=Path)
    parser.add_argument("--exclude-json", action="append", default=[], type=Path)
    parser.add_argument("--prior-seeds", required=True, type=Path)
    parser.add_argument("--source-packages", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    if len(args.schedule) != 2:
        raise SystemExit("exactly two rule-separated schedules are required")

    prior_seed_data = json.loads(args.prior_seeds.read_text(encoding="utf-8"))
    prior_seeds = {seed.lower() for seed in prior_seed_data["seeds"]}
    source_data = json.loads(args.source_packages.read_text(encoding="utf-8"))
    source_hashes = {item["year"]: item["sha256"]
                     for item in source_data["packages"]}
    excluded_ids, excluded_positions = schedule.excluded_identities(
        args.exclude_jsonl, args.exclude_json, 4, 8)

    manifests = [json.loads(path.read_text(encoding="utf-8"))
                 for path in args.schedule]
    failures: list[str] = []
    rules = {manifest.get("rule") for manifest in manifests}
    if rules != {"freestyle", "forbidden"}:
        failures.append(f"unexpected rule set: {sorted(str(rule) for rule in rules)}")
    seeds = [manifest.get("masterSeed", "").lower() for manifest in manifests]
    if len(set(seeds)) != 2:
        failures.append("formal master seeds are not distinct")
    for seed in seeds:
        if seed in prior_seeds:
            failures.append(f"formal seed was present before freeze: {seed}")

    identity_sets = []
    position_sets = []
    summaries = []
    for path, manifest in zip(args.schedule, manifests):
        entries = manifest.get("entries", [])
        ids = {entry["canonicalId"] for entry in entries}
        positions = {entry["canonicalPositionId"] for entry in entries}
        identity_sets.append(ids)
        position_sets.append(positions)
        if len(entries) != 50 or len(ids) != 50 or len(positions) != 50:
            failures.append(f"{path}: expected 50 unique identities and positions")
        lengths = [len(entry["scheduledMovesRelative"]) for entry in entries]
        if not lengths or min(lengths) < 4 or max(lengths) > 8:
            failures.append(f"{path}: prefix outside 4..8 plies")
        if ids & excluded_ids:
            failures.append(f"{path}: overlaps excluded ordered identities")
        if positions & excluded_positions:
            failures.append(f"{path}: overlaps excluded canonical positions")
        for package in manifest.get("sourcePackages", []):
            if package["year"] not in source_hashes:
                failures.append(f"{path}: unapproved source year {package['year']}")
            elif package["sha256"] != source_hashes[package["year"]]:
                failures.append(f"{path}: source hash drift for {package['year']}")
        for entry in entries:
            if not entry.get("provenance"):
                failures.append(f"{path}: missing provenance")
            for provenance in entry.get("provenance", []):
                if provenance.get("sourceType") not in {
                        "official-opening-package", "official-top-division-game"}:
                    failures.append(f"{path}: non-official provenance")
        summaries.append({
            "path": str(path),
            "sha256": sha256(path),
            "rule": manifest.get("rule"),
            "masterSeed": manifest.get("masterSeed"),
            "identityCount": len(entries),
            "uniqueCanonicalIdentityCount": len(ids),
            "uniqueCanonicalPositionCount": len(positions),
            "minimumPrefixPlies": min(lengths) if lengths else None,
            "maximumPrefixPlies": max(lengths) if lengths else None,
        })

    cross_identity_overlap = identity_sets[0] & identity_sets[1]
    cross_position_overlap = position_sets[0] & position_sets[1]
    if cross_identity_overlap:
        failures.append("rule schedules share ordered opening identities")
    if cross_position_overlap:
        failures.append("rule schedules share canonical starting positions")

    report = {
        "schemaVersion": 1,
        "status": "pass" if not failures else "fail",
        "schedules": summaries,
        "priorSeedCatalog": {"path": str(args.prior_seeds),
                             "sha256": sha256(args.prior_seeds),
                             "count": len(prior_seeds)},
        "sourcePackageManifest": {"path": str(args.source_packages),
                                  "sha256": sha256(args.source_packages),
                                  "count": len(source_hashes)},
        "excludedOrderedIdentityCount": len(excluded_ids),
        "excludedCanonicalPositionCount": len(excluded_positions),
        "crossRuleOrderedIdentityOverlap": sorted(cross_identity_overlap),
        "crossRuleCanonicalPositionOverlap": sorted(cross_position_overlap),
        "failures": failures,
    }
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
    print(f"formal schedule separation: {report['status']}; failures={len(failures)}")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
