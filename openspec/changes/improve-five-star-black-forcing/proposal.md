## Why

The rule-partitioned five-star benchmark exposed a deterministic failure mode in every reviewed five-star-black loss: the engine verified that its default allowed an opponent VCT, searched only six heuristic alternatives, and then retained a known-losing move when no alternative completed a scoped disproof. The same failures persisted with the corpus disabled, so the next strength gain must improve proof-guided escape and proactive black forcing rather than tune opening-book acceptance against the old opponents.

## What Changes

- Turn the prior five-star-black losses into symmetry-expanded diagnostic/regression positions without treating their opening IDs or outcomes as production rules or final strength evidence.
- When a default move has a verified opponent win, derive targeted escape candidates from the proof certificate, widen beyond the first six heuristic moves, and rank `own proven win > opponent win disproved in comparable scope > unknown > verified loss`.
- If every examined move is verified losing, select the longest verified survival; never silently label a merely exhausted/no-immediate-safe position as a mathematical proven loss.
- Complete most-proving-node DFPN behavior and reuse proof/transposition work across alternatives so defensive widening remains within mobile budgets.
- Add a bounded proactive threat-space stage for quiet moves that create multiple independent future forcing continuations, while retaining independent certificate verification and fail-closed behavior.
- Prevent corpus advice from replacing a verified four-star forced defense unless the candidate has equivalent or better completed proof evidence.
- Make evaluation decisions independent of the frozen legacy advisor's random seed; keep user randomness only among proof-equivalent choices.
- After all parameters are frozen using non-final diagnostics, generate untouched natural schedules and run four 100-game cells: forbidden/no-forbidden versus frozen four-star and frozen legacy three-star, each split 50 five-star black and 50 five-star white.
- Publish color-correct Markdown/JSON/raw reports with black-loss diagnostics, white non-regression, paired uncertainty, legality/proof replay, latency, commands, provenance, and checksums under the workspace and `/Users/wangzicheng/Downloads/logs_five_chess`.

## Capabilities

### New Capabilities

- `five-star-loss-aware-forcing`: Certificate-directed escape widening, completed proof-class ordering, proactive quiet-threat search, deterministic evaluation behavior, and corpus protection for verified defenses.
- `five-star-black-strength-benchmark`: Failure-regression diagnostics plus fresh rule-separated 100-game comparisons against frozen four-star and legacy three-star with black-first reporting and white non-regression.

### Modified Capabilities

None. The relevant predecessor specifications remain change-local and have not been archived into `openspec/specs`.

## Impact

- Affects `FiveChessAI.c/.h`, benchmark telemetry/runner and report tooling, proof/threat tests, and OpenSpec/report artifacts.
- Keeps one-to-four-star behavior and the curated corpus immutable; five-star remains the only player level receiving the new portfolio.
- Adds no training model, opponent-specific runtime rule, live network dependency, or final-schedule-derived opening move.
