## Why

Revision-7 diagnostics indicate that the proof-engine candidate is especially valuable as white, while production five-star v5.1 remains a stable black policy. A color-specialized hybrid may combine those complementary strengths, and its production behavior should be evaluated as a seeded stochastic policy rather than rejected merely because independent runs choose different proof-equivalent moves.

## What Changes

- Add a candidate-only five-star hybrid that routes white decisions to `5.4.1-transactional-deadline-root-parallel-5s` and black decisions to frozen production `5.1.0-elite-rule-partitioned-local-v2`.
- Preserve production five-star v5.1, frozen four-star, legacy three-star, UI mappings, persistence, and all one-through-four-star paths until the hybrid passes its own gates.
- Exercise the hybrid in production-style seeded user randomness. Random selection is permitted only among candidates equal in completed proof class, scope, distance, tactical obligation, and corpus support; every seed and selected candidate rank is logged.
- Replace exact move-for-move regeneration as a stochastic strength gate with independent legality, forbidden-rule, terminal-result, certificate, provenance, randomness-class, and board-integrity replay. Different legal proof-equivalent choices across separately seeded runs are expected and reported.
- Freeze the hybrid router, both component profiles, randomness contract, opponents, schedules, tools, and promotion criteria before final games.
- Run four fresh 100-game cells: no-forbidden/forbidden versus frozen four-star and legacy three-star, each with 50 hybrid-black and 50 hybrid-white games, then publish color-correct Markdown/JSON/raw reports.
- Treat hybrid white as the primary hypothesis and hybrid black as a non-regression check against the expected v5.1 component behavior; do not pool rules or colors to hide a failing cell.

## Capabilities

### New Capabilities

- `color-specialized-five-star-hybrid`: Candidate routing, component identity, proof-equivalent seeded randomness, fallback, telemetry, and lower-level isolation for a white-proof-engine/black-v5.1 five-star policy.
- `seeded-stochastic-five-star-benchmark`: Fresh color- and rule-separated strength evaluation that validates stochastic policy constraints and game correctness without requiring separately seeded games to choose identical moves.

### Modified Capabilities

None. The predecessor five-star capabilities remain change-local; this change consumes their frozen profiles and evidence without altering their requirements or retroactively reclassifying revision-7 deterministic evaluation.

## Impact

- Affects `FiveChessAI.c/.h`, benchmark profile selection/routing, per-step provenance and random-selection telemetry, stochastic replay/report tooling, new OpenSpec fixtures, and report bundles.
- Adds no training, live network dependency, opponent-specific runtime line, or color-dependent coordinate table.
- Keeps the player-facing five-star on v5.1 until the hybrid passes correctness, latency, stochastic-class, four-star, and legacy-generalization gates.
