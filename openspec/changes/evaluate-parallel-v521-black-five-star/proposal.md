## Why

The latest color-specialized candidate improved white play, but its production-v5.1 black component did not reproduce the stronger black-side signal previously observed with v5.2.1. We need a controlled candidate that keeps the latest white engine, upgrades black to v5.2.1, and gives v5.2.1 the same eight-thread root-search opportunity while preserving the five-second move ceiling.

## What Changes

- Add a benchmark-only five-star hybrid that routes white to `5.4.1-transactional-deadline-root-parallel-5s` and black to a new parallelized derivative of `5.2.1-certificate-dependency-widening`.
- Separate parallel-root capability from the broader proof-engine-candidate feature set so v5.2.1 can partition completed proof roots across up to eight workers without silently inheriting unrelated 5.4.1 policy changes.
- Enforce a shared decision deadline with best-completed legal fallback so every white or black move returns within 5,000 ms.
- Keep frozen four-star byte-for-byte single-threaded and keep production/UI five-star unchanged during candidate evaluation.
- Run a same-schedule one-worker versus eight-worker v5.2.1-black A/B, then fresh 100-game Freestyle and 100-game forbidden matches against frozen four-star, each with 50 candidate-white and 50 candidate-black games.
- Report hybrid-white, hybrid-black, same-color opponent results, overall score, paired uncertainty, worker activity, CPU/RSS, timeouts, routing, legality, and whether eight-worker black search improves the same-schedule advantage.

## Capabilities

### New Capabilities

- `parallel-v521-black-five-star-hybrid`: Color routing, isolated v5.2.1 parallel-root search, eight-worker resource limits, completed-result merging, deadlines, telemetry, and production-level isolation.
- `parallel-v521-four-star-benchmark`: Same-schedule one-versus-eight-worker black A/B plus fresh Freestyle/forbidden strength evaluation against frozen single-thread four-star.

### Modified Capabilities

None. Existing production and prior change-local five-star capabilities remain unchanged; this change introduces a new candidate and evaluation contract.

## Impact

- Affects `FiveChessAI.c/.h`, benchmark profile/routing and JSONL telemetry, strict/sanitizer/race fixtures, schedule generation/audit, replay/report tools, and a new checksummed report bundle.
- Adds no training, live network dependency, opponent-specific move table, or four-star resource change.
- Does not authorize a UI switch; production remains v5.1 until a later explicit promotion decision.
