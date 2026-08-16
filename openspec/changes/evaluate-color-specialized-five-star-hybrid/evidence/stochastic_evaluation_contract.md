# Seeded stochastic evaluation contract

Frozen before hybrid implementation outcomes: 2026-08-15.

## Hypothesis

- Hybrid white delegates to `5.4.1-transactional-deadline-root-parallel-5s`.
- Hybrid black delegates to `5.1.0-elite-rule-partitioned-local-v2`.
- User-game randomness is part of the policy and is allowed only inside the best move's completed equivalence class.
- The final sample is four fresh 100-game cells: Freestyle/forbidden versus frozen four-star/legacy three-star, each with 50 hybrid-white and 50 hybrid-black games.

## Replay meaning

Every recorded game must replay through independent C rule helpers for its scheduled prefix, alternating side, every move, forbidden-black legality, terminal result, certificate flags, component provenance, decision seed, random equivalence signature, and final board. A separately seeded run is not required to reproduce the same moves. Random variation and wall-clock budget exhaustion are distinct fields and may not be conflated.

Revision-7 master seeds `0x882b8c2441e20eab` and `0xe18dbabfada1c1f8`, all revision-7 opening identities/positions, all older formal seeds in `formal_prior_seed_catalog.json`, diagnostics, corpus-shaped positions, and known failure fixtures are excluded from fresh schedule generation. Revision-7 W/D/L is motivation only and cannot tune the router, component parameters, randomness eligibility, opening selection, or gates.

## Point-estimate gates

- Frozen four-star, each rule: hybrid white score rate at least 50%; same-color black delta non-negative; hybrid overall score rate at least 50%.
- Legacy three-star, each rule: same-color white delta and overall direction non-negative.
- Every cell: legality/certificate/random-equivalence replay passes and every hybrid decision is at most 5,000 ms.
- No rule or color pooling may hide a failure. Wilson and paired intervals are mandatory; a passing point estimate with an interval overlapping the boundary is inconclusive, not demonstrated superiority.

Passing these evaluation gates does not itself authorize a UI switch. Production five-star remains v5.1 unless a later explicit promotion change is requested.
