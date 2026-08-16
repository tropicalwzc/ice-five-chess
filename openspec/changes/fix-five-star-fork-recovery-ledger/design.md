## Context

The five-star research path already has a top-level `FCDecisionLedger`, proof
sessions, loss-aware escape jobs, and telemetry for worker utilization.  The
latest audit shows that these mechanisms are not yet measuring the intended
search.  `fc_proof_session_begin` reserves the full proof graph/table arena,
but `fc_proof_session_end` only frees the arena; it does not return the bytes
to `memoryReserved`.  A session is about 10.7 MiB and eight workers can
legitimately need about 86 MiB at once, but sequential sessions accumulate
reservations until the ledger reports roughly 256 MiB exhausted.  The current
`memoryConsumed` path does not describe this live allocation either.

The recovery path has a second, independent failure mode.  A failed four-star
handoff constructs a single full-board legal fallback, and escape search is
entered only after an opponent forced-win proof is verified.  The audited
fallback positions therefore bypass the pre-proof tactical decision point.
Moreover, candidate scoring contains local immediate-reply checks that are
not sufficient to establish whether a candidate leaves two legal opponent
wins.  A fallback coordinate is an observable symptom of this control flow,
not a proof that the coordinate itself was illegal.

The change is confined to the five-star research profile.  It must preserve
the frozen four-star control and the production route, operate under the
existing five-second ceiling, and keep worker completion order from changing
the selected move.  The existing private worker proof graphs remain the
implementation boundary for this change; shared-TT, virtual-PN, and a new
parallel-dovetail scheduler are deliberately not prerequisites.

## Goals / Non-Goals

**Goals:**

- Make proof-session memory reservations reversible and observable as live,
  peak, released, and exhausted quantities for one decision.
- Evaluate opponent immediate winning replies after the candidate is actually
  placed, before a deeper proof becomes decisive, and prioritize candidates
  that avoid a two-reply fork.
- Keep an ordered, legal candidate set through an invalid/unknown four-star
  handoff and select a deterministic least-risk fallback when no proof
  completes.
- Make the 97 audited fallback positions replayable and measure fork exposure,
  fallback legality, ledger reuse, unknown outcomes, and useful proof work.
- Compare one, four, and eight workers with identical inputs and deterministic
  result merging so that additional cores can be evaluated separately from
  search correctness.

**Non-Goals:**

- No shared mutable transposition table, virtual proof/disproof numbers,
  parallel-dovetailing scheduler, or general rearchitecture of DFPN.
- No change to Renju/Gomoku rule semantics, the frozen three-/four-star
  algorithms, UI difficulty mapping, or the production route.
- No guarantee of a particular 100-game win-rate uplift; fixed tactical and
  resource gates must pass before a new strength run is interpreted.
- No opponent-specific move table, training data, network dependency, or
  automatic promotion of the research profile.

## Decisions

### 1. Model proof memory as a reversible live reservation

Extend `FCDecisionLedger` with a release operation and a peak counter.  A
successful reservation atomically increases live reserved bytes and updates a
high-water mark; it fails without changing the ledger when the live amount
would exceed the decision memory budget.  Use a compare-and-swap loop rather
than `fetch_add` followed by rollback so concurrent workers cannot temporarily
oversubscribe the budget.  Keep cumulative allocation/consumption telemetry
separate from live reservation telemetry.

`FCProofSession` stores the exact reservation size, the owning ledger, and a
held flag.  `fc_proof_session_end` releases that exact amount exactly once,
including sessions that were exhausted, cancelled, or ran in a worker.  If
allocation fails after reservation, the begin path releases immediately.  A
session that could not reserve memory has no held reservation and cannot
release another session's bytes.  The result projection reports live bytes at
decision end, peak bytes, released bytes, and the number of ledger exhaustion
events.

This is preferred over treating every allocation as cumulative consumption:
the budget limits simultaneous arenas, and cumulative bytes alone caused the
false exhaustion observed in the audit.  It also makes eight-worker pressure
visible without penalizing a sequence of correctly torn-down sessions.

### 2. Run a fork-first tactical probe before proof-dependent recovery

Add a coordinator-side candidate annotation pass before own-win proof and
before the opponent-proof gate that currently starts escape search.  For each
candidate, the pass shall:

1. verify the move with the established legality oracle/cache policy;
2. place the current side's stone on a private board/position;
3. treat an own five as terminal success; otherwise enumerate legal immediate
   wins for the opponent, stopping after the second reply when only risk
   classification is needed; and
4. record the exact reply count (or a conservative `unknown` result if the
   budget ends) and whether the candidate leaves a two-reply fork.

The opponent scan is performed after the current move is placed.  It must not
reuse the existing helper shape that places the *opponent* stone on the
candidate coordinate.  The reference legality semantics remain authoritative,
including forbidden-black checks.

Candidate ordering is stable: own immediate win, mandatory defense, fork-safe
tactical move, fork-safe baseline/ordinary move, one-reply move, then
fork-risk move; ties use the existing score and coordinate order.  The probe
pool includes the four-star move, generated tactical moves, and canonical
legal moves from the relevance zone.  If no fork-safe candidate is found and
the deadline still permits it, it extends through a canonical full-board legal
scan.  `candidateCoverageComplete` records whether the full legal universe
was checked, so a bounded probe is never described as a proof of global
safety.

This is preferred over waiting for a verified opponent proof: the audit shows
that the damaging fork is already present while the proof result is often
unknown.  It is also preferred over a purely local nine-cell reply scan:
locality is a useful ordering hint but cannot certify that a second legal win
does not exist elsewhere on the board.

### 3. Preserve candidates across an unknown or failed four-star handoff

When the four-star result is missing or illegal, initialize the five-star
baseline from the same deterministic candidate generator used by normal
recovery, then run the fork probe.  The first canonical legal coordinate is
used only if candidate generation or probing cannot produce a usable entry;
it is not inserted as the sole candidate merely because the handoff failed.

When the four-star move is legal but its proof or opponent proof is unknown,
the fork-ranked alternatives remain eligible.  A candidate is accepted as a
verified defense only after the existing proof/certificate replay rules pass.
If proof remains unknown, the coordinator may still select the highest-ranked
completed fork-safe legal candidate, and records `UNKNOWN_OR_DEADLINE` rather
than `VERIFIED_LOSS`.  If every examined candidate is fork-risky, it selects
the candidate with the smallest known opponent reply count, then stable score
and coordinate order, and records incomplete coverage.  The result is always
legal when any legal move exists.

The result and diagnostics add enough attribution to distinguish a
four-star-handoff failure, a fork-safe recovery selection, a no-safe-candidate
case, and a plain full-board fallback.  No `(0,0)`-style sentinel is allowed
to represent a non-terminal search result.

### 4. Keep worker evaluation deterministic and scoped

Add an explicit research-only worker-count override for 1, 4, and 8 workers.
Workers continue to receive private board/proof state and the same aggregate
ledger.  Candidate indexes are assigned in canonical order; each result is
written to its index, and the coordinator merges results by index rather than
accepting whichever thread finishes first.  Early stop is allowed only for a
verified result and cannot discard an earlier canonical result that is needed
for deterministic fallback selection.

The replay harness uses the same 97 board snapshots, profile manifest, rule
mode, seeds, and deadline for all three worker cells.  It records wall time,
CPU time, live/peak memory, proof sessions, completed proofs, fork-safe
selections, unknowns, fallbacks, and actual concurrent workers.  “Parallel
advantage” is reported as useful completed proof work or fixed-fixture
outcome improvement per unit time; worker count alone is not treated as an
improvement.  Any selected-move or legality drift across 1/4/8 on a fixed
position is a gate failure.

### 5. Turn the audited failures into gates before another game suite

Extract the 97 fallback positions into a versioned fixture manifest containing
board state, side, forbidden-rule mode, legacy hint, prior fallback metadata,
and the source-log hash.  Add unit fixtures for:

- reservation release after normal, exhausted, allocation-failure, and
  concurrent session teardown;
- a synthetic candidate that creates two opponent wins and an alternate
  fork-safe candidate;
- invalid four-star handoff with multiple legal candidates;
- unknown proof with a legal fork-safe fallback; and
- deterministic 1/4/8 worker replay.

The fixed-position gate requires legal output for every fixture, no false
verified loss from a bounded candidate list, fork avoidance whenever a safe
candidate is available in complete coverage, zero unreconciled ledger bytes at
decision end, and identical selected output across worker cells.  Only after
these pass should the existing color/rule-separated 100-game comparison be
rerun.

## Risks / Trade-offs

- **[Risk]** Full-board opponent reply checks can consume the same deadline that
  the proof search needs. → **Mitigation:** stop after two replies for risk
  classification, reuse the existing legality path, order the relevance zone
  first, and expose probe time/candidate coverage in telemetry.
- **[Risk]** A fork-safe heuristic may reject a strategically strong move that
  is tactically acceptable under a deeper proof. → **Mitigation:** it does not
  claim a proof or overwrite a verified win/mandatory defense; the verified
  proof result can supersede the annotation, and the risk threshold is tested
  against replay fixtures.
- **[Risk]** A release bug could undercount memory while workers are still
  using an arena. → **Mitigation:** session-owned reservation tokens, exactly
  once release assertions, concurrent stress tests, and peak/live invariants.
- **[Risk]** Deterministic merging may leave CPU idle after an early proof.
  → **Mitigation:** retain verified early stop, but preserve canonical result
  storage and report useful work rather than maximizing worker occupancy.
- **[Risk]** The 97 positions are selected from observed fallbacks and may not
  represent all openings. → **Mitigation:** use them as a correctness gate,
  keep the 100-game schedule as a separate strength measurement, and do not
  claim general game-theoretic improvement from the fixture result.

## Migration Plan

1. Add the ledger release/peak fields and session ownership token behind the
   five-star research profile; run ledger unit and concurrent teardown tests.
2. Add fork annotations, handoff candidate generation, and deterministic
   unknown fallback selection while keeping proof workers at one; pass the
   synthetic and 97-position tactical gates.
3. Add the worker-count override and replay/report fields; run 1/4/8 fixed
   cells and reject any legality or selected-move drift.
4. Rerun the existing forbidden/no-forbidden 100-game cells against frozen
   four-star with the new manifest.  Compare color-specific paired outcomes,
   unknown/fallback rates, and resource metrics before considering a later
   architecture change.

Rollback is a research-profile switch to the frozen 5.7 manifest or the
existing 5.1 production route.  Historical logs and frozen control binaries
are not rewritten.

## Open Questions

- Should `memoryPeakReserved` be compared only with allocator reservations, or
  also with an optional process-resident high-water sample on the benchmark
  runner?
- What fork-probe candidate limit gives the best five-second trade-off on
  forbidden-black boards before the complete-coverage pass is required?
- Should the 1/4/8 worker override be a compile-time test flag, a profile
  manifest field, or a benchmark CLI argument once the fixed gate is stable?
- Which additional RIF exchange/Swap2 schedules belong in a later evaluation
  change rather than in this recovery-focused fix?
