## Why

The 5.8.1 early micro-VCF candidate completed the requested 200-game standard follow-up with positive point estimates, zero candidate-side 5-second violations, independently replayed games, and verified early avoidance of shallow losing moves. The user has now explicitly authorized promoting this isolated candidate from research selection to the playable five-star difficulty.

## What Changes

- Bind the playable five-star UI entry used by both iPhone and iPad to `five-star-early-micro-vcf-sentinel-candidate@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`.
- Preserve exact 5.4.1 as a selectable benchmark and rollback profile rather than modifying or deleting it.
- Preserve the four-star and lower-level bindings, difficulty ordering, and persisted difficulty values.
- Add a source-level UI-binding regression and run the existing AI/profile and application build checks.
- Record the promotion evidence and an explicit one-line rollback target.

## Capabilities

### New Capabilities

- `five-star-ui-early-vcf-promotion`: Playable five-star profile selection, unchanged lower-level mappings, exact rollback identity, and promotion verification.

### Modified Capabilities


## Impact

- Changes the five-star assignment in `ice five chess/doublethree.m`, which is shared by the iPhone and iPad controller paths.
- Adds a focused source regression under `tools/` and promotion evidence under this change.
- Does not change engine search code, profile parameters, exact 5.4.1 behavior, the four-star profile, controller difficulty routing, storyboards, persisted values, or user data.
