# 5.8.2 soft-weight follow-up

This follow-up lowers the isolated black double-three structural-defense
weight to 40/100. A single detected white double-three must now overcome a
distance cost against the existing move; multiple independent gains retain
the residual-first defense. A move that already leaves zero residual gains is
never rewritten solely because another move is also safe.

## Fixed small-match result

The same 12 seeded openings, both candidate colors, deterministic-best mode,
no-forbidden rule mode, and 120-move cap were used as the original 24-game
cells.

| Cell | Candidate black (W/D/L) | Candidate white (W/D/L) | Overall (W/D/L) |
| --- | ---: | ---: | ---: |
| soft-40 vs frozen four-star | 8/0/4 | 5/0/7 | 13/0/11 |
| soft-40 vs exact 5.8.1 | 9/1/2 | 2/1/9 | 11/2/11 |

For comparison, the initial hard-preemption 5.8.2 cell was 6/0/6 as
candidate black against both controls. The exact 5.8.1 no-defense diagnostic
cell was 9/1/2 as candidate black, so the soft-40 setting removes the measured
single-threat regressions without changing the control profile.

The optimized profile header is:

`5.8.2-black-double-three-soft-40-16g-32c-80ms`

Replay verified identical opening prefixes, legal moves, terminal results,
board restoration, and all available proof certificates. No incomplete scan,
deadline anomaly, or rollback was recorded. Certificate replay totals were
97 for the four-star cell and 82 for the 5.8.1 cell.

Raw records:

- [`five-star-5.8.2-soft40-vs-four-star.jsonl`](five-star-5.8.2-soft40-vs-four-star.jsonl)
- [`five-star-5.8.2-soft40-vs-5.8.1.jsonl`](five-star-5.8.2-soft40-vs-5.8.1.jsonl)
- [`replay-soft40-summary.json`](replay-soft40-summary.json)

Source and artifact hashes are recorded in [`source-provenance.md`](source-provenance.md).
