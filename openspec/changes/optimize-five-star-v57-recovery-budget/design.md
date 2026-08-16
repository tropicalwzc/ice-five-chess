## Context

The 5.7 profile is a research-only hybrid: it uses the frozen four-star move
as a baseline and adds loss-aware proof, escape, quiet-threat, and elite-corpus
stages.  The latest forbidden/no-forbidden run is useful evidence, but it also
shows that the engine's control flow is masking search incompleteness as playing
strength.  Candidate generation is intentionally bounded (defensive branches
may inspect at most a small set and ordinary branches use a local radius), yet
both an empty generated set and an all-unsafe generated set currently set
`provenLoss`.  A failed four-star hint can return before five-star own-win or
escape work begins.  Root workers enumerate the same threats again, and DFPN
no-progress returns `unknown` without giving postponed siblings a turn.

Forbidden-black profiling additionally shows billions of legality calls and
full-board copies in hot paths.  Several proof and corpus phases each receive
their own local allowance, and an emergency time value can extend a stage past
the intended player-visible deadline.  The implementation therefore needs an
explicit protocol for result certainty, resource ownership, and deterministic
merging before another strength comparison is meaningful.

The affected code is the five-star path in `FiveChessAI.c/.h`, the legacy
Objective-C recovery bridge in `doublethree.m`, and the existing benchmark and
report tooling.  The old production route and frozen three-/four-star controls
remain compatibility boundaries.

## Goals / Non-Goals

**Goals:**

- Make a non-terminal decision return a legal move whenever one exists, while
  distinguishing verified proof results from bounded-search unknown results.
- Ensure five-star recovery continues after a four-star hint failure or an
  incomplete proof and checks candidates in a deterministic, relevance-ordered
  sequence.
- Use multiple cores for independent root gains without repeating root threat
  enumeration, sharing mutable board state, or making result selection depend on
  thread completion order.
- Let DFPN revisit postponed siblings and keep all proof obligations within one
  deadline and one aggregate node/memory/session/query budget.
- Make forbidden legality checks incremental and prove equivalence with the
  existing oracle before allowing the cache to prune search.
- Produce enough telemetry and fixed-position gates to separate throughput,
  correctness, and opening/corpus attribution from win/loss variance.

**Non-Goals:**

- No neural training, outcome-selected move table, opponent-specific runtime
  policy, or new external/network dependency.
- No attempt to prove the game-theoretic outcome of unrestricted Gomoku/Renju,
  and no claim that a natural-prefix 100-game suite models RIF exchange/Swap2.
- No changes to the frozen three-star/four-star algorithms, their profile
  identities, UI mappings, or the current production route.
- No increase of the player-visible hard ceiling beyond five seconds; a timeout
  remains a valid unknown result with a legal fallback.

## Decisions

### 1. Use a four-state decision contract

Add an internal result enum (terminal no-legal-move, verified win, verified
loss, and unknown/deadline) and retain the existing telemetry fields as a
compatibility projection.  `provenLoss` is set only when the terminal board has
no legal move or every legal move in the *complete* candidate universe has an
independently replayed loss certificate.  A bounded candidate list with no safe
entry is recorded as unknown, not as a proof.  A deterministic legal fallback
is selected from the best completed candidate, then from a full-board legal
scan if the staged list is empty.

The caller bridge will end a game only for the explicit no-legal-move state or a
verified terminal loss; unknown must still place the fallback move and expose a
diagnostic reason.  This is preferred over changing the caller to guess from
`x/y` because it prevents older callers from treating a partially initialized
result as a loss.

### 2. Make recovery a staged, fail-open search

When the four-star hint is unavailable or its analysis is unknown, initialize a
five-star result from the best legal baseline and continue.  Build escape
candidates in fixed layers:

1. certificate/dependency moves that directly address the verified opponent
   threat;
2. tactical defenses (immediate wins, forced blocks, counter-threats);
3. nearby ordinary moves in the relevance zone;
4. all remaining legal moves as a bounded full-board fallback.

Each layer is deduplicated by coordinate and is dispatched only after cheap
legality and immediate-loss checks.  A candidate can replace the baseline only
after a scoped disproof is replayed on a private board; if no disproof finishes,
the best completed unknown/survival candidate is retained.  This ordering keeps
the search useful under a five-second deadline without declaring omitted moves
unsafe.

### 3. Dispatch single-gain root proof jobs

Enumerate root threats once on the coordinator and create one immutable job per
distinct gain.  A worker receives the gain, its precomputed dependency mask,
the root position snapshot, and a child budget; it starts proof below that gain
instead of calling the all-root `fc_prove_forced_win` entry point again.  Worker
mutable positions, proof graphs, and local transposition tables are private.

Jobs are submitted in canonical coordinate/order-key order.  Results are stored
by job index and merged in that same order after completion.  A verified win or
replayable scoped disproof sets an atomic stop flag; workers observe it at node
boundaries and the coordinator joins only the launched workers.  Overlap and
legality dependencies remain metadata for optional read-only TT sharing, never
an implicit connected-component serialization key.

### 4. Add postponed siblings and deterministic dovetailing to DFPN

The proof graph keeps a queue of postponed edges with their current proof/disproof
thresholds and revisit epoch.  When the selected edge makes no progress or hits
its local slice, the session postpones it, selects the next most-proving sibling,
and periodically requeues expired edges.  Threshold and counter updates are
performed with saturating arithmetic and committed only for completed child
states.  The session returns unknown on deadline or aggregate-budget exhaustion;
it never upgrades no-progress to a loss.

The scheduler uses fixed tie-breakers (proof number, disproof number, move
coordinate, insertion index) so one-thread and multi-thread diagnostic runs are
replayable even though the worker completion order differs.

### 5. Treat forbidden legality as a reversible cache with an oracle gate

Extend `FCIncrementalPosition` with affected-line revisions and exact local
facts for overline, double-four, and the project's established double-three
rule.  Make/unmake updates only the four crossing directions and their affected
five-windows, invalidating the corresponding side masks.  Threat generation,
refutation generation, and immediate-win scans query the incremental mask first.

The reference `fc_is_legal_move` remains the oracle.  A validation mode compares
cached and oracle answers on every move in adversarial fixtures and on a seeded
random sample during diagnostics.  Any mismatch disables cache pruning for the
decision and is reported; the optimization is not allowed to silently alter
rule semantics.

### 6. Account for one decision budget

Create a decision ledger owned by the top-level five-star call.  It tracks an
absolute monotonic deadline (internal target 4,500 ms, hard ceiling 5,000 ms),
aggregate proof/search nodes, temporary bytes, active sessions, and corpus/book
queries.  Every stage reserves and consumes from the same ledger; no stage may
raise its limit to an emergency value after the deadline.  Reservations are
released only for completed results, and telemetry records requested, consumed,
and abandoned units plus the stage that caused exhaustion.

The ledger is passed to root, escape, quiet, corpus, and fallback stages rather
than recreated in helpers.  A fixed stage order and deterministic reservation
policy makes A/B comparisons attributable while still allowing up to the
profile's worker cap (eight for the research candidate).

### 7. Separate profile semantics from corpus/book attribution

Keep `openingBookEnabled` and `eliteCorpusEnabled` independent in the profile
manifest.  A corpus candidate is advisory: it must pass the same legality,
proof/recovery, and budget rules as a searched move and cannot bypass a verified
immediate win or mandatory defense.  Diagnostic runs write the complete profile
manifest, corpus version, worker cap/actual utilization, and cache-oracle
mismatch count.  The opening/corpus-off and on cells use the same seeds and
opening schedule; their results are reported separately from the algorithmic
candidate.

### 8. Gate strength testing on correctness and performance evidence

Before a new 100-game run, execute fixed fixtures for fallback legality,
four-star failure handoff, certificate replay, root-job equivalence, postponed
sibling determinism, and forbidden-cache oracle equivalence.  Run one/four/eight
worker microbenchmarks and require no correctness drift, no unexplained
duplicate root work, and p95 decision time within the five-second contract.
Only then run forbidden and no-forbidden 100-game cells against the frozen
four-star; retain the legacy three-star cells as a non-gating generalization
check.  Reports keep black/white colors, paired schedules, unknown/no-progress
counts, and Wilson intervals separate.

## Risks / Trade-offs

- **[Risk]** A legal fallback may be weaker than the old heuristic move when the
  proof budget is exhausted. → **Mitigation:** preserve the completed baseline
  score/order, use deterministic full-board legal fallback, and report the
  unknown reason instead of hiding it as a loss.
- **[Risk]** Shared read-only transposition metadata can become stale across
  rule modes or profile versions. → **Mitigation:** include rule/profile/search
  class in keys and merge only completed, certificate-checked states; private
  worker tables remain the default.
- **[Risk]** More workers increase memory pressure and contention. → **Mitigation:**
  ledger reservations, a hard cap of eight, private bounded sessions, and
  one/four/eight worker telemetry with automatic fallback to one worker.
- **[Risk]** Incremental forbidden facts may diverge from historical double-three
  semantics. → **Mitigation:** oracle cross-checks, adversarial fixtures, and a
  fail-closed switch that keeps the reference path on any mismatch.
- **[Risk]** Fixed reservation order can leave CPU idle while a proof is waiting.
  → **Mitigation:** use independent root jobs and dovetailing within the reserved
  stage; do not borrow budget across stages without recording the transfer.
- **[Risk]** A 100-game result remains sensitive to opening identity and color
  balance. → **Mitigation:** freeze schedules, report color-specific paired
  deltas, and label natural-prefix/no-swap cells as diagnostic rather than
  game-theoretic evidence.

## Migration Plan

1. Add the result-state/ledger and telemetry types behind the five-star research
   profile; keep the existing profile entry points and production route intact.
2. Implement recovery and root-job changes with the legacy oracle and one worker,
   then enable the incremental legality cache after equivalence fixtures pass.
3. Enable postponed-sibling scheduling and eight-worker dispatch behind explicit
   profile flags.  Run fixed-position correctness/performance gates and compare
   against the frozen 5.7 manifest.
4. Run the color/rule-separated four-star and three-star reports, archive raw
   manifests and logs, and promote only in a later user-approved change.

Rollback is a profile-level switch to the frozen 5.7 candidate or the existing
5.1 production route.  No control binary, UI mapping, or historical report is
rewritten by this change.

## Open Questions

- Should the ledger's memory unit be allocator bytes, resident-set high-water
  mark, or both on the target macOS benchmark runner?
- What minimum replayable certificate size is practical for a scoped disproof
  before the certificate must be classified as unknown?
- Should the eight-worker cap be the default on all supported devices, or selected
  from a profile manifest after a one-time CPU/memory capability probe?
- Which formal RIF exchange/Swap2 schedules, if any, should be added in the next
  evaluation change rather than mixed into this recovery-focused implementation?
