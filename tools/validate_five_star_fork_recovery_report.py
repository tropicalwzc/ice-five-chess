#!/usr/bin/env python3
"""Validate a completed fixed-position fork-recovery replay report."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


WORKERS = (1, 4, 8)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    report = json.loads(args.report.read_text(encoding="utf-8"))
    entries = manifest.get("entries", [])
    if len(entries) != 97 or report.get("fixtureCount") != 97:
        raise SystemExit("fixture count gate failed")
    if report.get("fixtureManifestSha256") != sha256(args.manifest):
        raise SystemExit("fixture manifest hash drift")
    if tuple(report.get("workerMatrix", [])) != WORKERS:
        raise SystemExit("worker matrix must be exactly 1/4/8")
    ids = [entry["id"] for entry in entries]
    cells = report.get("cells", {})
    for worker in WORKERS:
        cell = cells.get(str(worker))
        if cell is None or cell.get("workerCount") != worker:
            raise SystemExit(f"missing worker cell {worker}")
        rows = cell.get("records", [])
        if [row.get("id") for row in rows] != ids:
            raise SystemExit(f"fixture order drift in worker cell {worker}")
        for row in rows:
            if (not row.get("found") or not row.get("legal") or
                    row.get("decisionStatus") == 2 or
                    row.get("provenLoss", False)):
                raise SystemExit(f"invalid result in worker cell {worker}: "
                                 f"{row.get('id')}")
            if row.get("decisionMemoryReserved") != 0:
                raise SystemExit(f"live ledger bytes in worker cell {worker}: "
                                 f"{row.get('id')}")
            if (row.get("decisionMemoryReleased", 0) <
                    row.get("decisionMemoryPeakReserved", 0)):
                raise SystemExit(f"memory release mismatch in worker cell "
                                 f"{worker}: {row.get('id')}")
            if row.get("actualWorkerCap") != worker:
                raise SystemExit(f"worker cap drift in worker cell {worker}: "
                                 f"{row.get('id')}")
            if (row.get("candidateCoverageComplete") and
                    row.get("forkSafeCandidates", 0) > 0 and
                    row.get("forkRisk") == 3):
                raise SystemExit(f"avoidable fork in worker cell {worker}: "
                                 f"{row.get('id')}")
    for fixture_id in ids:
        outcomes = []
        for worker in WORKERS:
            row = next(record for record in cells[str(worker)]["records"]
                       if record["id"] == fixture_id)
            outcomes.append((row["x"], row["y"], row["decisionStatus"],
                             row["forkRisk"]))
        if len(set(outcomes)) != 1:
            raise SystemExit(f"selected output drift: {fixture_id}")
    if not report.get("gate", {}).get("passed"):
        raise SystemExit("replay gate is not marked passed")
    print("five-star fork-recovery report: pass")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
