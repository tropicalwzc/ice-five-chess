## Baseline identity

The stable research alias is:

```text
5.6
  = 5.6.2-white-v541-black-v521-overlap-aware-parallel8-5s
  white: 5.4.1-transactional-deadline-root-parallel-5s
  black: 5.2.3-overlap-aware-deeper-root-parallel-8w-5s
  internal deadline: 4,500 ms
  player-visible hard ceiling: 5,000 ms
```

The alias is benchmark/research metadata only.  `5.6.0-white-v541-black-v521-
serial1-control-5s` and `5.6.1-white-v541-black-v521-parallel8-5s` remain
immutable historical identities.  The player-facing production profile is
still v5.1.

## 5.7 candidate identity

The implementation produced by this apply pass is recorded separately as:

```text
5.7.0-white-v541-black-v521-independent-root-parallel8-5s
  white: 5.4.1-transactional-deadline-root-parallel-5s
  black: loss-aware v5.2.1 path with independent root jobs,
         candidate-level escape batches, and 8-worker cap
  internal decision deadline: 4,500 ms
  player-visible hard ceiling: 5,000 ms
```

This is a benchmark/research identity only.  The two 100-game cells in this
pass use the frozen natural-prefix/no-swap opening schedule and the frozen
single-thread four-star opponent.  They do not alter the 5.6 aliases or the
production/UI route.  The report records actual worker concurrency rather than
the configured cap, and reports the forbidden legality-cache oracle mismatch
count explicitly.  The current reference rule helper intentionally retains its
historical continuation-based double-three interpretation; the explicit
crossing fixture verifies cache/oracle equivalence and a double-four rejection,
but is not a claim of complete RIF double-three semantics.

## Evidence-led diagnosis

### Opening/sample effect (already demonstrated)

The current and previous forbidden reports use disjoint 50-opening schedules.
In the current schedule, the 100 games have 43 black wins, 56 white wins, and
one draw; in the previous schedule they have 58 black wins and 42 white wins.
The candidate and four-star cells move together (current candidate black 44%
versus four-star black 43%; previous candidate black 58% versus four-star
black 58%).  In the current 50 color-exchanged pairs, 40 pairs (80%) have the
same winner in both engine-color assignments; the previous suite has 44 such
pairs.  This is strong evidence that the 14-point absolute change is dominated
by opening/sample identity, not an isolated candidate regression.

The schedules are official-game prefixes, but `generate_formal_opening` simply
replays coordinates and assigns `side = ply % 2`; no swap or RIF fifth-move
choice is represented.  `forbiddenBlack=true` therefore tests a useful
rule-mode diagnostic, not a formal balanced Renju match.

### Parallel-utilization regression (directly demonstrated)

The 5.6.2 forbidden raw bundle aggregates 982 parallel batches, 982 launched
workers, and 982 overlap groups for 7,976 root jobs (7,216 completed).  Thus
the average is exactly one worker per batch despite an eight-worker profile.
The archived 5.6.1 forbidden bundle has 1,046 batches, 7,108 worker launches,
and 9,319 root jobs (8,022 completed), about 6.8 workers per batch.

The cause is structural: `fc_parallel_prove_forced_win` sets
`workerCount = min(profile->proofWorkerCount, compactGroupCount)`, while the
connected overlap grouping collapses the roots in these positions to one
group.  `fc_populate_threat_dependencies` also adds entire lines to the
forbidden legality dependency mask.  Overlap is consequently acting as a
serialization key, not as a safe TT-sharing hint.

### Escape and legality bottlenecks (directly demonstrated)

Across 843 current forbidden black decisions there are 3,669 escape
alternatives, of which 3,605 (98.26%) finish as unknown, only one is a scoped
disproof, and 425 decisions exhaust the budget.  In the 28 games lost while
the candidate is black, the 482 black decision steps contain the same 3,669
escape checks; the escape loop calls serial `fc_prove_forced_win` at
`FiveChessAI.c:5626` rather than `fc_parallel_prove_forced_win`.

The same 843 decisions perform about 10.58 billion legality calls and 2.46
billion full 15x15 board copies.  The incremental position explicitly falls
back to the reference legality oracle for forbidden black, and the forbidden
threat/refutation enumerators retain full-board scans.  This consumes the
5-second gate before deeper defensive proof can finish.

### Unisolated semantic change

The current 5.6.2 profile sets `proofCandidateStagesEnabled=true`; the archived
5.6.1 profile did not serialize that field and therefore did not enable the
candidate own-win/quiet-stage path for the v5.2.1 black component.  Because the
profile change and overlap scheduler change landed together, their individual
effects are not identified by the existing W/D/L reports.  A same-schedule
toggle A/B is required before attributing any strength difference to either.

## Search design guided by the literature

1. **Job-level root proof search.**  Dispatch each distinct gain as an
   independent job, dynamically load-balanced over at most eight workers.
   Keep dependency/legality overlap as metadata and for optional shared-TT
   policy; never treat different gains as one proof result.
2. **Virtual proof/disproof numbers.**  If a shared TT is introduced, reserve
   a virtual PN/DN contribution when a worker claims a node, and merge only
   completed node states.  This follows parallel DFPN/PNS practice and avoids
   several workers repeatedly selecting the same most-proving child.
3. **Postponed siblings and dovetailing.**  Search the most promising root
   first, postpone expensive siblings, and periodically reinsert unfinished
   jobs so one deep branch cannot starve all other gains.
4. **Escape batch.**  When the default move has a verified opponent certificate,
   reserve (rather than opportunistically consume) a fraction of the remaining
   deadline for defensive candidates.  Run each candidate with a private
   session and stop on the first independently replayable scoped disproof;
   otherwise retain the best completed survival/unknown result.
5. **Forbidden legality cache.**  Cache line encodings and the exact local
   overline/double-four/double-three facts affected by a move.  Update and
   undo only the four crossing directions plus affected five-windows.  Every
   cached result must be checked against the original oracle in randomized and
   adversarial fixtures before it can prune a proof.
6. **Formal opening protocols.**  Keep a raw forbidden/no-swap suite for
   algorithm diagnosis, but add RIF exchange/Swap2/Taraguchi-style schedules
   with explicit role/choice metadata.  Do not pool those rule cells with
   freestyle or natural-prefix results.

## Non-goals

- No game-outcome tuning, opponent-specific move table, or training corpus.
- No changes to frozen 3-star/4-star controls or production/UI routing.
- No claim of a black theoretical advantage from a 100-game natural-prefix
  benchmark.
