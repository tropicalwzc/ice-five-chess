# Opponent-guard candidate vs UI-bound 5.4.1

Date: 2026-08-20 (Asia/Shanghai)

This is a directional follow-up, not a promotion test. It uses six of the
previously frozen natural freestyle openings (60–65), exchanges colors, and
runs deterministic-best play for 12 total games. The opponent is the exact
UI-bound `5.4.1-transactional-deadline-root-parallel-5s` profile rather than
the frozen four-star profile.

## Result

| Candidate role | W/D/L | Score |
|---|---:|---:|
| Black | 3/0/3 | 50.0% |
| White | 3/0/3 | 50.0% |
| Overall | 6/0/6 | 50.0% |

The paired result is completely color/opening driven:

- Openings 60, 61, and 65: black won with either model.
- Openings 62, 63, and 64: white won with either model.
- Exchanging candidate and 5.4.1 therefore did not change the winner on any
  opening identity.

## Guard and runtime

- Candidate decisions: 109; p50/p95/max 305.646/4261.554/4673.444 ms.
- 5.4.1 decisions: 109; p50/p95/max 179.304/4500.294/4512.981 ms.
- Decisions above 5,000 ms: 0 for both profiles.
- Runner anomalies: 0.
- Candidate guard: 88 eligible decisions, 202 audited candidates, 45 scoped
  disproofs, 46 unknowns, 111 verified-loss candidates, and 3 decisions that
  avoided a verified losing provisional move.
- The three avoided-loss events occurred once with the candidate as white on
  opening 60 and twice with the candidate as black on opening 63. Those games
  still ended as candidate losses, so the defensive corrections did not
  produce a W/D/L gain in this sample.

## Provenance and limitation

Command:

```sh
five_chess_benchmark \
  --output followup-direct-candidate-vs-v541.jsonl \
  --profile five-star-opponent-guard \
  --suite opponent-guard-smoke \
  --random-mode best --strategy hybrid \
  --seed 0x4755415244534d4b \
  --opening-start 60 --opening-count 6 --max-moves 120 \
  --forbidden-black 0 --opponent five-star-control --paired-phase 0
```

Raw JSONL SHA-256:
`8add219d5e696a38ce06b9b307b0bdeff519745ed138505bf4a6b6b8712d71dc`.

Twelve games are too few for a strength conclusion. The result only says that
this paired sample found no direct W/D/L advantage for either profile; the
guard did change three verified-losing provisional decisions.

