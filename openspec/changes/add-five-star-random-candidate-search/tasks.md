## 1. Candidate model and mutation surface

- [x] 1.1 Define the research candidate manifest, canonical profile serialization, source/build hash, and exact 5.8.1 incumbent seed without changing playable or control bindings.
- [x] 1.2 Add validated one-factor mutation arms for double-three weight, immediate-block policy, fork-probe budget, VCT-on-unknown policy, and recovery ordering.
- [x] 1.3 Reject unknown mutation fields and out-of-range values before launching a game, with deterministic node IDs derived from parent and mutation.

## 2. Reproducible opening sampling

- [x] 2.1 Implement deterministic, without-replacement sampling of exactly 12 generated training opening IDs from a recorded round seed.
- [x] 2.2 Add disjoint fresh validation sampling and protect the formal held-out opening set from training selection.
- [x] 2.3 Serialize the ordered opening IDs, pool version, seed, rule mode, move limit, random mode, and benchmark identity in every round manifest/header.

## 3. Paired benchmark runner

- [x] 3.1 Extend benchmark execution to accept an explicit opening-ID sample and a black-only research mode while preserving existing contiguous schedules and standard suites.
- [x] 3.2 Run candidate and incumbent against the exact frozen four-star profile with identical sample, rule, seed, and build inputs.
- [x] 3.3 Emit raw JSONL and profile/opponent snapshots for each candidate/incumbent cell without overwriting existing Downloads evidence.
- [x] 3.4 Add machine-readable round summaries with W/D/L, score rate, latency percentiles, hard-limit counts, and anomaly counts.

## 4. Champion ledger and promotion gates

- [x] 4.1 Implement an append-only node/round ledger with parent lineage, mutation, sample manifests, evidence paths, replay digests, and promotion/rejection reasons.
- [x] 4.2 Initialize and resume the current champion from exact 5.8.1; keep the failed recovery profile as rejected evidence rather than as a parent.
- [x] 4.3 Enforce the per-rule black score floor of 0.50 and reject ties or lower paired training scores in favor of the incumbent.
- [x] 4.4 Run fresh independent validation before promotion and compare the shared objective across no-forbidden and forbidden-black modes.
- [x] 4.5 Preserve explicit rule-specific branches without allowing them to replace the shared champion.

## 5. Integrity and anti-overfitting checks

- [x] 5.1 Reuse the existing replay/certificate/board-restoration validators and make any anomaly or hard-limit violation invalidate the cell for promotion.
- [x] 5.2 Add periodic formal held-out checks that report results but never feed mutation selection or the champion objective.
- [x] 5.3 Cache identical node/sample evaluations and make interrupted rounds resumable from finalized ledger entries only.
- [x] 5.4 Generate a human-readable iteration report showing champion history, rejected candidates, rule-mode trade-offs, and sample provenance.

## 6. Tests and acceptance evidence

- [x] 6.1 Add unit tests for deterministic sampling, uniqueness, training/held-out separation, mutation validation, score calculation, and promotion decisions.
- [x] 6.2 Add a fixed-seed integration smoke test proving candidate/incumbent pairing, reproducible headers, and unchanged control profile snapshots.
- [x] 6.3 Run a small multi-round search from 5.8.1 and verify that a below-50% black candidate is rejected and cannot become the current pointer.
- [x] 6.4 Run replay and certificate-integrity checks over all accepted cells and exclude invalid games from strength totals.
- [x] 6.5 Run `openspec validate add-five-star-random-candidate-search --strict`, `git diff --check`, and the complete targeted C/Python test commands.
- [x] 6.6 Record final source/build hashes and preserve all search outputs under a new dated `Downloads/logs_five_chess` directory without updating the playable binding.
