## Why

The current hybrid candidate is being referred to informally as “5.6”, while
the executable profile is `5.6.2-white-v541-black-v521-overlap-aware-parallel8-5s`.
The historical 5.6.0 serial control and 5.6.1 parallel candidate must remain
reproducible, so the new name needs to be an explicit alias rather than a
rewrite of those identities.

The latest forbidden-rule report shows only 44% for the candidate while the
previous report showed 58%.  The raw evidence already indicates two separate
effects: the two reports use disjoint opening schedules whose aggregate winner
balance changes from 58–42 black–white to 43–56, and the 5.6.2 overlap
dispatcher actually launches one worker per batch because its connected overlap
groups are too conservative.  In addition, the most important loss-aware
escape verification is still serial and forbidden-black legality falls back to
full-board copies.

Finally, the benchmark flag `forbiddenBlack` is not a complete RIF opening
protocol.  It is a rule-mode switch applied to natural Gomocup/Renju prefixes;
it does not model swap, Swap2, Taraguchi, or the RIF opening-choice steps.
Therefore a black-color score from that suite cannot by itself establish a
game-theoretic black advantage or its disappearance.

## What Changes

- Record the current candidate under the stable research alias `5.6` while
  preserving the 5.6.0 and 5.6.1 historical identities and evidence.
- Add a same-schedule attribution matrix separating: profile semantics,
  serial versus real multi-worker execution, overlap scheduling, escape-search
  parallelism, and opening-protocol effects.
- Replace connected-component overlap grouping as a proxy for concurrency with
  job-level root scheduling.  Overlapping gains remain independently proved;
  overlap metadata may guide TT sharing but must not serialize every root or
  merge proof obligations.
- Add a parallel, deadline-aware batch for loss-aware escape candidates with a
  reserved budget, private mutable sessions, and completed-result-only merge.
- Complete and oracle-test a reversible forbidden-black legality cache before
  using it for proof pruning or dependency grouping.
- Add separate no-exchange, RIF exchange, Swap2/Taraguchi-style, and (where
  available) 20x20-to-15x15 translated opening suites.  Report color and
  first-player effects independently.
- Use authoritative proof-number/DFPN, parallel-search, Renju-solving, and
  official RIF rule references to guide the next candidate; this change does
  not train on game outcomes and does not authorize UI promotion.

## Impact

This is a research and benchmark change.  It will eventually affect
`FiveChessAI.c/.h`, the benchmark scheduler/replayer, rule helpers, and report
provenance.  Until a later apply-and-promote change passes the frozen gates,
production/UI routing remains unchanged and the existing 3-star/4-star
controls remain frozen.
