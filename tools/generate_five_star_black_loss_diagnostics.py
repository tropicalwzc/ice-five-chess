#!/usr/bin/env python3
"""Freeze five-star baselines and extract prior black-loss proof diagnostics.

The emitted boards are test/diagnostic evidence only.  This tool deliberately
does not emit a C include or a move lookup table.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PRIOR = ROOT / "reports/five_chess/five_star_rule_partitioned_20260814"
INPUTS = (
    ("free_vs_four", False, "four-star", PRIOR / "free_vs_four.jsonl"),
    ("free_vs_legacy", False, "legacy-three-star", PRIOR / "free_vs_legacy.jsonl"),
    ("forbidden_vs_four", True, "four-star", PRIOR / "forbidden_vs_four.jsonl"),
    ("forbidden_vs_legacy", True, "legacy-three-star", PRIOR / "forbidden_vs_legacy.jsonl"),
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rows(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def transform_point(transform: int, x: int, y: int) -> tuple[int, int]:
    last = 14
    return (
        (x, y), (last - x, y), (x, last - y), (last - x, last - y),
        (y, x), (last - y, x), (y, last - x), (last - y, last - x),
    )[transform]


def first_no_escape(game: dict) -> tuple[int, dict] | None:
    opening_plies = len(game["moves"]) - len(game["steps"])
    assert opening_plies >= 0
    for index, step in enumerate(game["steps"]):
        if (step["engine"] == "new" and step.get("proofStatus") == 1 and
                step.get("proofCertificateVerified") and
                step.get("tacticalClass") == 4 and
                step.get("overrideReason") == 0):
            ply = opening_plies + index
            assert game["moves"][ply][:2] == [step["x"], step["y"]]
            return ply, step
    return None


def changed_corpus(game: dict) -> list[dict]:
    opening_plies = len(game["moves"]) - len(game["steps"])
    answer = []
    for index, step in enumerate(game["steps"]):
        if (step["engine"] == "new" and step.get("corpusAccepted") and
                [step["x"], step["y"]] !=
                [step.get("fourStarX"), step.get("fourStarY")]):
            answer.append({
                "ply": opening_plies + index + 1,
                "historicalMove": [step["x"], step["y"]],
                "fourStarMove": [step["fourStarX"], step["fourStarY"]],
                "reason": step["corpusReason"],
                "matchType": step["corpusMatchType"],
                "supportGames": step["corpusSupportGames"],
                "supportEvents": step["corpusSupportEvents"],
            })
    return answer


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    sources = {}
    diagnostics = []
    primary_count = 0
    for label, forbidden, opponent, path in INPUTS:
        data = rows(path)
        header = next(item for item in data if item["type"] == "header")
        games = [item for item in data if item["type"] == "game"]
        sources[label] = {"path": str(path.relative_to(ROOT)),
                          "sha256": sha256(path), "masterSeed": header["masterSeed"],
                          "seedDomain": header["seedDomain"]}
        for game in games:
            if game["newColor"] != 1 or game["winner"] != -1:
                continue
            found = first_no_escape(game)
            assert found is not None, (label, game["openingId"])
            ply_index, step = found
            board_moves = game["moves"][:ply_index]
            symmetry_boards = []
            for symmetry in range(8):
                symmetry_boards.append({
                    "transform": symmetry,
                    "stones": [[*transform_point(symmetry, move[0], move[1]), move[2]]
                               for move in board_moves],
                })
            diagnostics.append({
                "source": label,
                "rule": "forbidden" if forbidden else "freestyle",
                "opponent": opponent,
                "historicalOpeningId": game["openingId"],
                "decisionPly": ply_index + 1,
                "historicalDefault": [step["defaultX"], step["defaultY"]],
                "historicalSelected": [step["x"], step["y"]],
                "opponentProofDistance": step["proofDistance"],
                "opponentCertificateId": step["proofCertificateId"],
                "corpusDivergences": changed_corpus(game),
                "assertions": [
                    "board-integrity", "rule-legality", "certificate-validity",
                    "known-loss-ranks-below-unknown", "board-deterministic-evaluation",
                ],
                "symmetries": symmetry_boards,
            })
            if opponent == "four-star":
                primary_count += 1

    diagnostics_payload = {
        "schemaVersion": 1,
        "purpose": "diagnostic-regression-only-not-runtime-advice",
        "selectionPolicy": "all prior five-star-black losses; first verified opponent win with no escape",
        "primaryUniqueRuleOpenings": primary_count,
        "recordsIncludingDualBaselines": len(diagnostics),
        "symmetryPositions": len(diagnostics) * 8,
        "sources": sources,
        "records": diagnostics,
    }
    diagnostics_path = args.output_dir / "loss_diagnostics.json"
    diagnostics_path.write_text(json.dumps(diagnostics_payload, indent=2) + "\n")

    baseline_files = [
        ROOT / "ice five chess/FiveChessAI.c",
        ROOT / "ice five chess/FiveChessAI.h",
        ROOT / "ice five chess/doublethree.m",
        ROOT / "ice five chess/FiveChessEliteCorpus.inc",
        ROOT / "tools/elite_corpus_manifest.json",
        PRIOR / "README.md",
        PRIOR / "five_star_rule_comparison.json",
        ROOT / "openspec/changes/add-curated-five-star-opening-advisor/final-opening-schedule.json",
    ]
    manifest = {
        "schemaVersion": 1,
        "profile": "five-star-curated-opening-advisor@5.1.0-elite-rule-partitioned-local-v2",
        "fourStar": "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book",
        "legacyThreeStar": "legacy-three-star@5224020",
        "files": {str(path.relative_to(ROOT)): sha256(path) for path in baseline_files},
        "priorFormalSources": sources,
        "lossDiagnosticsSha256": sha256(diagnostics_path),
    }
    (args.output_dir / "baseline_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n")

    production = (ROOT / "ice five chess/FiveChessAI.c").read_text()
    historical_ids_literal = "2, 10, 11, 17, 18, 35, 36, 39, 42, 49"
    audit = {
        "schemaVersion": 1,
        "runtimeDiagnosticIncludeGenerated": False,
        "historicalIdListPresentInProduction": historical_ids_literal in production,
        "priorSeedDomains": sorted({item["seedDomain"] for item in sources.values()}),
        "formalSeedStatus": "must-be-generated-after-profile-freeze",
        "policy": "prior failures and corpus positions are diagnostic-only; formal schedules use untouched natural seeds",
    }
    assert not audit["historicalIdListPresentInProduction"]
    (args.output_dir / "separation_audit.json").write_text(
        json.dumps(audit, indent=2) + "\n")

    checksum_targets = [diagnostics_path, args.output_dir / "baseline_manifest.json",
                        args.output_dir / "separation_audit.json"]
    (args.output_dir / "checksums.sha256").write_text("".join(
        f"{sha256(path)}  {path.name}\n" for path in checksum_targets))


if __name__ == "__main__":
    main()
