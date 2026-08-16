## Why

The latest fork-first recovery change passed its fixed-position correctness gates
but did not produce a measurable strength gain over the four-star control; the
5.7 worker replay also showed no useful advantage from eight workers.  We need a
reproducible 5.7 baseline again before changing the scheduler, without erasing
the historical reports or the previously measured parallel/recovery work.

## What Changes

- Add an independent research-profile switch for the latest fork-first
  candidate probe and ranking behavior.
- Make the normal 5.7 profile disable that switch, restoring the pre-fork-first
  move-selection behavior while retaining its root/escape parallel proof,
  incremental forbidden-legality path, legacy recovery, and unified decision
  budget.
- Keep an explicit opt-in profile for replaying the fork-recovery experiment and
  update fork-specific tests/tools to request it deliberately.
- Include the switch in profile snapshots and add regression checks proving that
  the default 5.7 profile and the opt-in experiment are distinguishable.
- Preserve historical fixtures, manifests, reports, and reversible ledger
  accounting; do not rewrite prior evidence.

## Capabilities

### New Capabilities

- `five-star-v57-rollback-baseline`: Explicitly gates the experimental
  fork-first behavior while preserving the measured 5.7 research contract.

### Modified Capabilities

None.

## Impact

- Affects the five-star profile structure and fork-layer gates in
  `ice five chess/FiveChessAI.c` and `FiveChessAI.h`.
- Updates the C test suite, fork fixture runner, and profile serialization.
- Does not change frozen three-/four-star behavior, UI routing, game rules,
  historical reports, or the 5.7 root/escape worker implementation.
