# Revision-6 deadline determinism incident

Date: 2026-08-14

Revision 6 completed four original 100-game cells with the required 50/50 candidate color split, zero runner anomalies, and no candidate decision above the 5,000ms player-visible ceiling. Independent deterministic regeneration nevertheless diverged in `free_vs_four` at opening identity 3 with candidate five-star as white:

- original ply 21: `(7,11)`;
- regenerated ply 21: `(7,13)`;
- both decisions reached the approximately 4,500ms internal deadline;
- the original run completed nine parallel batches and retained the verified baseline win, while regeneration completed eight batches and committed a partially searched loss-aware escape.

This violated the requirement that safety-wall runs may vary only in unfinished raw counters, while the selected move and completed proof metadata remain identical. All revision-6 games, schedules, and master seeds are therefore invalidated and non-promotional. The raw evidence remains under `reports/five_chess/five_star_proof_engine_20260814/formal/revision6/`; master seeds `0x9ee4d91480ac5889` and `0x834da1df0ff1d340` are permanently catalogued as prior seeds.

The root cause was non-transactional decision composition after parallel proof search. Quiet initiative, loss-aware refinement, and quiet-defense stages could update the selected move incrementally. If the shared deadline arrived within a stage, the caller could retain a partial override whose availability depended on how many worker batches happened to finish.

Candidate profile `5.4.1-transactional-deadline-root-parallel-5s` makes each of those stages transactional. Before a stage starts, it snapshots the last completed move and completed result metadata. If the shared deadline fires during the stage, all partial overrides are discarded, the prior completed result is restored, and `budgetExhausted=true` records the unfinished work.

The exact opening-3 reproduction was rerun twice for both candidate colors after the fix. Each color produced an identical full move list, winner, termination, chosen moves, and completed metadata across repetitions. Maximum candidate decisions were approximately 4,500.330ms and 4,500.286ms. Temporary raw reproductions are preserved at `/private/tmp/txn_deadline_opening3_a.jsonl` and `/private/tmp/txn_deadline_opening3_b.jsonl` for the active session.

No revision-7 freeze or formal seed may be issued until current-source primitive, activity, fixed-position, broader deadline determinism, sanitizer, and platform gates pass again.
