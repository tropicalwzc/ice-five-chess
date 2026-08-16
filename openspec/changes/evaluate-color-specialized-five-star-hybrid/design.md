## Context

The current player-facing five-star is production v5.1. The revision-7 proof-engine candidate uses eight isolated root workers and a 4,500 ms internal deadline, and its strongest diagnostic signal was on the white side. Revision-7 exact evaluation regeneration failed because wall-clock completion changed some baseline, corpus, and proof outcomes; however, the product requirement now explicitly values varied play. The new evaluation must distinguish intended seeded variation among equally ranked moves from an unsafe random downgrade.

The hybrid is a policy composition, not a new search algorithm: when the hybrid owns white it uses the frozen proof-engine candidate; when it owns black it uses frozen v5.1. It must remain candidate-only until fresh stochastic strength evidence is available. Four-star and legacy three-star remain independent opponents and playable controls.

## Goals / Non-Goals

**Goals:**

- Measure whether white proof-engine plus black v5.1 produces a stronger practical five-star policy against four-star and legacy three-star in both rule modes.
- Make production-style randomness explicit, seeded, bounded to proof/evaluation-equivalent alternatives, and auditable.
- Preserve exact component search behavior: hybrid black uses direct v5.1 and hybrid white uses the direct revision-7 proof engine for the same board, rule, seed, and random mode. The candidate-only policy layer may veto a legacy score-near random selection that lacks completed-equivalence evidence and return the same component's best completed move.
- Independently validate every recorded game, move, forbidden decision, accepted certificate, selected randomness class, terminal result, and board restoration.
- Keep every decision below the existing 5,000 ms player-visible limit and retain one-game-process-at-a-time execution with eight candidate search workers.

**Non-Goals:**

- Tune either component from final match outcomes.
- Interpret uncontrolled wall-clock completion as intentional random selection.
- Require two independently seeded stochastic runs to produce identical moves.
- Add side-specific opening coordinates, opponent-specific responses, training, or live network data.
- Replace production five-star before the fresh report is complete and its gates are evaluated.

## Decisions

### 1. Route at the analysis entry point by actual stone color

Add a candidate analysis entry point and benchmark profile identity for the hybrid. `side == -1` dispatches to the frozen proof-engine candidate; `side == 1` dispatches to production v5.1. The router does not merge scores, candidates, tables, or corpus state across components. Per-step telemetry records `hybridComponent` and the exact component version. If the frozen component reports that a selected legacy random alternative is not equivalence-verified, the hybrid reruns that same component in best-completed mode and records one eligible rank; this is a policy safety veto, not a different search engine. Because production v5.1 itself has no whole-decision wall limit, candidate-only black routing adds a 4,300ms outer deadline while preserving v5.1 search parameters, leaving finalization reserve inside the 5,000ms player limit.

Alternative rejected: synthesize one profile by copying white-oriented proof parameters into v5.1. A profile struct cannot express the different composition algorithms and would not prove that black remains byte-for-byte v5.1 behavior.

### 2. Treat randomness as a constrained policy layer

Formal games use `FC_RANDOM_USER` with a logged master seed and deterministic per-game/per-decision seed derivation. A random alternative is eligible only when its completed proof class, completed scope, proof distance, tactical obligation, corpus support tier, and safety/loss status equal the best candidate. The log records whether randomness was used, eligible candidate count, selected rank, seed, and equivalence signature. A non-equivalent random choice is a correctness failure.

Repeated seeds remain a diagnostic tool: the same board and same seed should choose the same move when no wall deadline changes the completed candidate set. Separately seeded games are not expected to match move-for-move. Wall-limited differences are reported as timing variation and may not be mislabeled as proof-equivalent random selection.

Alternative rejected: accept any legal near-best move. Score proximity alone can cross a mandatory defense or completed proof boundary.

### 3. Validate stochastic games without exact regeneration

The replay authority reconstructs each game from its frozen scheduled prefix and recorded moves. It verifies schedule/color identity, seed/provenance fields, alternating side, C rule legality, forbidden-black semantics, terminal result, board integrity, every accepted certificate, and every random equivalence signature. The affected cell is invalid on any failure.

The report may run an additional independently seeded repetition to describe outcome variance, but it must not compare that repetition move-for-move or require identical proof metadata. The strength suite itself is one predeclared 100-game sample per cell; no result-selected reruns are allowed.

Alternative rejected: reuse revision-7 games by taking their white half and v5.1 games from another suite. Different openings, seeds, randomness modes, and opponent histories would destroy paired interpretation.

### 4. Freeze fresh schedules and use symmetric resource policy

After component-equivalence, randomness-class, sanitizer, golden, and latency tests pass, freeze the router, source/profile hashes, component versions, corpus, tools, worker count, randomness contract, and gates. Generate fresh rule-separated master seeds and 50 natural opening identities per rule, excluding all prior formal and diagnostic identities. Each cell exchanges color on every opening, giving 50 hybrid-white and 50 hybrid-black games.

Only one game process runs at a time. White proof-engine decisions may use eight isolated search workers; black v5.1 and frozen opponents keep their existing algorithms. This is a model-definition difference, so CPU time, wall time, workers, and peak RSS are reported rather than artificially equalizing algorithms.

### 5. Evaluate the color-specialization hypothesis directly

Reports lead with hybrid white W/D/L, Wilson interval, opponent white result, and same-color delta. Hybrid black and overall follow. Natural black advantage is retained without reweighting.

Against four-star, the practical hypothesis requires hybrid white score rate at least 50% in each rule, non-negative same-color black direction, and overall score rate at least 50% in each rule. Against legacy three-star, white and overall directions must be non-negative. Confidence intervals are reported; overlapping uncertainty is inconclusive rather than demonstrated superiority. No pooled rule or color number can hide a failing cell.

These are evaluation gates, not automatic UI promotion authority. A passing report may support a later explicit production switch; a failing report keeps v5.1.

## Risks / Trade-offs

- [Hybrid identity hides which engine moved] → Log component enum/version on every candidate step and validate it against side.
- [Randomness selects a tactically weaker move] → Require a complete equivalence signature and fail the game if any ranked field differs.
- [Wall-clock variation is confused with deliberate randomness] → Log random-selection use separately from budget exhaustion and completed candidate count.
- [Black v5.1 routing drifts] → Compare hybrid-black and direct-v5.1 golden decisions across both rules, seeds, and random modes before freeze.
- [White candidate replay remains wall-sensitive] → Replay recorded moves for correctness and certificate soundness; do not require independently seeded policy realizations to be identical.
- [Fresh stochastic sample is noisy] → Use 100 games per cell, paired color-exchanged openings, Wilson/paired intervals, and honest inconclusive classification.
- [Final outcomes cause overfitting] → Freeze before seed generation and forbid post-result parameter or routing changes.

## Migration Plan

1. Add the candidate router, component/randomness telemetry, and benchmark selector without touching the production/UI five-star entry.
2. Add component-equivalence, proof-equivalent randomness, legality, latency, golden, sanitizer, and build tests.
3. Freeze hashes and generate fresh separated schedules/seeds.
4. Run the four 100-game stochastic cells sequentially and independently replay recorded evidence.
5. Publish and checksum the complete workspace and Downloads bundles.
6. Keep v5.1 on any failure. If all evaluation gates pass, present the result for an explicit production/UI switch in a subsequent change.

## Open Questions

- The existing telemetry may not expose every equivalence field needed to audit a random choice. If so, add fields rather than infer equivalence from score proximity.
- An independent second 400-game stochastic repetition is useful for variance but is not required by the current request; the first four-cell suite remains the frozen evaluation sample.
