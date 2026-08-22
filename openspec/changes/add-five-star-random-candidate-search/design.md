## Context

The current benchmark can generate deterministic openings from a master seed, but its normal interface evaluates a contiguous opening range and its profile factories are compiled-in identities. The failed recovery candidate shows why a fixed, hand-picked match is insufficient: it dropped below 50% as black against the frozen four-star model in both tested rule modes. The next research loop needs reproducible randomness, paired comparisons, and a durable incumbent record rather than another one-off profile.

The search is a local research workflow. It must not alter the playable profile binding, the exact 5.8.1 control, or the frozen four-star opponent. The existing formal opening schedule remains held out from candidate selection.

## Goals / Non-Goals

**Goals:**

- Sample exactly 12 distinct training openings per round with a deterministic seed and record the selected IDs.
- Compare a mutated candidate and its incumbent on the same openings against the same frozen four-star profile.
- Use black W/D/L and score rate as the primary objective, reject candidates below 50%, and require fresh-sample validation before promotion.
- Keep no-forbidden and forbidden-black results separate and prevent a shared incumbent from improving one rule mode by silently degrading the other.
- Store an append-only candidate-node ledger that supports resume, replay, provenance, and inspection of rejected candidates.
- Allow one-factor parameter or method mutations without editing source files for each round.

**Non-Goals:**

- Do not implement a neural network, gradient learner, or online self-play training system.
- Do not change Gomoku legality, VCF/VCT proof semantics, the 5.8.1 control, or the four-star opponent.
- Do not use the formal held-out opening schedule for mutation selection.
- Do not promote a research node to the playable binding in this change.

## Decisions

### 1. Use a dedicated search driver with explicit benchmark sampling

Add a small search driver under `tools/` and extend the benchmark interface with an explicit opening-ID list or equivalent sampled-opening mode. The driver owns rounds, mutations, paired runs, validation, and ledger writes; the benchmark remains the source of game execution and JSONL step telemetry. This keeps the search policy out of engine decision code and makes a round reproducible from its manifest.

The driver uses the existing deterministic PRNG family (or an equivalent fixed-width SplitMix64 implementation) with a round seed. It samples without replacement from generated training openings `0..99`; formal held-out prefixes are not eligible for training samples.

### 2. Keep paired common-random-number comparisons

For every candidate round, the incumbent and candidate use the identical ordered opening IDs, rule mode, four-star profile identity, random mode, move limit, and engine build. This is preferred over independently sampled candidates because the difference between two 12-game results is otherwise dominated by opening variance. The sample seed and full ID list are written before either run starts.

The search may run black-only games for the fast training gate because the objective is candidate-as-black strength. White games remain an optional safety/diagnostic phase and are required for a promotion report when a mutation changes color-shared search behavior.

### 3. Define a conservative promotion score

For each rule mode, score a node as `(wins + 0.5 * draws) / games`. A candidate is immediately rejected when its black score is below `0.50`, or when any game is anomalous, violates the hard decision limit, fails replay, or contains an invalid certificate. A training winner must strictly improve the incumbent on the paired sample; ties remain with the incumbent.

An apparent training winner is then run against the four-star model on a fresh, non-overlapping 12-opening validation sample. The shared champion is promoted only when both no-forbidden and forbidden-black validation gates reach 0.50 and the candidate improves the incumbent's lexicographic objective `(min(rule scores), average(rule scores), total score)`. If a rule-specific branch is intentionally retained, it must have a separate node lineage and cannot replace the shared champion.

### 4. Restrict mutations to one factor per child node

Each child starts from the current validated champion and changes one declared parameter or policy arm: for example a double-three defense weight, immediate-block switch, fork-probe budget, VCT-on-unknown switch, or recovery ordering. The canonical mutation manifest is hashed with the source/build hash to form the node identity. Multi-factor experiments are allowed only as a later child of individually measured factors and must still be recorded as one explicit mutation.

The initial incumbent is the exact 5.8.1 candidate profile. The previously measured recovery profile is recorded as a rejected historical result and is not used as the starting champion.

### 5. Use an append-only node and round ledger

The driver writes a machine-readable ledger containing `nodeId`, `parentNodeId`, profile snapshot/hash, mutation, source/build hashes, rule mode, round/validation seeds, opening IDs, raw JSONL paths, W/D/L, score rate, latency percentiles, anomaly counts, replay status, and promotion reason. A separate current-pointer file identifies the latest validated champion. Writes are staged and finalized only after all required paired/validation files pass integrity checks.

The output root defaults to the existing `Downloads/logs_five_chess` hierarchy with a dated search directory, while a command-line path can be used for local smoke tests. Existing standard evidence directories are never overwritten.

### 6. Add periodic held-out checks without contaminating search

After a configurable number of successful promotions, or at explicit user request, the current champion is evaluated on a fresh sample from the formal held-out opening set. Held-out results are reporting evidence only and never feed mutation selection or the champion objective. This prevents repeated random selection from becoming overfitting to the generated training pool.

## Risks / Trade-offs

- [Risk] Twelve games have high binomial variance and a lucky candidate can appear better. → Use paired samples, a fresh validation sample, strict ties-to-incumbent behavior, and periodic held-out checks.
- [Risk] Requiring both rule modes can reject a useful rule-specific improvement. → Preserve rule-specific branches with separate lineage while keeping the shared champion conservative.
- [Risk] Profile override plumbing can accidentally change controls. → Start from an explicit profile snapshot, reject unknown fields, and assert control/four-star profile hashes in every header.
- [Risk] Repeated candidate runs are expensive. → Train black-only, cache identical node/sample evaluations, and reserve full color evaluation for promotion/safety reports.
- [Risk] Search results can be irreproducible if the opening generator changes. → Record source/build hashes, PRNG seed, opening IDs, profile snapshot, and benchmark command for every round.

## Migration Plan

1. Add the search manifest, profile mutation/override surface, sampled-opening benchmark mode, and ledger schema behind research-only tooling.
2. Seed the ledger with exact 5.8.1 as the validated incumbent and import the failed recovery result as rejected evidence.
3. Run a two-round smoke search with a fixed seed, then verify that repeating the same manifest produces identical openings and decisions.
4. Run random training/validation rounds against the frozen four-star model for both rule modes and retain only validated champion nodes.
5. Run periodic held-out checks and write reports under a new dated Downloads directory; do not update the playable binding.
6. Roll back by selecting the previous validated node or abandoning the search directory; engine controls remain unchanged.

## Open Questions

- Should the first production search use 12 black-only games per rule mode or 12 paired black/white games for every mutation?
- What promotion margin beyond a strict score improvement is affordable once the first validated champion is established?
- Which one-factor mutation arms should be enabled in the initial search manifest, and which should remain diagnostic-only?
