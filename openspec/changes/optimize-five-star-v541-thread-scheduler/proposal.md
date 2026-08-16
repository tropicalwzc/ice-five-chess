## Why

The selected 5.4.1 proof engine already uses independent root and escape
workers, but every proof batch can pay thread creation, proof-arena setup, and
per-node global token contention. Earlier v5.7 scheduler experiments showed
that a persistent bounded pool and block-based token reservations reduce this
overhead without changing proof semantics, so the same technique should be
measured directly on the 5.4.1 model.

## What Changes

- Add an opt-in 5.4.1 scheduler profile using the existing persistent worker
  pool, private worker sessions, and bounded token blocks.
- Use a block size selected from the prior fixed-position scan (64 by default)
  while retaining exact aggregate budget and deadline enforcement.
- Add a direct benchmark route and worker-count override for the 5.4.1
  scheduler candidate so it can be compared with the unchanged 5.4.1 profile.
- Add focused profile, pool-reuse, token-return, legality, certificate, and
  deterministic-result coverage.
- Run the fixed-position speed comparison and the same 200-game natural-prefix
  matches against frozen four-star and legacy three-star controls.
- Keep the default 5.4.1 production route unchanged until the measurements
  clear the existing correctness and performance gates.

## Capabilities

### New Capabilities

- `five-star-v541-thread-scheduler`: An opt-in, semantics-preserving scheduler
  profile for speeding up 5.4.1 proof searches with persistent workers and
  block-based token reservations.

### Modified Capabilities

No existing spec-level capability is modified; the default 5.4.1 profile and
its production routing remain the comparison baseline.

## Impact

- `ice five chess/FiveChessAI.c` and `FiveChessAI.h`: profile factory,
  analyzer route, and public declarations.
- `tools/five_chess_benchmark.m` and AI tests: profile selection, fixed worker
  matrix, and contract assertions.
- New replay/report artifacts under `reports/five_chess/`.
- No new dependency and no persistent data migration.
