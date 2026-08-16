## 1. Contract and telemetry

- [x] 1.1 Add scheduler/token-block diagnostics and document their meaning in
  the public diagnostics contract.
- [x] 1.2 Add the v5.7 scheduler candidate profile identity/manifest fields
  without changing the default rollback profile or frozen controls.

## 2. Persistent worker scheduler

- [x] 2.1 Implement a lazily initialized bounded pthread pool with condition
  variables, exact requested-worker completion, and a direct fallback.
- [x] 2.2 Route root proof batches through the pool while preserving private
  worker sessions, canonical job claims, and frozen-order merge semantics.
- [x] 2.3 Route escape proof batches through the same pool and preserve safe
  result publication, scoped-disproof replay, and deadline unwind.

## 3. Low-contention budget accounting

- [x] 3.1 Implement bounded aggregate and decision-ledger token block claims
  with compare-exchange caps and TLS remainders.
- [x] 3.2 Return unused block tokens at job/worker boundaries and prove that
  one-/four-/eight-worker paths cannot exceed their configured budgets.

## 4. Verification and evidence

- [x] 4.1 Add unit/stress assertions for pool reuse, requested concurrency,
  token return, determinism, board restoration, certificates, and fallback.
- [x] 4.2 Run `sh tools/run_five_chess_ai_tests.sh` with warnings as errors and
  record the result.
- [x] 4.3 Re-run the fixed 97-position v5.7 baseline at 1/4/8 workers and
  compare legality, selected moves, unknown/fallback rate, CPU/wall time,
  useful proof work, scheduler telemetry, and peak memory.
- [x] 4.4 Publish a new scheduler research report and a concise conclusion on
  whether multithreading now provides measurable useful-work or latency gain;
  do not promote it automatically.
