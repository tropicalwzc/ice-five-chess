## Why

The v5.7 root-parallel scheduler can run many independent root obligations, but the fixed-position replay shows that extra workers mostly duplicate short or early-stopped work.  Long VCF/VCT lines still spend the five-second decision window serially, so the saved scheduler overhead does not become deeper useful search or a measurable advantage over the four-star control.

This change introduces a v5.7 research candidate that ranks shallow tactical previews first, keeps root proof ordering deterministic, and uses the existing bounded pool only for sufficiently long recursive sibling branches.  The candidate may search a deeper VCT horizon, while the decision ledger, certificate checks, legality rules, and five-second hard gate remain authoritative.

## What Changes

- Add an opt-in v5.7 branch-first proof profile; keep the frozen v5.7 and persistent root-scheduler profiles unchanged.
- Add deterministic shallow previews for advanced-four/advanced-three threat branches and use their score, tactical severity, refutation width, and dependency locality to order deep work.
- Replace indiscriminate root dispatch in the candidate path with serial root/obligation ordering and bounded recursive branch waves.
- Add a pool task path for long recursive sibling branches with private board/session/certificate state, deterministic result merge, and no nested pool dispatch from a pool worker.
- Allow the branch-first candidate to raise proof depth and budget within the shared decision ledger and the five-second hard deadline.
- Give eligible forcing continuations an adaptive tactical extension: advanced-four branches receive `+2` plies, advanced-three branches receive `+1`, and both remain capped at 16 plies.
- Add diagnostics for previewed branches, branch waves, branch jobs, depth reached, useful branch work, fallback, and deadline/ledger termination.
- Extend fixed-position and strength replay tooling so branch-first results can be compared with the existing v5.7 baseline on worker counts 1/4/8.

## Capabilities

### New Capabilities

- `five-star-v57-branch-first-proof`: Deterministic shallow-preview ordering and bounded recursive sibling parallelism for the opt-in v5.7 proof candidate.

### Modified Capabilities

<!-- No existing repository capability specification is present; the default v5.7 contract is intentionally not modified. -->

## Impact

- `ice five chess/FiveChessAI.c` and `.h`: profile fields, branch scheduler, private worker tasks, certificate/result merge, and diagnostics.
- `tools/five_star_fork_fixture_runner.c` and replay/report scripts: branch-first benchmark selection and telemetry validation.
- New research artifacts under `openspec/changes/optimize-five-star-v57-branch-first-search/` and `reports/five_chess/`.
- No new runtime dependency; the implementation continues to use the existing pthread pool and C11 atomics.
