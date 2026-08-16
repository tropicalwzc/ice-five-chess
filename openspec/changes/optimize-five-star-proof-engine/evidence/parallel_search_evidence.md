# Root-parallel five-star search evidence

Date: 2026-08-14  
Candidate: `5.4.1-transactional-deadline-root-parallel-5s`  
Reference host: Apple M4 Pro, 10 performance + 4 efficiency cores

## Frozen candidate resource policy

- Maximum concurrent search workers: 8.
- Ordinary per-session node cap: 36,000.
- Aggregate root-parallel node cap: 288,000, divided by the number of ordered unique root jobs before launch and capped at 36,000 per job.
- Per-stage worker wall allowance: at most 1,200ms.
- Complete candidate search allowance: 4,200ms, inside one absolute 4,500ms decision deadline and the 5,000ms player-visible gate.
- Private session arena: 10,747,904 bytes per worker.
- Eight-worker arena ceiling: 85,983,232 bytes (about 82.0 MiB).
- Formal runner policy: one game process at a time. Four-star and legacy three-star remain byte-identical and receive the same otherwise-idle host, priority, wall-clock rules, and thermal preparation; multithreaded proof search is part of the candidate five-star algorithm.

Workers share only immutable board/root inputs and the absolute deadline. Each owns its incremental position, graph, TT, certificate arena, root filter, generation, and diagnostics. Results are stored by the frozen threat order and merged by root identity, never by completion order. A scoped disproof is accepted only if root enumeration is complete and every root completed its scope. A no-progress DFPN fixed point terminates as `unknown` rather than spinning until timing jitter changes work counts.

## Correctness and concurrency

- Strict C11 `-Wall -Wextra -Werror -pedantic`: pass.
- One/two/legacy-three-star golden and frozen-four-star checks: pass.
- ASan + UBSan full C suite: pass.
- ThreadSanitizer full C suite: pass, zero data races.
- iOS Simulator Debug build: pass; only pre-existing asset/Objective-C/deployment warnings remain.
- Repeated parallel root proof: identical move, proof class, distance, certificate ID, and deterministic node count when governed by the fixed node budget rather than a wall timeout.
- Parallel proof certificate replay, complete aggregate disproof replay, partial-budget unknown, board restoration, and thread-local diagnostic reduction: pass.
- Activity fixture: 8 workers searched 8 roots, all 8 completed, and the aggregate scoped disproof replayed independently.

Raw work counters can differ slightly when a decision reaches the wall ceiling; they remain honest work telemetry. The chosen move and completed proof metadata were deterministic across all four fixed-position repetitions in both rule modes.

## Four-repetition fixed-position 5-second gate

Each rule used eight fixed diagnostic identities, both colors, four serial paired repetitions, phase order `0,1,0,1`, deterministic evaluation, and one process at a time. The candidate and v5.1 control were alternated within each opening/color pair.

| Rule | Engine | wall p50 ms | wall p95 ms | wall mean ms | wall raw max ms | CPU mean ms | CPU raw max ms | peak RSS | Result |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| no-forbidden | v5.1 control | 292.223 | 4466.595 | 723.206 | 4481.150 | 721.612 | 4470.289 | 43.9 MiB | control |
| no-forbidden | parallel candidate | 329.064 | 4549.451 | 1035.275 | 4549.994 | 1454.883 | 10970.306 | 43.9 MiB | pass |
| forbidden | v5.1 control | 360.711 | 6775.151 | 1107.459 | 6856.296 | 1104.380 | 6841.723 | 37.5 MiB | control |
| forbidden | parallel candidate | 360.712 | 2713.550 | 849.629 | 2729.200 | 968.937 | 4186.000 | 37.5 MiB | pass |

Both candidate rule suites had zero decisions above 5,000ms, deterministic moves and completed proof metadata, active parallel search, and no illegal/anomalous decision. Candidate moves differed from v5.1 in both rules, so the new search is behaviorally active. Descriptive percentile comparisons are reported but are not promotion gates under the approved hard-ceiling rule.

CPU time is process CPU time measured around each complete decision, so values above wall time directly expose useful worker concurrency rather than process-level game sharding. Peak RSS is the process high-water mark observed in the raw step records; the larger no-forbidden run reached 45,989,888 bytes (43.9 MiB), below the predeclared 85,983,232-byte eight-worker private-arena cap. The v5.1 and candidate rows can share a high-water value because both execute serially inside the same paired process; the profile snapshots separately retain the exact per-session and aggregate arena bounds.

Representative parallel activity:

- No-forbidden: 23 batches, 105 root jobs, 102 completed, 94 cumulative worker launches across the 16 representative decisions.
- Forbidden: 15 batches, 61 root jobs, all 61 completed, 59 cumulative worker launches.

Summaries:

- `fixed_position_transactional_5s/free_summary.json` — SHA-256 `68d51809b8c2b60763095720b0db1b7b8f55abee4dc02872f69e9d912686f11e`
- `fixed_position_transactional_5s/forbidden_summary.json` — SHA-256 `b8c5a77b8a6c947f739fd046161ca56f770ef540807e695cc7335726c59855bc`

The five-prefix, both-color deadline diagnostic was also repeated under both rules. All 40 short games had identical complete result metadata; only raw unfinished node counters varied. This specifically renews deterministic safety-wall behavior after the revision-6 incident.

## Worker-count ablation

One serial 16-decision pass per rule compared 1, 4, and 8 workers on identical positions. All three counts chose the same moves in both rules.

| Rule | Workers | p50 ms | p95/max ms | mean ms | Parallel jobs completed |
|---|---:|---:|---:|---:|---:|
| no-forbidden | 1 | 327.820 | 4547.979 | 1101.252 | 0 (single-session fallback) |
| no-forbidden | 4 | 330.524 | 4548.703 | 1034.784 | 91 / 94 |
| no-forbidden | 8 | 329.412 | 4548.391 | 1035.253 | 102 / 105 |
| forbidden | 1 | 361.002 | 2335.477 | 873.237 | 0 (single-session fallback) |
| forbidden | 4 | 361.561 | 2742.029 | 851.313 | 61 / 61 |
| forbidden | 8 | 360.119 | 2734.064 | 851.844 | 61 / 61 |

Eight workers are retained for the declared M4 Pro profile: they complete more no-forbidden root jobs before the same hard ceiling than four workers, keep the same deterministic decisions, and remain below the measured 82.0 MiB worker-arena cap. Four workers are a viable lower-memory fallback for future device-specific policy, but are not mixed into the formal profile.
