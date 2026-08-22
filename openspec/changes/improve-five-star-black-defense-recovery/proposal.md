## Why

The 5.8.2 soft double-three defense removes the measured single-threat regressions, but the remaining candidate-black losses expose two gaps: structural preemption can discard an original immediate blocker, and the opponent guard stops at an unresolved VCF without checking a shallow two-step fork. These gaps can turn an otherwise survivable position into an immediate loss, so the next 5.8.2 iteration should improve recovery ordering and threat coverage rather than further lowering the double-three weight.

## What Changes

- Preserve the pre-structural baseline and four-star/handoff candidates as explicit recovery candidates whenever black double-three preemption changes the selected move.
- Make opponent-guard recovery audit direct immediate-win blockers before consuming its bounded alternative budget, including the original advisory/default move when it is legal.
- Add a bounded black-side two-step opponent-fork probe for positions where the opponent VCF is unknown, and use its result to distinguish no-fork, fork, and unresolved escapes.
- Allow a bounded VCT follow-up for relevant VCF-unknown black defenses without converting unknown results into proof claims.
- Ensure structural, guard, and corpus handoff stages preserve the final candidate's proof, loss, and telemetry semantics after any recovery replacement.
- Add fixed regression fixtures from the remaining 5.8.2 candidate-black losses, including the opening-9 immediate blocker regression and the quiet white fork positions.
- Run the same paired small-match protocol against frozen four-star and exact 5.8.1, reporting black/white results, recovery classifications, fork/VCT telemetry, latency, and replay integrity.

## Capabilities

### New Capabilities

- `five-star-black-defense-recovery`: Baseline-preserving opponent-guard recovery, immediate-block priority, bounded two-step fork detection, and VCF-unknown/VCT escalation for the isolated 5.8.2 candidate.
- `five-star-black-defense-recovery-benchmark`: Regression fixtures, paired small-match evaluation, replay validation, and telemetry reporting for the recovery changes.

### Modified Capabilities

<!-- No repository-level capability specs exist yet; the completed double-three change is change-local and remains a control/reference. -->

## Impact

- Affects `ice five chess/FiveChessAI.c` and `FiveChessAI.h` opponent-guard, candidate handoff, proof-status, and profile configuration paths.
- Extends `tools/five_chess_ai_tests.c`, `tools/five_chess_benchmark.m`, and replay/evidence tooling with fixed loss-position fixtures and recovery telemetry.
- Adds a new isolated 5.8.2 research profile/version; 5.8.1, frozen four-star, lower difficulty profiles, and the playable binding remain unchanged until evaluation gates pass.
- Reuses existing rule-aware legality, VCF/VCT proof, decision-ledger, certificate-replay, and board-restoration infrastructure; no external service or learned model is introduced.
