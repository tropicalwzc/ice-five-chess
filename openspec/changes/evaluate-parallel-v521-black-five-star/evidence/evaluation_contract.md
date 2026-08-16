# Parallel v5.2.1 black evaluation contract

Frozen before implementation outcomes on 2026-08-15.

## Candidate and control

- Candidate white: `5.4.1-transactional-deadline-root-parallel-5s`, maximum eight concurrent proof workers.
- Candidate black: v5.2.1 policy plus isolated parallel-root execution, maximum eight concurrent proof workers.
- Thread control white: identical 5.4.1 component.
- Thread control black: identical v5.2.1 policy and whole-decision deadline, one proof worker and serial root budget.
- Opponent: frozen four-star `4.0.0-frozen-vcf-vct-control`, single-threaded and unchanged.
- Only one game process runs at a time. The CPU asymmetry is part of the candidate definition and is reported.

## Correctness and resource gates

- Every recorded move must be legal and every recorded game must replay through production C rule helpers.
- Worker jobs own private mutable state, never create nested workers, and merge only completed proof results or verified certificates.
- Every candidate/control decision returns a legal best-completed result within 5,000 ms; internal decision deadline is 4,500 ms.
- Seeded user randomness may vary among eligible completed classes. An independently seeded run is not required to reproduce an identical move sequence.
- Production/UI five-star remains v5.1 and this change has no promotion authority.

## Formal sample

- Fresh Freestyle and forbidden schedules are generated only after freeze.
- Each rule has 50 natural openings with colors exchanged.
- Each rule runs 100 one-worker-control games and 100 eight-worker-candidate games, all against four-star, for 400 games total.
- Control and candidate use the exact same schedule and master seed within a rule.
- All prior formal, diagnostic, corpus-shaped, and known-failure seeds, identities, and canonical positions are excluded; cross-rule overlap is forbidden.

## Rule-specific interpretation

For each rule, candidate strength requires:

- candidate white score rate at least 50%;
- candidate-black minus four-star-black same-color score delta at least zero;
- candidate overall score rate at least 50%.

Thread benefit requires eight-worker black and overall scores to be no lower than the one-worker control, with at least one strict point-estimate improvement across those measures in each rule. Wilson and opening-paired 95% intervals are reported. A positive point estimate whose interval overlaps zero is inconclusive, not demonstrated improvement. Rules and colors are never pooled to hide a failing gate.
