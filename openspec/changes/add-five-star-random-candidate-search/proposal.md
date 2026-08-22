## Why

The baseline-preserving recovery candidate regressed badly in the first standard cells: it won only 5/12 games as black without forbidden moves and 4/12 with forbidden moves against the frozen four-star model. A single fixed matchup is not enough to guide further tuning, so the research workflow needs reproducible random small-sample search with paired comparisons, an explicit 50% black-win floor, and independent validation before promoting a candidate.

## What Changes

- Add a deterministic random sampler that selects 12 distinct training openings per search round and records the seed and exact opening IDs.
- Add candidate-node metadata for parent, mutation, profile snapshot/hash, training sample, validation sample, W/D/L, score rate, latency, anomalies, and promotion status.
- Evaluate a candidate and its incumbent on the same sampled openings against the frozen four-star model to reduce sampling noise.
- Reject candidates with black score rate below 50%, hard-limit violations, invalid games, or replay/certificate failures.
- Validate a surviving candidate on a fresh independent sample before making it the new incumbent; preserve the best validated candidate as the next search node.
- Expose controlled one-factor parameter/method mutations for recovery weight, immediate-block policy, fork probing, and VCT escalation without changing the playable binding.
- Track forbidden and no-forbidden rules separately and require both rule modes to pass their promotion gates unless a branch is explicitly retained as rule-specific research.
- Add reports and replay checks for each iteration so random search remains reproducible and does not overfit a single opening subset.

## Capabilities

### New Capabilities

- `five-star-random-candidate-search`: Reproducible random opening sampling, one-factor candidate mutation, incumbent/champion node management, promotion gates, and search provenance.
- `five-star-candidate-evaluation`: Paired 12-opening four-star evaluation, independent validation, forbidden/no-forbidden score aggregation, replay integrity, and iteration reports.

### Modified Capabilities

<!-- No repository-level capability specs exist yet; this change introduces a research workflow without changing the playable profile contract. -->

## Impact

- Extends `FCAIProfile` construction and benchmark tooling so candidate parameters can be overridden and serialized without creating ad-hoc source edits for every round.
- Adds a small-sample search driver/reporting tool and JSONL metadata for sample seeds, opening IDs, candidate ancestry, and promotion decisions.
- Reuses the existing frozen four-star profile, opening generator, replay/certificate validation, and Downloads evidence layout.
- Does not update the playable five-star binding, 5.8.1 controls, frozen four-star model, or formal held-out opening schedule.
