# Early micro-VCF candidate vs UI-bound 5.4.1

Date: 2026-08-20 (Asia/Shanghai)

This is a 12-game directional quick test, not a strength or promotion claim.
It uses six frozen natural freestyle prefixes (opening IDs 66-71), exchanges
colors, and runs deterministic-best play against exact UI-bound
`5.4.1-transactional-deadline-root-parallel-5s`.

## Candidate black first

| Candidate role | W/D/L | Score |
|---|---:|---:|
| Black | **6/0/0** | 100.0% |
| White | 0/0/6 | 0.0% |
| Overall | 6/0/6 | 50.0% |

Every one of the six paired openings was won by black with either model.
There was therefore no paired-opening winner change and no evidence here that
the sentinel caused the 6-0 candidate-black result. The sample remains wholly
color/opening driven, just like the earlier 60-65 direct test.

| Opening | Candidate black | Candidate white | Paired outcome |
|---:|---:|---:|---|
| 66 | Win | Loss | Black won both |
| 67 | Win | Loss | Black won both |
| 68 | Win | Loss | Black won both |
| 69 | Win | Loss | Black won both |
| 70 | Win | Loss | Black won both |
| 71 | Win | Loss | Black won both |

## Defensive behavior

The candidate made 156 decisions (63 as black, 93 as white). The sentinel was
eligible on 150 decisions and performed 183 bounded candidate audits.

- It encountered 48 verified-loss candidate audits and changed the early
  provisional move on 2 decisions. Both events occurred with the candidate as
  white, and both games were still lost.
- Opening 67, white: early VCF distance 7 rejected `[5,7]` and chose `[9,8]`;
  downstream own/final work later selected `[9,11]` (81.265 ms sentinel,
  three audits, two verified-loss alternatives).
- Opening 71, white: early VCF distance 7 rejected `[8,10]` and chose `[2,6]`,
  which remained the final move (39.522 ms, two audits, one verified loss).
- The authoritative final guard caught 22 provisional losses that the shallow
  sentinel did not catch. All were candidate-white decisions. Seventeen still
  had only verified-loss final choices, while five reached an unresolved
  selected class; three of those five were recorded as final-guard avoidance.
- The final guard changed a verified-losing provisional move on 4 decisions
  total. One was the opening-67 downstream correction after the early
  replacement, so the sentinel and final guard changed different stages of
  the same decision.
- No early/final evidence mismatch and no rollback occurred.

This evidence supports the intended narrow claim: the low-cost sentinel can
prevent some shallow VCF mistakes before expensive own search, but it does not
replace the depth-9 VCF/conditional depth-10 VCT acceptance boundary and did
not rescue either affected game in this small sample.

## Runtime

Nearest-rank latency statistics:

| Scope | Decisions | p50 | p95 | max |
|---|---:|---:|---:|---:|
| Candidate overall | 156 | 425.961 ms | 4,507.769 ms | 4,622.268 ms |
| Candidate black | 63 | 118.267 ms | 4,507.769 ms | 4,562.671 ms |
| Candidate white | 93 | 1,110.882 ms | 4,507.054 ms | 4,622.268 ms |
| 5.4.1 control | 156 | 170.964 ms | 3,114.432 ms | 4,500.442 ms |
| Sentinel eligible decisions | 150 | 0.250 ms | 81.924 ms | 82.840 ms |

The 80 ms setting bounds the VCF query slice; the serialized wrapper elapsed
time includes entry/replay/accounting overhead and reached 82.840 ms. There
were 23 sentinel deadline exhaustions, all conservatively classified rather
than treated as safe. Across eligible decisions, downstream wall-clock budget
remaining was 2,137.424/2,934.558/3,095.244/3,096.753 ms min/p50/p95/max.

The sentinel consumed 3,863 fresh nodes. Its verified result cache was reused
on 16 decisions with 47 diagnostic result-cache hits. No decision exceeded
5,000 ms, no game reported an anomaly, and the maximum was candidate white on
opening 70, ply 17, `[9,10]`, at 4,622.268 ms. Independent replay found no
illegal move, board mutation, winner mismatch, or certificate failure.

## Replay and provenance

The replay validated all 384 moves and both winners/terminations for all 12
games. It cold-reproved all 17 early VCF and 18 final-guard VCF certificates
whose exact roots are serialized, restoring the board after every query. It
also checked all 74 general analysis certificates for the runtime independent
verification flag and nonzero certificate ID; their historical provisional
roots/certificate nodes are not serialized in JSONL, so a second exact-node
replay is not possible from this artifact alone. No VCT win certificate was
reported in this run.

Command:

```sh
five_chess_benchmark \
  --output quick-match-candidate-vs-v541.jsonl \
  --profile five-star-early-vcf \
  --suite opponent-guard-smoke \
  --random-mode best --strategy hybrid \
  --seed 0x4755415244534d4b \
  --opening-start 66 --opening-count 6 --max-moves 120 \
  --forbidden-black 0 --opponent five-star-control --paired-phase 0
```

Core artifact SHA-256:

- raw JSONL: `490d4a3866298bed4c397b3446d09ca4d4092573619ef9e5752a3bca95902f5f`
- opening schedule: `6878817bebf2f596ab626df84d5f46b86c69026da0d2d5c9850af40bf4a8ffba`
- replay library: `f54b3a64d32a99be6092b2a0d9d65f995557d35f4685cd05abad74ec334cb9f5`

The playable UI remains on 5.4.1 regardless of this result.
