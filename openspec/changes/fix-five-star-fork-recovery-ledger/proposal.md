## Why

The final 5.7 recovery-budget run did not show a meaningful strength gain over
the frozen four-star control: the no-forbidden cell was 50/1/49 versus 49/1/50,
and the forbidden cell was 51/1/48 versus 48/1/51, with paired uncertainty
spanning zero.  The failure audit found 97 fallback games, all lost after the
opponent already had two immediate winning points; all 97 legacy hints were
legal, while the fallback move was almost always `(0,0)` and never entered the
parallel escape path.

The audit also found that proof-session memory reservations accumulate because
session teardown frees allocations without returning the reservation to the
decision ledger.  The ledger consequently approaches its 256 MiB limit and
turns later proof work into `unknown`.  Fixing this accounting and defending
against opponent forks before proof certainty is reached is necessary before
another strength comparison can measure playing strength rather than search
exhaustion.

## What Changes

- Return per-session memory reservations on every completed, cancelled, or
  failed proof session, while retaining separate live, peak, and exhausted
  telemetry for one five-star decision.
- Add a fork-first tactical layer that evaluates whether a candidate leaves the
  opponent two legal immediate winning replies, including when deeper proof is
  `unknown` or the four-star handoff fails.
- Keep the best legal baseline and ordered defensive candidates alive through
  recovery; do not turn an incomplete candidate list into a proven loss or
  fall directly to the first legal coordinate when a fork-safe candidate is
  available.
- Replay the 97 audited fallback positions as fixed fixtures and report fork
  avoidance, legal fallback, proof-session reuse, and unknown-rate metrics.
- Run deterministic 1/4/8-worker comparisons on the same positions and seeds,
  measuring useful proof work and decision outcomes.  Full shared-TT,
  virtual-PN, and parallel-dovetailing architecture is explicitly deferred to
  a later change.

## Capabilities

### New Capabilities

- `five-star-fork-recovery`: Pre-proof opponent-fork detection, ordered
  defensive candidate retention, and a legal, deterministic recovery result.
- `five-star-decision-ledger`: Reversible per-decision accounting for proof
  session memory, live/peak usage, exhaustion, and unknown-result attribution.
- `five-star-worker-evaluation`: Reproducible fixed-position and 1/4/8-worker
  evaluation with fork, fallback, budget, and effective-work metrics.

### Modified Capabilities

None.  No archived capability spec currently defines these requirements; the
 frozen three-star/four-star controls and the production route remain outside
 this change.

## Impact

- Affects the five-star research path in `ice five chess/FiveChessAI.c` and its
  internal headers, especially proof-session lifecycle, recovery candidate
  selection, and tactical legality checks.
- Adds fixed-position fixtures and benchmark/report tooling using the existing
  experiment logs under `/Users/wangzicheng/Downloads/logs_five_chess`.
- Changes research-profile telemetry and diagnostics only; it does not change
  the frozen four-star implementation, UI mapping, production route, or game
  rules.
- Adds no external dependency, training data, opponent-specific move table, or
  automatic promotion of the 5.7 profile.
