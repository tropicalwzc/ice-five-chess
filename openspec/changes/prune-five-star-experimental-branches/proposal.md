## Why

The playable five-star difficulty is frozen at 5.8.1, but the engine and research tools still expose historical and post-promotion experimental variants. The user now explicitly requests their removal, superseding the earlier requirement to keep exact 5.4.1 selectable for rollback.

## What Changes

- Flatten the current 5.8.1 parameters so production no longer inherits from retired factories.
- **BREAKING** Remove obsolete five-star factories, experimental routes, dedicated code and research executables; retain only current five-star selection.
- Preserve lower difficulties and shared rule/search primitives required by them or 5.8.1.
- Preserve historical spec/evidence records; replace obsolete runnable checks with current-version regression coverage.

## Capabilities

### New Capabilities

- `five-star-production-only`: Single frozen five-star version with dependency-safe retirement and regression verification.

### Modified Capabilities

## Impact

FiveChessAI C API/implementation, doublethree binding where needed, tests and tools. No SwiftUI, persistence, difficulty mapping, corpus or tuning changes. Historical rollback instructions become archival only; recovering old variants requires Git history.
