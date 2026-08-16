## Why

The unpromoted v5.2.1 five-star candidate widened proof-guided escape search but completed zero scoped disproofs across the formal 400-game run, while same-seed diagnostics increased p95 decision latency by about 17%–23%. The next five-star gain must reduce per-node and repeated-proof cost, then spend the recovered budget on sound relevance-guided and proactive threat search instead of increasing uniform depth or wall-clock limits.

## What Changes

- Make this change the implementation authority for the unfinished proof-efficiency, initiative-search, and associated correctness work left by `improve-five-star-black-forcing`; reuse its completed diagnostics and loss-aware semantics without promoting its rejected v5.2.1 profile.
- Add an incremental five-star position state with make/unmake updates for Zobrist keys, occupied/frontier masks, affected line facts, immediate-win/threat masks, and rule-aware legality facts, eliminating repeated full-board hashing, copying, and nested threat scans where equivalence can be verified.
- Add one bounded proof session per five-star decision so own-win checks, the frozen four-star baseline move, opponent checks, escape alternatives, and corpus alternatives share rule/search-aware transpositions and allocations.
- Add deterministic root-split parallel DFPN for the candidate five-star so independent forcing roots can use multiple CPU cores under one absolute decision deadline and one predeclared aggregate node/memory budget; each worker owns its mutable position, graph, and TT, and only independently verified results are merged.
- Replace post-hoc proof-number accounting over ordinary depth-first traversal with deterministic thresholded DFPN over an explicit AND/OR proof graph, including most-proving-child selection, saturated updates, completed-scope semantics, and independently replayable certificates.
- Add sound relevance-zone and dependency-based threat-space search using gain/cost/rest, counter-threat categories, line dependencies, and iterated related zones; omitted defender moves must be proven irrelevant or the result remains unknown.
- Add a staged forcing portfolio of immediate tactics, VCF, VCT/dependency search, and a bounded implicit/quiet-threat setup stage. Quiet moves may override the baseline only with completed verified evidence and may not consume an unconditional larger global budget.
- Preserve the current production four-star as an immutable opponent/player level and preserve frozen legacy three-star plus one/two-star behavior, parameters, UI mappings, persistence, opening behavior, and user randomness rules.
- Establish fixed-position performance evidence against production five-star v5.1 before strength testing, covering node throughput, proof completion, transposition reuse, parallel utilization, allocation/full-scan counters, p50/p95/max latency, both rule modes, and correctness replay; require every candidate decision to return its best completed legal move within a five-second player-visible hard limit.
- After code and parameters are frozen on non-final diagnostics, run four fresh 100-game cells: no-forbidden/forbidden versus frozen four-star and frozen legacy three-star, each with 50 new-five-star-black and 50 new-five-star-white games, for 400 total games.
- Publish color-correct Markdown/JSON/raw reports with white results explicitly reported, paired uncertainty, legality and certificate replay, latency/proof telemetry, commands, provenance, and checksums under the workspace and `/Users/wangzicheng/Downloads/logs_five_chess`.

## Capabilities

### New Capabilities

- `five-star-incremental-proof-engine`: Five-star-only incremental position state, reusable proof sessions, thresholded DFPN, sound relevance/dependency threat search, staged quiet initiative, certificate verification, and fail-closed budget behavior.
- `five-star-proof-performance-benchmark`: Reproducible micro/per-position performance gates plus fresh rule- and color-separated 400-game strength evaluation against immutable four-star and legacy three-star baselines.

### Modified Capabilities

None. The predecessor capabilities are still change-local and have not been archived into `openspec/specs`; this change consumes their frozen evidence while defining the replacement requirements explicitly.

## Impact

- Affects `FiveChessAI.c/.h`, five-star analysis composition, proof/threat state structures, benchmark telemetry/runners, deterministic diagnostic fixtures, test tooling, and generated reports.
- Keeps the current production v5.1 available as rollback and latency/proof-efficiency baseline until all promotion gates pass.
- Keeps the frozen four-star and legacy three-star profile identities, decision behavior, golden fixtures, UI mappings, and formal-control provenance unchanged, and continues to expose four-star as a playable difficulty and formal opponent.
- Adds no training-data dependency, neural model, live network dependency, opponent-specific runtime line, or result-selected production coordinate.
