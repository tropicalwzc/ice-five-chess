# Revision-7 formal replay failure

Date: 2026-08-15

Candidate `5.4.1-transactional-deadline-root-parallel-5s` completed the revision-7 original 400-game suite and an independent 400-game regeneration. Every one of the eight JSONL datasets is internally valid under the frozen schedule and C rule reference: 800 games, 27,982 moves, 11,086 candidate decisions, 2,391 verified-certificate flags, zero runner anomalies, and zero candidate decisions above the 5,000 ms player-visible ceiling.

Exact original/regenerated replay nevertheless failed in every cell:

| Cell | Move-divergent games | Candidate metadata-divergent steps before the first move divergence |
|---|---:|---:|
| Freestyle vs frozen four-star | 1 | 7 |
| Freestyle vs legacy three-star | 2 | 5 |
| Forbidden vs frozen four-star | 4 | 5 |
| Forbidden vs legacy three-star | 4 | 9 |
| Total | 11 | 26 |

The comparison counts metadata only while both runs still have an identical move history. Differences after the first changed move are intentionally excluded because they no longer describe the same position.

The first reproducible boundary is Freestyle opening 42 with the candidate as black at ply 12. With the same board and seed, one execution retains frozen-baseline `(4,8)` after corpus rollback and another accepts corpus `(7,8)`; both decisions finish near the 4,500 ms internal deadline. `tools/five_chess_deadline_reproduction.c` repeats this split from a pure C fixture. Forbidden opening 7 with the candidate as white at ply 11 similarly alternates between quiet `(7,11)` near 455 ms and corpus `(10,11)` near 4,551 ms.

The remaining nondeterminism is not the revision-6 partial-stage commit bug. Transactional rollback correctly prevents unfinished quiet/loss-aware stages from leaking partial overrides. The remaining causes are:

1. corpus acceptance can depend on whether wall-limited baseline and proof queries complete before the absolute deadline;
2. completed proof status can cross the wall boundary even when the final move does not change;
3. the byte-identical frozen four-star itself is wall-limited, and some full-process regenerations produced a different four-star move before the candidate moved.

Therefore revision 7 violates the exact-regeneration gate. All four strength cells are invalid for promotion. Their W/D/L outcomes are retained only as diagnostic evidence and must not be used for another parameter-tuning cycle.

Even if replay were ignored, the strength gate would still fail: candidate white improvement against frozen four-star in the forbidden-rule cell is exactly `+0.0%` in both the original and regenerated suites, while the predeclared gate requires a strictly positive direction in both rules.

Release decision: do not promote. Production five-star remains `5.1.0-elite-rule-partitioned-local-v2`; frozen four-star and one-through-four-star behavior remain unchanged.
