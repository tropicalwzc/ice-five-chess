## Why

The 5.7 diagnostic run improved the aggregate score only modestly, while exposing
search failures that are unrelated to chess strength: a truncated candidate set
can be reported as a proven loss, a failed four-star hint aborts five-star
recovery, and the root parallel dispatcher repeats the same threat enumeration
inside every worker.  Under the forbidden rule the resulting legality scans and
board copies consume the five-second decision window, so the engine often returns
`unknown` without giving the recovery search a fair chance.

This change makes those outcomes explicit and spends one shared, deterministic
decision budget on sound recovery and proof work.  It is needed before another
5.7 strength run; otherwise a win/loss report would conflate search incompleteness
with the candidate's actual playing strength.

## What Changes

- Add explicit decision outcomes for no-legal-move, verified loss, verified win,
  and deadline/unknown, with a best legal fallback for every non-terminal
  position.
- Keep five-star recovery alive when the frozen four-star hint or a proof stage
  returns unknown; search certificate-dependent, tactical, nearby, and full-board
  legal candidates in ordered layers and stop only on an independently replayable
  disproof.
- Replace root overlap groups with independent single-gain proof jobs.  Workers
  share only immutable inputs/optional transposition metadata, merge completed
  results in a fixed order, and stop early on a verified win or scoped disproof.
- Add postponed-sibling/dovetail scheduling so a deep unresolved DFPN edge cannot
  monopolize the deadline or turn no-progress into a false loss.
- Route threat, refutation, and immediate-win scans through an incrementally
  updated forbidden-rule legality mask, retaining the reference oracle for
  randomized and adversarial equivalence checks.
- Introduce one per-decision ledger for wall time, nodes, memory, proof sessions,
  and corpus queries; remove hidden emergency extensions and record every budget
  handoff in telemetry.
- Expose independent opening-book and elite-corpus switches and deterministic
  profile manifests so corpus effects can be measured without changing search
  semantics or the frozen three-/four-star controls.
- Add recovery, legality, concurrency, determinism, and budget tests, then rerun
  the existing forbidden/no-forbidden 100-game cells against frozen four-star
  (with legacy three-star as a non-gating generalization check) and publish
  color-separated Markdown/JSON evidence.  This change does not promote a
  production/UI route.

## Capabilities

### New Capabilities

- `five-star-recovery-search`: Sound result states, legal fallback selection,
  resilient four-star handoff, layered escape search, and postponed DFPN siblings.
- `five-star-parallel-proof`: Single-gain root proof jobs, deterministic result
  merging, verified early-stop behavior, and scoped-disproof certificates.
- `five-star-legality-budget`: Incremental forbidden-rule legality with oracle
  validation plus a unified per-decision resource ledger and deadline policy.
- `five-star-search-evaluation`: Reproducible profile/A-B manifests, diagnostic
  fixtures, performance gates, and rule/color-separated strength reports.

### Modified Capabilities

None.  Existing capabilities are change-local and have not been archived under
`openspec/specs`; frozen three-star/four-star behavior and the production route
remain outside this change.

## Impact

- Affects `ice five chess/FiveChessAI.c`, its public/internal headers and
  `doublethree.m`, especially candidate generation, proof/recovery orchestration,
  DFPN scheduling, and forbidden legality helpers.
- Adds telemetry and test/benchmark support for decision outcomes, budget usage,
  worker utilization, cache-oracle mismatches, certificate replay, and fixed
  schedules; generated reports remain under the existing workspace report tree
  and `/Users/wangzicheng/Downloads/logs_five_chess`.
- Keeps the 5.7 research profile separate from the old 5.1 production route and
  preserves frozen three-star/four-star binaries/configurations as controls.
- Adds no training data, network dependency, opponent-specific move table, or
  automatic UI promotion.
