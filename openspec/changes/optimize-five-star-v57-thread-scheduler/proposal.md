## Why

The rolled-back v5.7 baseline still launches a fresh `pthread_create`/`join`
set for every root and escape proof batch.  The workers also perform an atomic
global node-token operation at nearly every DFPN boundary.  On the 97-position
replay, eight workers therefore add CPU time without increasing useful proof
work: the search is paying for lifecycle and contention rather than exploring
more obligations.

## What Changes

- Add a bounded, lazily initialized eight-slot worker pool shared by root and
  escape proof batches.  A batch still owns its immutable inputs, result array,
  worker diagnostics, deadline, and stop flag; the pool only reuses OS threads.
- Dispatch only the requested 1/4/8 workers and wait for exactly that batch's
  workers before releasing stack-owned batch storage.  Preserve deterministic
  job indices and frozen-order result merging.
- Replace per-node atomic token increments in parallel batches with bounded
  local block reservations.  The reservation never exceeds the aggregate or
  decision ledger budget; unused tokens are returned when a job/worker yields.
- Use the same scheduler path for root and escape batches and expose pool reuse,
  block claims, and scheduler fallbacks in diagnostics.
- Add deterministic stress tests and rerun the fixed 97-position 1/4/8 worker
  comparison plus the existing unit/legality/certificate tests.  No strength
  promotion or change to the default v5.7 fork gate is included.

## Capabilities

### New Capabilities

- `five-star-v57-persistent-worker-scheduler`: Reusable bounded worker
  execution, exact batch completion, deterministic merging, and low-contention
  aggregate token accounting for five-star proof work.

### Modified Capabilities

None.

## Impact

- Affects the research proof dispatcher in `ice five chess/FiveChessAI.c/.h`
  and the benchmark/test telemetry in `tools/`.
- Root and escape proof semantics, profile defaults, legality rules, result
  certainty, and frozen one-through-four-star paths remain unchanged.
- Historical reports and the rollback change remain immutable; new comparison
  evidence is written under a new report directory.
