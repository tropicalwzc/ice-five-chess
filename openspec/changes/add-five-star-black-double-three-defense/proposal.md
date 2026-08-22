## Why

The 5.8.1 five-star profile can recognize some three-based opponent threats, but it does not make the exact “one white move creates two open threes” condition a black-side defensive obligation. Its VCT escalation is also gated by a completed VCF disproof, so a VCF `unknown` can leave a double-three-producing move in the selected candidate. The change adds a bounded, rule-aware preemption layer and measures it against both the frozen four-star control and the current 5.8.1 profile.

## What Changes

- Add an isolated 5.8.2 five-star candidate derived from 5.8.1.
- Add a rule-aware scan matching the existing `doublethreetest` semantics: count white reply moves that create open threes in at least two directions with both ends open.
- During black decisions, when no independently verified own VCF is available, prioritize legal candidates that eliminate all detected white double-three gains; preserve honest `unknown` behavior for incomplete scans or unresolved broader proofs.
- Add targeted candidate generation, black-only defensive telemetry, board-restoration checks, symmetry/rule fixtures, and false-positive coverage for single open threes.
- Add two paired small-match evaluations using identical openings, colors, seeds, and frozen controls:
  - 5.8.2 versus frozen four-star.
  - 5.8.2 versus 5.8.1.
- Keep the current playable five-star binding and all lower difficulty profiles unchanged until correctness, latency, and evaluation gates pass.

## Capabilities

### New Capabilities

- `five-star-black-double-three-defense`: Exact white double-three detection, black-side preemption, VCF-priority interaction, bounded candidate selection, fail-closed semantics, and telemetry.
- `five-star-double-three-benchmark`: Paired small-match protocols for 5.8.2 versus frozen four-star and 5.8.2 versus 5.8.1, including replay, color separation, tactical incidence, and latency reporting.

### Modified Capabilities

None.

## Impact

- Affects `ice five chess/FiveChessAI.c`, `FiveChessAI.h`, the C regression suite, benchmark profile selection/serialization, and change-local evaluation evidence.
- Adds a new research profile and benchmark selectors without changing the 5.8.1 or frozen four-star controls.
- Reuses the existing rule-aware legality, VCF/VCT proof, decision ledger, and certificate replay infrastructure; no learned weights, network dependency, or runtime opening table is added.
- A later promotion may update the iPhone/iPad five-star binding, but this proposal does not make that binding change part of the initial candidate implementation.
