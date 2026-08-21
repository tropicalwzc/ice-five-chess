# Five-star opponent-guard directional smoke A/B

> This 48-game paired smoke is directional only; it is not a formal strength claim.

## Five-star as black (primary)

| Profile | W/D/L | Score | 95% Wilson |
|---|---:|---:|---:|
| UI-bound 5.4.1 control | 8/0/4 | 66.7% | 39.1%–86.2% |
| opponent-guard candidate | 8/0/4 | 66.7% | 39.1%–86.2% |

## White and overall

| Profile | White W/D/L (score) | Overall W/D/L (score) |
|---|---:|---:|
| UI-bound 5.4.1 control | 2/0/10 (16.7%) | 10/0/14 (41.7%) |
| opponent-guard candidate | 1/0/11 (8.3%) | 9/0/15 (37.5%) |

## Guard and resource evidence

| Profile | Eligible | Audited | Disproofs | Unknown | Verified loss | Avoided | p50/p95/max ms | >5s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| UI-bound 5.4.1 control | 0 | 0 | 0 | 0 | 0 | 0 | 195.9/4500.3/4601.8 | 0 |
| opponent-guard candidate | 223 | 627 | 90 | 125 | 412 | 9 | 499.9/4500.2/4693.5 | 0 |

## Provenance and limitations

- Control raw SHA-256: `5e1f81144e9cdd249b8560eaed2142c5bc1d66e75f90da80bae92ea567d078ed`
- Candidate raw SHA-256: `14dfa21f235c86dc23eac6ab22779d3c16e69212c6fbdbe6c16fcff152ecd867`
- Schedule SHA-256: `ca08f0b75f9f6c6022eb5b375c9b4b516eaa4b5bd92680674ce7e31973fcb0f4`
- Replay SHA-256: `21ced2dbee58c76150f0c9e9aa26098c0fca08c42f25907a9954629fd7755534`
- Control command: `five_chess_benchmark --output smoke-control.jsonl --profile five-star --suite opponent-guard-smoke --random-mode best --strategy hybrid --seed 0x4755415244534d4b --opening-start 60 --opening-count 12 --max-moves 120 --forbidden-black 0 --opponent four-star --paired-phase 0`
- Candidate command: `five_chess_benchmark --output smoke-candidate.jsonl --profile five-star-opponent-guard --suite opponent-guard-smoke --random-mode best --strategy hybrid --seed 0x4755415244534d4b --opening-start 60 --opening-count 12 --max-moves 120 --forbidden-black 0 --opponent four-star --paired-phase 0`
- Regeneration/replay command: `python3 tools/replay_opponent_guard_smoke.py --library libopponent_guard_replay.dylib --schedule smoke_schedule.json --control smoke-control.jsonl --control-regenerated smoke-control-regenerated.jsonl --candidate smoke-candidate.jsonl --candidate-regenerated smoke-candidate-regenerated.jsonl --output smoke-replay.json`
- Avoidable VCF/VCT incidents require independent replay output and are not inferred solely from runtime telemetry.
- Any formal strength or forbidden-mode promotion claim requires a separately proposed, larger, freshly frozen dual-rule evaluation.

## Predeclared gates

- correctnessAndReplay: **FAIL**
- hardDeadline: **PASS**
- noNewAvoidableForcingIncident: **PASS**
- blackDirectionNonNegative: **PASS**
- whiteNoObviousRegression: **FAIL**
- overallNoObviousRegression: **PASS**

Playable binding decision: **retain-ui-bound-5.4.1**
