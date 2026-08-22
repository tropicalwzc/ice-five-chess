#!/usr/bin/env python3
"""Random small-sample search for research-only five-star candidates.

The runner deliberately treats the benchmark as an executable oracle.  It
does not mutate source files or the playable profile; every child node is a
profile mutation manifest evaluated against the frozen four-star opponent.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
sys.path.insert(0, str(SCRIPT_DIR))

from replay_candidate_search import (  # noqa: E402
    configure_library,
    replay_search_cell,
)


MASK64 = (1 << 64) - 1
DEFAULT_MASTER_SEED = 0xA8B6C4D220260813
TRAINING_POOL_SIZE = 100
FORMAL_POOL_SIZE = 50
SAMPLE_SIZE = 12
OPPONENT_PROFILE = "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book"

DEFAULT_MUTATION_ARMS = (
    "black-double-three-weight=10",
    "black-double-three-weight=20",
    "black-double-three-weight=30",
    "immediate-block=1",
    "two-step-fork=1",
    "vct-on-unknown=1",
    "guard-max-alternatives=4",
    "recovery-ordering=immediate-block",
)


def mix64(value: int) -> int:
    value = (value + 0x9E3779B97F4A7C15) & MASK64
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK64
    return (value ^ (value >> 31)) & MASK64


def sample_ids(seed: int, pool_size: int, count: int,
               excluded: set[int] | None = None) -> list[int]:
    excluded = excluded or set()
    pool = [value for value in range(pool_size) if value not in excluded]
    if count != SAMPLE_SIZE:
        raise ValueError("candidate search samples must contain exactly 12 IDs")
    if len(pool) < count:
        raise ValueError("opening pool is smaller than the requested sample")
    state = seed & MASK64
    selected: list[int] = []
    for _ in range(count):
        state = mix64(state)
        index = state % len(pool)
        selected.append(pool.pop(index))
    return selected


def round_seed(master_seed: int, round_number: int, label: str) -> int:
    label_hash = int.from_bytes(
        hashlib.sha256(label.encode("utf-8")).digest()[:8], "big")
    return mix64(master_seed ^ ((round_number + 1) *
                                0xD1342543DE82EF95) ^ label_hash)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def source_hash(repo_root: Path) -> str:
    paths = (
        repo_root / "ice five chess" / "FiveChessAI.c",
        repo_root / "ice five chess" / "FiveChessAI.h",
        repo_root / "tools" / "five_chess_benchmark.m",
        repo_root / "tools" / "replay_candidate_search.py",
        repo_root / "tools" / "five_chess_candidate_search.py",
    )
    digest = hashlib.sha256()
    for path in paths:
        digest.update(str(path.relative_to(repo_root)).encode("utf-8"))
        digest.update(path.read_bytes())
    return digest.hexdigest()


def parse_mutation_spec(spec: str) -> dict[str, str]:
    if spec in ("", "none"):
        return {}
    values: dict[str, str] = {}
    for token in spec.split(";"):
        if token.count("=") != 1:
            raise ValueError(f"invalid mutation token: {token}")
        key, value = token.split("=", 1)
        if not key or not value or key in values:
            raise ValueError(f"invalid or duplicate mutation key: {token}")
        values[key] = value
    allowed = {
        "black-double-three-weight",
        "immediate-block",
        "two-step-fork",
        "vct-on-unknown",
        "guard-max-alternatives",
        "recovery-ordering",
    }
    unknown = set(values) - allowed
    if unknown:
        raise ValueError(f"unsupported mutation keys: {sorted(unknown)}")
    if "black-double-three-weight" in values:
        value = int(values["black-double-three-weight"])
        if not 0 <= value <= 100:
            raise ValueError("black-double-three-weight must be 0..100")
    for key in ("immediate-block", "two-step-fork", "vct-on-unknown"):
        if key in values and values[key] not in ("0", "1"):
            raise ValueError(f"{key} must be 0 or 1")
    if "guard-max-alternatives" in values:
        value = int(values["guard-max-alternatives"])
        if not 1 <= value <= 32:
            raise ValueError("guard-max-alternatives must be 1..32")
    if ("recovery-ordering" in values and
            values["recovery-ordering"] not in
            ("immediate-block", "structural")):
        raise ValueError("unsupported recovery-ordering value")
    return values


def canonical_mutation(values: dict[str, str]) -> str:
    if not values:
        return "none"
    return ";".join(f"{key}={values[key]}" for key in sorted(values))


def child_mutation(parent_spec: str, arm: str) -> str:
    parent = parse_mutation_spec(parent_spec)
    arm_values = parse_mutation_spec(arm)
    if len(arm_values) != 1:
        raise ValueError("each child arm must change exactly one key")
    key, value = next(iter(arm_values.items()))
    if parent.get(key) == value:
        raise ValueError("mutation arm does not change its parent")
    parent[key] = value
    return canonical_mutation(parent)


def node_id(parent_id: str, mutation: str, source_digest: str) -> str:
    payload = json.dumps(
        {"parent": parent_id, "mutation": mutation,
         "sourceHash": source_digest}, sort_keys=True,
        separators=(",", ":"),).encode("utf-8")
    return "node-" + sha256_bytes(payload)[:16]


def score_rate(cell: dict[str, Any]) -> float:
    return float(cell["scoreRate"])


def objective(scores: dict[str, float]) -> tuple[float, float, float]:
    values = [scores["free"], scores["forbidden"]]
    return (min(values), sum(values) / len(values), sum(values))


def floor_passed(scores: dict[str, float], floor: float = 0.5) -> bool:
    return all(scores[mode] >= floor for mode in ("free", "forbidden"))


def paired_improves(candidate_scores: dict[str, float],
                    incumbent_scores: dict[str, float]) -> bool:
    return objective(candidate_scores) > objective(incumbent_scores)


def append_jsonl(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as stream:
        stream.write(json.dumps(value, ensure_ascii=False,
                                sort_keys=True) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def atomic_write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8",
                                     dir=path.parent, delete=False) as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2,
                  sort_keys=True)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
        temporary = Path(stream.name)
    temporary.replace(path)


def read_header(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                value = json.loads(line)
                if value.get("type") != "header":
                    raise ValueError(f"{path}: first row is not a header")
                return value
    raise ValueError(f"{path}: empty benchmark output")


def unique_output_path(path: Path) -> Path:
    if not path.exists():
        return path
    index = 1
    while True:
        candidate = path.with_name(f"{path.stem}.retry{index}{path.suffix}")
        if not candidate.exists():
            return candidate
        index += 1


class SearchRunner:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.repo_root = args.repo_root.resolve()
        self.benchmark = args.benchmark.resolve()
        self.library = configure_library(args.library.resolve())
        self.output_dir = args.output_dir.resolve()
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.nodes_path = self.output_dir / "nodes.jsonl"
        self.rounds_path = self.output_dir / "rounds.jsonl"
        self.heldout_path = self.output_dir / "heldout.jsonl"
        self.current_path = self.output_dir / "current.json"
        self.source_digest = source_hash(self.repo_root)
        self.build_digest = sha256_file(self.benchmark)

    def root_node(self) -> dict[str, Any]:
        root_id = "root-5.8.1-" + sha256_bytes(
            b"five-star-5.8.1|five-star-random-candidate-search")[:16]
        return {
            "nodeId": root_id,
            "parentNodeId": None,
            "profile": "five-star-5.8.1",
            "mutation": "none",
            "status": "promoted",
            "promotionReason": "initial exact 5.8.1 incumbent",
            "sourceHash": self.source_digest,
            "buildHash": self.build_digest,
            "historicalRejected": [{
                "profile": "5.8.2-black-defense-recovery-v1-baseline-block-fork-vct",
                "reason": "black score below 50% in previous four-star cells",
                "freeBlack": {"wins": 5, "games": 12, "scoreRate": 5 / 12},
                "forbiddenBlack": {"wins": 4, "games": 12,
                                    "scoreRate": 4 / 12},
            }],
        }

    def load_current(self) -> dict[str, Any]:
        if not self.current_path.exists():
            current = self.root_node()
            append_jsonl(self.nodes_path, current)
            atomic_write_json(self.current_path, current)
            return current
        current = json.loads(self.current_path.read_text(encoding="utf-8"))
        if current.get("sourceHash") != self.source_digest:
            raise RuntimeError("current champion source hash differs; use a new output directory")
        if current.get("buildHash") != self.build_digest:
            raise RuntimeError("current champion build hash differs; use a new output directory")
        parse_mutation_spec(current.get("mutation", "none"))
        return current

    def run_cell(self, *, node: dict[str, Any], sample_ids_value: list[int],
                 sample_seed: int, sample_label: str, forbidden: bool,
                 round_dir: Path, role: str, suite: str =
                 "five-star-candidate-search") -> dict[str, Any]:
        mode = "forbidden" if forbidden else "free"
        canonical_path = round_dir / sample_label / mode / f"{role}.jsonl"
        mutation = node.get("mutation", "none")
        expected_parent = node.get("parentNodeId")
        if canonical_path.exists():
            try:
                cached = replay_search_cell(
                    canonical_path, self.library, sample_ids_value, mutation,
                    node["nodeId"], expected_parent, forbidden,
                    self.args.replay_certificates, expected_suite=suite)
            except (AssertionError, OSError, ValueError, json.JSONDecodeError):
                cached = None
            if cached is not None and cached.get("status") == "pass":
                cached.update({
                    "role": role,
                    "mode": mode,
                    "sampleLabel": sample_label,
                    "sampleSeed": f"0x{sample_seed:016x}",
                    "nodeId": node["nodeId"],
                    "mutation": mutation,
                    "cached": True,
                    "rawSha256": sha256_file(canonical_path),
                })
                return cached
        raw_path = unique_output_path(canonical_path)
        raw_path.parent.mkdir(parents=True, exist_ok=True)
        command = [
            str(self.benchmark), "--output", str(raw_path),
            "--profile", "five-star-5.8.1", "--suite", suite,
            "--opponent", "four-star", "--random-mode", "best",
            "--seed", f"0x{sample_seed:016x}",
            "--opening-ids", ",".join(str(value) for value in sample_ids_value),
            "--max-moves", str(self.args.max_moves), "--black-only", "1",
            "--forbidden-black", "1" if forbidden else "0",
            "--mutation", mutation, "--node-id", node["nodeId"],
            "--sample-label", sample_label,
        ]
        if node.get("parentNodeId") is not None:
            command.extend(["--parent-node-id", node["parentNodeId"]])
        log_path = raw_path.with_suffix(".log")
        completed = subprocess.run(
            command, cwd=self.repo_root, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        log_path.write_text(
            json.dumps({"command": command, "returnCode": completed.returncode,
                        "stdout": completed.stdout, "stderr": completed.stderr},
                       ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        if completed.returncode != 0:
            return {
                "status": "invalid", "path": str(raw_path),
                "returnCode": completed.returncode,
                "reason": "benchmark process failed", "log": str(log_path),
            }
        try:
            result = replay_search_cell(
                raw_path, self.library, sample_ids_value, mutation,
                node["nodeId"], node.get("parentNodeId"), forbidden,
                self.args.replay_certificates, expected_suite=suite)
        except (AssertionError, OSError, ValueError, json.JSONDecodeError) as error:
            return {
                "status": "invalid", "path": str(raw_path),
                "reason": str(error), "log": str(log_path),
            }
        result.update({
            "role": role,
            "mode": mode,
            "sampleLabel": sample_label,
            "sampleSeed": f"0x{sample_seed:016x}",
            "nodeId": node["nodeId"],
            "mutation": mutation,
            "log": str(log_path),
            "command": command,
            "rawSha256": sha256_file(raw_path),
            "cached": False,
        })
        return result

    def evaluate_pair(self, *, parent: dict[str, Any], candidate: dict[str, Any],
                      ids: list[int], seed: int, label: str,
                      round_dir: Path) -> dict[str, Any]:
        result: dict[str, Any] = {"sampleLabel": label, "sampleSeed": seed,
                                  "openingIds": ids, "cells": {}}
        for forbidden in (False, True):
            mode = "forbidden" if forbidden else "free"
            result["cells"][f"incumbent-{mode}"] = self.run_cell(
                node=parent, sample_ids_value=ids, sample_seed=seed,
                sample_label=label, forbidden=forbidden, round_dir=round_dir,
                role="incumbent")
            result["cells"][f"candidate-{mode}"] = self.run_cell(
                node=candidate, sample_ids_value=ids, sample_seed=seed,
                sample_label=label, forbidden=forbidden, round_dir=round_dir,
                role="candidate")
        result["incumbentScores"] = {
            mode: score_rate(result["cells"][f"incumbent-{mode}"])
            for mode in ("free", "forbidden")
        }
        result["candidateScores"] = {
            mode: score_rate(result["cells"][f"candidate-{mode}"])
            for mode in ("free", "forbidden")
        }
        result["candidateFloorPassed"] = floor_passed(
            result["candidateScores"])
        result["incumbentValid"] = all(
            result["cells"][f"incumbent-{mode}"].get("status") == "pass"
            for mode in ("free", "forbidden"))
        result["candidateValid"] = all(
            result["cells"][f"candidate-{mode}"].get("status") == "pass"
            for mode in ("free", "forbidden"))
        result["improves"] = paired_improves(
            result["candidateScores"], result["incumbentScores"])
        # Kept as an explicit alias so existing round readers can distinguish
        # the training gate from the later validation gate.
        result["trainingImproves"] = result["improves"]
        return result

    def write_report(self, rounds: list[dict[str, Any]], current: dict[str, Any]) -> None:
        lines = [
            "# Five-star random candidate search",
            "",
            f"Current champion: `{current['nodeId']}`",
            f"Mutation: `{current.get('mutation', 'none')}`",
            "",
            "| Round | Candidate | Status | Free black W/D/L | Forbidden black W/D/L | Integrity | Reason |",
            "|---:|---|---|---|---|---|---|",
        ]
        for item in rounds:
            train = item.get("training", {})
            scores = train.get("candidateScores", {})
            cells = train.get("cells", {})
            def cell_text(mode: str) -> str:
                cell = cells.get(f"candidate-{mode}", {})
                games = cell.get("games", {})
                return (f"{games.get('wins', 0)}/{games.get('draws', 0)}/"
                        f"{games.get('losses', 0)} "
                        f"({scores.get(mode, 0):.3f}; "
                        f"p95 {cell.get('latency', {}).get('p95Ms', 0):.1f}ms)")
            integrity = ", ".join(
                f"{mode}:a{cells.get(f'candidate-{mode}', {}).get('anomalyCount', 0)}"
                f"/h{cells.get(f'candidate-{mode}', {}).get('hardLimitViolations', 0)}"
                for mode in ("free", "forbidden"))
            lines.append(
                f"| {item.get('round', '?')} | `{item.get('candidateNodeId', '?')}` "
                f"| {item.get('status', '?')} | "
                f"{cell_text('free')} | {cell_text('forbidden')} | "
                f"{integrity} | "
                f"{item.get('reason', '')} |")
        if self.heldout_path.exists():
            lines.extend(["", "## Formal held-out checks", "",
                          "Held-out cells are report-only and never affect "
                          "promotion or mutation selection.", "",
                          "| Round | Node | Free score | Forbidden score | Status |",
                          "|---:|---|---:|---:|---|"])
            for line in self.heldout_path.read_text(
                    encoding="utf-8").splitlines():
                if not line.strip():
                    continue
                item = json.loads(line)
                cells = item.get("cells", {})
                free = cells.get("free", {})
                forbidden = cells.get("forbidden", {})
                lines.append(
                    f"| {item.get('round', '?')} | `{item.get('nodeId', '?')}` "
                    f"| {free.get('scoreRate', 0):.3f} | "
                    f"{forbidden.get('scoreRate', 0):.3f} | "
                    f"{item.get('status', '?')} |")
        lines.extend([
            "", "## Provenance", "",
            f"- source hash: `{self.source_digest}`",
            f"- benchmark hash: `{self.build_digest}`",
            "- training pool: generated opening IDs `0..99`",
            "- sample size: 12 distinct IDs per round",
            "- opponent: frozen four-star",
            "- promotion objective: `(min score, mean score, total score)`",
        ])
        (self.output_dir / "search_report.md").write_text(
            "\n".join(lines) + "\n", encoding="utf-8")

    def evaluate_heldout(self, *, node: dict[str, Any], round_number: int,
                         sample_seed: int, ids: list[int],
                         round_dir: Path) -> dict[str, Any]:
        cells: dict[str, Any] = {}
        for forbidden in (False, True):
            mode = "forbidden" if forbidden else "free"
            cells[mode] = self.run_cell(
                node=node, sample_ids_value=ids, sample_seed=sample_seed,
                sample_label="heldout", forbidden=forbidden,
                round_dir=round_dir, role="champion",
                suite="five-star-natural-final")
        result = {
            "round": round_number,
            "nodeId": node["nodeId"],
            "sampleSeed": f"0x{sample_seed:016x}",
            "openingIds": ids,
            "poolVersion": "gomocup-formal-held-out-v4",
            "cells": cells,
            "status": "pass" if all(
                cell.get("status") == "pass" for cell in cells.values())
            else "invalid",
            "reportOnly": True,
        }
        append_jsonl(self.heldout_path, result)
        return result

    def run(self) -> int:
        parent = self.load_current()
        rounds: list[dict[str, Any]] = []
        if self.rounds_path.exists():
            rounds = [json.loads(line) for line in
                      self.rounds_path.read_text(encoding="utf-8").splitlines()
                      if line.strip()]
        start_round = len(rounds) + 1
        for round_number in range(start_round,
                                  start_round + self.args.rounds):
            train_seed = round_seed(self.args.master_seed, round_number,
                                     "training")
            ids = sample_ids(train_seed, TRAINING_POOL_SIZE, SAMPLE_SIZE)
            validation_seed = round_seed(self.args.master_seed, round_number,
                                         "validation")
            validation_ids = sample_ids(
                validation_seed, TRAINING_POOL_SIZE, SAMPLE_SIZE, set(ids))
            arm = self.args.mutation_arms[(round_number - 1) %
                                           len(self.args.mutation_arms)]
            candidate_mutation = child_mutation(
                parent.get("mutation", "none"), arm)
            candidate = {
                "nodeId": node_id(parent["nodeId"], candidate_mutation,
                                   self.source_digest),
                "parentNodeId": parent["nodeId"],
                "profile": "five-star-5.8.1",
                "mutation": candidate_mutation,
                "sourceHash": self.source_digest,
                "buildHash": self.build_digest,
            }
            round_dir = self.output_dir / f"round-{round_number:03d}"
            manifest = {
                "round": round_number,
                "parentNodeId": parent["nodeId"],
                "candidateNodeId": candidate["nodeId"],
                "mutationArm": arm,
                "candidateMutation": candidate_mutation,
                "training": {"seed": f"0x{train_seed:016x}",
                              "openingIds": ids,
                              "poolVersion": "generated-training-v1"},
                "validation": {"seed": f"0x{validation_seed:016x}",
                                "openingIds": validation_ids,
                                "poolVersion": "generated-training-v1"},
                "ruleModes": ["free", "forbidden"],
                "moveLimit": self.args.max_moves,
                "randomMode": "deterministic-best",
                "benchmarkIdentity": "five_chess_benchmark",
                "benchmarkPath": str(self.benchmark),
                "sourceHash": self.source_digest,
                "buildHash": self.build_digest,
            }
            atomic_write_json(round_dir / "manifest.json", manifest)
            training = self.evaluate_pair(
                parent=parent, candidate=candidate, ids=ids,
                seed=train_seed, label="training", round_dir=round_dir)
            record: dict[str, Any] = {
                "round": round_number,
                "parentNodeId": parent["nodeId"],
                "candidateNodeId": candidate["nodeId"],
                "mutationArm": arm,
                "candidateMutation": candidate_mutation,
                "training": training,
                "validation": None,
                "sourceHash": self.source_digest,
                "buildHash": self.build_digest,
            }
            if not training["candidateValid"]:
                record["status"] = "rejected"
                record["reason"] = "training cell integrity failure"
            elif not training["candidateFloorPassed"]:
                record["status"] = "rejected"
                record["reason"] = "candidate black score below 50% floor"
            elif not training["incumbentValid"]:
                record["status"] = "rejected"
                record["reason"] = "incumbent training cell invalid"
            elif not training["trainingImproves"]:
                record["status"] = "rejected"
                record["reason"] = "paired training objective did not improve"
            else:
                validation = self.evaluate_pair(
                    parent=parent, candidate=candidate, ids=validation_ids,
                    seed=validation_seed, label="validation",
                    round_dir=round_dir)
                record["validation"] = validation
                if not validation["candidateValid"]:
                    record["status"] = "rejected"
                    record["reason"] = "validation cell integrity failure"
                elif not validation["candidateFloorPassed"]:
                    record["status"] = "rejected"
                    record["reason"] = "validation black score below 50% floor"
                elif not validation["incumbentValid"]:
                    record["status"] = "rejected"
                    record["reason"] = "incumbent validation cell invalid"
                elif not validation["improves"]:
                    record["status"] = "rejected"
                    record["reason"] = "validation objective did not improve"
                else:
                    record["status"] = "promoted"
                    record["reason"] = "paired training and fresh validation passed"
            if (record["status"] == "promoted" and
                    self.args.held_out_every > 0 and
                    round_number % self.args.held_out_every == 0):
                heldout_seed = round_seed(
                    self.args.master_seed, round_number, "formal-heldout")
                heldout_ids = sample_ids(
                    heldout_seed, FORMAL_POOL_SIZE, SAMPLE_SIZE)
                record["heldout"] = self.evaluate_heldout(
                    node=candidate, round_number=round_number,
                    sample_seed=heldout_seed, ids=heldout_ids,
                    round_dir=round_dir)
            if record["status"] == "promoted":
                candidate["status"] = "promoted"
                candidate["promotionReason"] = record["reason"]
                candidate["training"] = training
                candidate["validation"] = record["validation"]
                parent = candidate
                atomic_write_json(self.current_path, parent)
                append_jsonl(self.nodes_path, candidate)
            else:
                candidate["status"] = "rejected"
                candidate["rejectionReason"] = record["reason"]
                append_jsonl(self.nodes_path, candidate)
            append_jsonl(self.rounds_path, record)
            rounds.append(record)
            self.write_report(rounds, parent)
        atomic_write_json(self.output_dir / "run_manifest.json", {
            "masterSeed": f"0x{self.args.master_seed:016x}",
            "roundsRequested": self.args.rounds,
            "sourceHash": self.source_digest,
            "buildHash": self.build_digest,
            "currentNodeId": parent["nodeId"],
            "outputDir": str(self.output_dir),
        })
        print(json.dumps({"status": "complete", "currentNodeId": parent["nodeId"],
                          "rounds": len(rounds),
                          "outputDir": str(self.output_dir)}, sort_keys=True))
        return 0


def default_output_dir() -> Path:
    return (Path.home() / "Downloads" / "logs_five_chess" /
            f"five_star_candidate_search_{datetime.now():%Y%m%d}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path,
                        default=default_output_dir())
    parser.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    parser.add_argument("--master-seed", type=lambda value: int(value, 0),
                        default=DEFAULT_MASTER_SEED)
    parser.add_argument("--rounds", type=int, default=1)
    parser.add_argument("--max-moves", type=int, default=120)
    parser.add_argument("--replay-certificates", action="store_true")
    parser.add_argument("--held-out-every", type=int, default=0,
                        help="run report-only formal checks every N rounds")
    parser.add_argument("--mutation-arm", dest="mutation_arms",
                        action="append")
    args = parser.parse_args()
    if args.rounds <= 0 or args.max_moves < 16 or args.held_out_every < 0:
        parser.error("rounds/max-moves/held-out-every have invalid values")
    args.mutation_arms = tuple(args.mutation_arms or DEFAULT_MUTATION_ARMS)
    try:
        for arm in args.mutation_arms:
            parsed = parse_mutation_spec(arm)
            if len(parsed) != 1:
                raise ValueError("each mutation arm must contain one key")
    except ValueError as error:
        parser.error(str(error))
    try:
        return SearchRunner(args).run()
    except (OSError, RuntimeError, ValueError, AssertionError) as error:
        print(f"candidate search failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
