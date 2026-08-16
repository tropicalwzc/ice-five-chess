## Context

5.4.1 is the current proof-engine candidate used by the five-star white
component and by the production profile selected in `doublethree.m`. Its
parallel root and escape searches already keep board state, DFPN graphs,
transposition tables, and certificates private to each worker. The remaining
cost is scheduler lifetime and budget coordination: the non-scheduler profile
creates a fresh pthread set for each batch and claims the shared node budget at
fine granularity.

The completed v5.7 scheduler study provides an implementation pattern already
used in this repository: a lazy process-level pool, private worker sessions,
canonical result merging, and bounded CAS token blocks. This change applies
that pattern to an opt-in profile derived directly from 5.4.1. The five-second
decision contract, legal-move rules, certificate verifier, and deterministic
root selection remain authoritative.

## Goals / Non-Goals

**Goals:**

- Provide a separately identifiable 5.4.1 scheduler candidate with the same
  search parameters and proof semantics as the baseline.
- Reuse the existing pool for root and escape batches and use a default token
  block size of 64, based on the prior fixed-position scan.
- Make requested worker counts 1, 4, and 8 observable in the direct benchmark
  route without changing the default profile.
- Verify exact budget accounting, unused-token returns, board restoration,
  certificate validity, deterministic output, legality, and the 5-second gate.
- Produce comparable four-star and legacy-three-star replay records with speed
  telemetry separated from strength results.

**Non-Goals:**

- Do not change 5.4.1 tactical weights, candidate ordering, proof depth,
  proof budget, or certificate acceptance rules.
- Do not share mutable DFPN graphs or transposition tables between workers.
- Do not add recursive branch-first scheduling or a new search heuristic in
  this change.
- Do not replace the default production route before the A/B and strength
  evidence is reviewed.

## Decisions

### 1. Derive an isolated profile

Add `fc_profile_five_star_v541_thread_scheduler_candidate()` by copying the
5.4.1 proof-engine candidate and enabling only the scheduler fields already
used by v5.7: persistent pool, token blocks, and a 64-token block. Keep the
5.4.1 tactical parameters, proof depth, aggregate node budget, legality,
memory, and recovery semantics unchanged. The scheduler profile reserves a
300 ms pool join/merge tail by using a 4,200 ms internal decision deadline,
and shortens only the per-worker emergency slice to 3,800 ms so private DFPN
arena teardown has bounded room before the five-second player-visible gate.
The first fixed matrix showed that importing v5.7's decision ledger and
recovery layers adds policy overhead to a baseline that does not use them, so
those fields remain disabled in this isolated speed study. The baseline
factory remains unchanged.

The candidate's version is distinct and its default worker count is eight.
The direct analyzer applies a validated worker override for 1/4/8 cells while
keeping the aggregate node budget fixed, so a worker-count comparison measures
scheduling rather than silently granting a different search allowance.

Alternative: mutate the 5.4.1 factory in place. Rejected because it would
invalidate the historical baseline and make a speed result impossible to
attribute to the scheduler.

### 2. Reuse the existing scheduler implementation

Route the candidate through the already implemented `fc_worker_pool_dispatch`
path for both root and escape proof batches. The pool task keeps batch data
alive until all requested workers complete, and each worker retains private
session/arena state for the batch. The existing canonical index merge remains
the source of the published proof result.

Token blocks reserve an exact bounded range with the existing atomic aggregate
counter (and the decision ledger when an enclosing caller supplies one);
workers flush and return unused tokens before completion. A failed pool
dispatch falls back to the existing direct proof path and increments the
existing fallback telemetry.

Alternative: add a second pool implementation specific to 5.4.1. Rejected
because duplicated lifecycle and synchronization code would create a separate
correctness surface without changing the measured work.

### 3. Add an explicit benchmark identity

Add a direct `five-star-v541-thread-scheduler` benchmark profile and analyzer
route. Mark the analysis result with a dedicated hybrid component identifier so
each step records the candidate version rather than looking like the old
5.4.1 component. Extend the focused C tests to compare the candidate with the
baseline and to exercise the pool/token contract on a fixed proof position.

Alternative: infer the candidate from profile flags in the generic route.
Rejected because replay scripts and reports need a stable identity and the
existing hybrid component telemetry is the repository's established pattern.

### 4. Use the existing formal replay contract

Run the same 50 formal opening prefixes with each color, in free and
forbidden-black modes, for 100 games per opponent. Validate every move with
the C rule library, require no anomalies, require every candidate step to stay
within 5,000 ms, and audit proof certificates and scheduler diagnostics.
Report p50/p95/max wall and CPU time, pool reuse, token claims/returns, unknown
and fallback counts, then report score rates separately. A point-estimate
strength change is not called a demonstrated improvement when the paired
interval crosses zero.

## Risks / Trade-offs

- [Risk] A persistent pool could retain stale task pointers or worker-local
  state. -> Keep the existing generation/completion barrier, private session
  reset, and batch wait contract; add repeated-dispatch tests.
- [Risk] Block reservations could consume more budget than the search uses. ->
  Bound each claim by the remaining aggregate/ledger budget and flush unused
  tokens before merging diagnostics; assert no budget overshoot.
- [Risk] Pool/session reuse can change the amount of useful work completed
  before the same 4.5-second reserve. -> Keep the candidate opt-in, retain the
  old profile as control, and compare unknown/fallback rates in addition to
  latency.
- [Risk] Lower CPU time may not mean lower wall time or higher playing
  strength. -> Report wall time, CPU time, useful proof work, and match scores
  as separate outcomes; do not promote on one metric.
- [Risk] Eight private arenas can increase peak memory. -> Preserve the
  existing 5.4.1 arena limits and validate memory telemetry in every replay;
  this change does not add a second memory policy.

## Migration Plan

1. Add the isolated profile, component identity, and benchmark route.
2. Run unit tests and fixed-position worker matrices against baseline 5.4.1.
3. Run free and forbidden 200-game four-star and legacy-three-star replays;
   replay them through the rule/certificate validator.
4. Keep `doublethree.m` on the old profile unless the resulting report is
   explicitly accepted for production; rollback is selecting the existing
   `fc_profile_five_star_proof_engine_candidate()` route.

## Open Questions

- Whether block size 64 remains best on the longer formal replay rather than
  only on the 97-position fixed matrix.
- Whether any scheduler speedup reduces unknown/fallback counts enough to have
  a measurable strength effect; this is an empirical result of the replay.
