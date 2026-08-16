# Interim proof-engine performance evidence

Status: **formal strength schedule remains blocked**. These are non-final
diagnostics and may be used for implementation choices only.

## Primitive equality and warm-query throughput

Command: `tools/run_five_chess_primitive_benchmark.sh`

- Both rule modes passed hash, line/winner, VCT threat-list, and all-point
  legality equality checks before timing.
- Incremental position size: 54,688 bytes.
- Declared batch per rule/iteration: 16 board keys, one winner/line scan, one
  VCT threat batch, and 225 black legality queries.
- 3 iterations per rule: reference 83.201 ms; warmed incremental 0.074 ms;
  1124.33x read throughput.
- This number deliberately excludes initialization and make/unmake cost. It
  demonstrates cached primitive activity but is not used as an end-to-end
  latency claim.

## Fixed first-decision diagnostics before lazy Session allocation

Each comparison uses the first new-engine decision from the same 8 elite
diagnostic opening identities with both colors (16 fixed decisions per rule).
Later divergent game states are excluded from the percentile comparison.

| Rule | Engine | p50 ms | p95 ms | max ms | mean ms |
|---|---|---:|---:|---:|---:|
| no-forbidden | production v5.1 | 283.112 | 4188.346 | 4188.346 | 678.252 |
| no-forbidden | candidate, deadline build | 284.618 | 2329.861 | 2329.861 | 543.737 |
| forbidden | production v5.1 | 359.889 | 6562.743 | 6562.743 | 1093.064 |
| forbidden | candidate, frozen-baseline build | 360.505 | 6567.019 | 6567.019 | 1189.734 |

All 32 candidate first moves matched v5.1 after the in-Session frozen-baseline
experiment was reverted. The table still fails the strict candidate-p50 and
forbidden-p95 gates by small margins. The implementation now creates the
candidate Session lazily, after these measurements; a fresh larger fixed set
is required before the performance gate can pass.

## Fixed first-decision diagnostics after lazy Session and corpus skip

Command shape (run separately for each profile and rule so timing processes do
not contend):

```sh
five_chess_benchmark --profile five-star-v51|five-star \
  --suite elite-fixed --random-mode best --strategy hybrid \
  --opening-start 0 --opening-count 8 --max-moves 225 \
  --forbidden-black 0|1 --opponent four-star --output RESULT.jsonl
```

The eight elite diagnostic openings are evaluated with both candidate colors,
for 16 first candidate decisions per rule. `elite-fixed` stops immediately
after that decision. The original `--max-moves 16` trial was discarded because
three openings already contained 16--17 stones and therefore produced only 13
decisions. Percentiles use the benchmark's predeclared nearest-rank definition.

| Rule | Engine | p50 ms | p95 ms | max ms | mean ms |
|---|---|---:|---:|---:|---:|
| no-forbidden | production v5.1 | 288.546 | 4250.366 | 4250.366 | 687.351 |
| no-forbidden | candidate | 287.121 | 2336.094 | 2336.094 | 546.947 |
| forbidden | production v5.1 | 362.247 | 6698.114 | 6698.114 | 1102.120 |
| forbidden | candidate | 361.658 | 6681.638 | 6681.638 | 1207.023 |

All 32 candidate moves match production v5.1. Candidate p50 and p95 are no
greater than v5.1 in each rule mode, and candidate max is also lower. The
forbidden candidate mean is higher because opening 0 with candidate white
regressed from 1195.879 ms to 2960.803 ms; this long-tail case remains an
optimization target and is not hidden by the passing percentile gate.

Across the 16 no-forbidden decisions, candidate Session hits were 150 versus
0, allocations fell from 108 to 86, allocated bytes fell from 170,393,600 to
123,207,680, and board-hash cell visits fell from 4,990,725 to 2,291,850.
Across forbidden decisions, Session hits were 25 versus 0, allocations fell
from 87 to 79, and allocated bytes fell from 131,072,000 to 112,721,920.
Forbidden legality calls rose from 425,321,580 to 453,647,776, consistent with
the remaining mean-latency regression.

This is still non-final evidence. Implementing the outstanding thresholded
DFPN and sound relevance work will invalidate these timings and requires a
fresh paired run before task 8.5 can be checked and before formal games begin.

## Thresholded DFPN and relevance-accounting build

The candidate now uses an explicit bounded AND/OR graph, proof/disproof
thresholds, deterministic most-proving expansion, compatible gain/cost/rest
dependency ordering, exact partial-enumeration unknown semantics, shared graph
progress, and independent certificate reconstruction/replay. C tests, frozen
legacy-three-star golden, ASan, and UBSan pass. The primitive benchmark remains
semantically equal at 1098.77x warmed read throughput (89.001 ms reference,
0.081 ms incremental).

On the first 16-decision run, all 32 moves still matched v5.1. Candidate
activity was nonzero: no-forbidden recorded 769 most-proving expansions, 24
completed root scoped disproofs, 2,543 graph nodes, 2,561 edges, and 827,910
verified relevance omissions; forbidden recorded 116 expansions, 4 completed
root scoped disproofs, 439 nodes, 425 edges, and 251,808 verified omissions.
Both rules recorded zero relevance-unresolved and zero graph-arena exhaustion.
Production v5.1 recorded zero DFPN graph activity, confirming path isolation.

Because single-process-order p50 moved by several milliseconds, the
no-forbidden gate was repeated three times with alternating profile order and
per-position medians before percentiles. It passed:

| Rule | Engine | p50 ms | p95 ms | max ms | mean ms |
|---|---|---:|---:|---:|---:|
| no-forbidden | production v5.1 | 289.270 | 4232.675 | 4232.675 | 685.661 |
| no-forbidden | thresholded DFPN candidate | 287.906 | 2135.971 | 2135.971 | 518.592 |

Forbidden corpus comparisons were then ablated. Sharing DFPN while forbidden
legality still used the full-board oracle caused a deterministic 1.78-second
regression on one multi-candidate corpus position, so that combination now
conservatively retains the frozen per-candidate proof path; other forbidden
DFPN stages remain enabled. The regression disappeared, but three-run
separate-process medians still failed the strict gate:

| Rule | Engine | p50 ms | p95 ms | max ms | mean ms |
|---|---|---:|---:|---:|---:|
| forbidden | production v5.1 | 361.577 | 6643.842 | 6643.842 | 1090.271 |
| forbidden | hybrid candidate | 362.717 | 6759.429 | 6759.429 | 1099.994 |

Task 8.5 and formal games remain blocked. The next benchmark revision must run
v5.1 and candidate back-to-back per fixed position in one process, alternating
order, so thermal/process-order noise can be separated from a real regression.
The gate will not be relaxed; if paired p50 or p95 still regresses, the
candidate requires further optimization.

## Rejected variants

1. Unconditionally add a depth-12 own-win query: one-opening p95 rose from
   about 265 ms to about 397 ms without changing a move.
2. Re-run frozen four-star inside the candidate global Session: this changed
   a frozen forbidden-rule decision because the shared 620 ms ceiling was
   narrower than multiple independent frozen 160 ms queries. Reverted.
3. Incrementally recompute forbidden legality on every crossing line:
   forbidden p95 regressed; forbidden mode now uses the complete reference
   legality fallback while retaining incremental hash/session state.
4. Check the deadline only at proof-node boundaries: a single threat batch
   could overrun by seconds. Deadline checks now live inside threat and
   refutation enumeration and incomplete enumeration returns unknown.

## Correctness/build gates completed so far

- C tests with `-Wall -Wextra -Werror -pedantic`: pass.
- Frozen legacy-three-star golden: pass.
- ASan + UBSan C suite: pass with no findings.
- Random long make/unmake sequences in both rule modes: pass.
- iOS Simulator Debug build: pass. Existing asset-name, legacy Objective-C
  precedence, and deployment-target warnings remain unrelated baseline
  warnings.

## User-approved five-second hard decision ceiling (v5.3.1)

On 2026-08-14 the latency acceptance rule was amended: p50/p95 against v5.1
remain reported, but sub-millisecond percentile ordering is no longer a
promotion gate. Every player-visible candidate decision must instead return
the best completed legal move within 5,000 ms. The C candidate stops at a
4,500 ms global deadline, shared by all nested ordinary/proof/corpus stages,
leaving 500 ms for the Objective-C hint pass, result finalization, and final
legality validation. Unfinished proof work becomes unknown. A dedicated test
with a 1 ms deadline returns a legal move, sets `budgetExhausted`, and finishes
well below the external limit.

The benchmark was rebuilt and rerun from scratch. Each rule used four paired
repetitions with fixed seed `0xe117ed1a62026081`, phases `0,1,0,1`, eight
fixed opening identities, and both colors. The gate examines the raw maximum
across all 64 candidate samples per rule; per-position medians are used only
for the descriptive percentiles.

| Rule | Engine | p50 ms | p95 ms | median-position max ms | raw max ms | hard gate |
|---|---|---:|---:|---:|---:|---|
| no-forbidden | production v5.1 | 287.243 | 4243.913 | 4243.913 | 4303.464 | control only |
| no-forbidden | candidate v5.3.1 | 289.134 | 2165.138 | 2165.138 | 2244.294 | pass |
| forbidden | production v5.1 | 361.430 | 6846.845 | 6846.845 | 6891.061 | control only |
| forbidden | candidate v5.3.1 | 362.669 | 4508.203 | 4508.203 | 4508.268 | pass |

All candidate moves were deterministic across repetitions and matched v5.1
on all 32 rule/color fixed positions. Proof metadata was deterministic in
both rule suites; deterministic node counts remain separately reported as
false because wall-clock-bounded v5.1 and candidate searches can stop at
different node boundaries.

No-forbidden candidate activity totaled 769 most-proving expansions, 57
compatible Session hits, 24 completed scoped disproofs, 109 dependency
combinations, 839,608 verified relevance omissions, zero relevance-unresolved,
and zero graph-arena exhaustion. Candidate allocated 142,868,480 bytes across
86 allocations versus v5.1's 170,393,600 bytes across 108 allocations.

The fixed forbidden corpus path deliberately retained the frozen proof route:
it therefore recorded zero DFPN graph expansion on this corpus-shaped suite.
It still reduced the long tail through the global decision deadline, returned
stable legal moves, recorded 310,389 verified omissions with zero unresolved
relevance, and reduced legality calls from 425,975,486 to 363,921,774 in the
representative summary. Dedicated both-rule DFPN/scoped-disproof fixtures,
not this corpus suite, remain the activity authority for the forbidden mode.

Summary checksums:

- no-forbidden: `9a05162cb7977a304443551e7d46b83233568acae3f86be00e85596e5cc36275`
- forbidden: `1e3d9206a0dade98aaf171d6e483a45dd39b9e730d92ab7206a949eb1716edaa`

The complete raw-input hashes are embedded in each summary. Formal strength
games remain blocked until all correctness/activity/ablation gates pass and a
post-gate freeze manifest is written.

This v5.3.1 hard-ceiling run was subsequently invalidated as the final 8.5
gate by the addition of threat dependency bitsets and AND-node relevance-zone
state. It remains valid evidence for the deadline mechanism at that source
revision. Task 8.5 must be rerun on the final pre-freeze binary.

## Post-dependency hard-ceiling rerun (task 8.5)

After the threat dependency, relevance-zone, DAG, and incremental line-code
changes, the benchmark and primitive tools were rebuilt from the current
working source. The strict C suite passed, and the primitive reference and
incremental batches remained semantically equal with a measured aggregate
speedup of 1088.609x (`referenceMs=81.646`, `incrementalMs=0.075`).

Each rule then used four serial paired repetitions with fixed seed
`0xe117ed1a62026081`, phases `0,1,0,1`, eight diagnostic opening identities,
both colors, evaluation randomness disabled, and `maxMoves=120`. The latency
measurement includes the legacy hint computation. The raw maximum across all
64 candidate samples per rule is the hard-limit authority; per-position
medians are used only for descriptive p50/p95/max/mean values.

| Rule | Engine | p50 ms | p95 ms | median-position max ms | mean ms | raw max ms | 5,000 ms gate |
|---|---|---:|---:|---:|---:|---:|---|
| no-forbidden | production v5.1 | 291.818 | 4407.559 | 4407.559 | 710.359 | 4456.739 | control only |
| no-forbidden | candidate | 329.406 | 4494.797 | 4494.797 | 986.760 | 4550.359 | pass |
| forbidden | production v5.1 | 361.686 | 6796.986 | 6796.986 | 1109.726 | 6867.681 | control only |
| forbidden | candidate | 361.023 | 1456.633 | 1456.633 | 764.858 | 1475.020 | pass |

Candidate moves were deterministic across repetitions in both rules, legal,
and returned under 5,000 ms. Candidate-v5.1 move equality is reported but is
not a performance gate: the candidate differed from v5.1 on at least one
position in each rule, as a stronger search is allowed to change the final
move. Descriptive percentile comparisons also remain non-gating under the
user-approved hard-ceiling rule.

The no-forbidden representative summaries recorded 254 most-proving
expansions, 12 compatible session hits, 18 completed scoped disproofs, 612,514
verified relevance omissions, five global terminations, and zero arena
exhaustions. The forbidden summaries recorded 48 most-proving expansions,
six compatible session hits, eight completed scoped disproofs, 252,980
verified omissions, one global termination, and zero arena exhaustions. Full
work counters and raw hashes are retained in:

- `evidence/fixed_position_hard_5s/finalpaired2_free_summary.json`
- `evidence/fixed_position_hard_5s/finalpaired2_forbidden_summary.json`
- the eight adjacent raw JSONL inputs referenced by those summaries

Summary SHA-256:

- no-forbidden: `09388ead10115aa28ed6f0257f1657f9ce46f4bb9ce17f795de5dc116e8682a1`
- forbidden: `63dccaf0342c3b394364778d91d98802f1ac971cc19bc85d1c87376c177cb18d`

Task 8.5 passes on this source state. Any later engine/profile/tool change
still invalidates this evidence and requires another pre-freeze rerun.

## Post-related-zone correctness/build renewal

After iterated related-zone certificates, independent scoped-disproof replay,
and adversarial relevance tests were added, the strict C suite and frozen
one/two/three-star golden suite passed. ASan/UBSan passed with
`ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`; randomized long
make/unmake sequences for both rules are part of that run. The iOS Simulator
Debug target built successfully. Existing duplicate-asset, deployment-target,
and legacy Objective-C precedence warnings remain unchanged baseline warnings.

The candidate session allocates 10,747,904 bytes. The largest held-out
activity fixture used 453 graph nodes and 456 edges with zero exhaustion under
the 32,768-node/131,072-edge caps. `FCIncrementalPosition` is 97,424 bytes.

The renewed primitive benchmark passed semantic equality at 1179.861x warmed
throughput (`referenceMs=90.850`, `incrementalMs=0.077`). These runs renew
tasks 8.1--8.4 but do not renew task 8.5; the four-repetition hard-ceiling
comparison below must still be regenerated from this source state.

## Final pre-freeze hard-ceiling renewal

The final source used four serial paired repetitions per rule with fixed seed
`0xe117ed1a62026081`, phases `0,1,0,1`, eight opening identities and both
colors. Candidate move determinism and the raw maximum across all 64 candidate
samples per rule are the gate authorities.

| Rule | Engine | p50 ms | p95 ms | median-position max ms | mean ms | raw max ms | 5,000 ms gate |
|---|---|---:|---:|---:|---:|---:|---|
| no-forbidden | production v5.1 | 290.878 | 4328.288 | 4328.288 | 700.277 | 4335.231 | control only |
| no-forbidden | candidate | 329.484 | 4405.319 | 4405.319 | 973.603 | 4448.157 | pass |
| forbidden | production v5.1 | 362.930 | 6757.312 | 6757.312 | 1103.753 | 6772.796 | control only |
| forbidden | candidate | 361.297 | 1455.593 | 1455.593 | 764.400 | 1457.176 | pass |

Both candidate suites are deterministic and pass the user-approved hard
ceiling. Candidate/control move equality is false in each rule, which is
allowed for a stronger candidate. No-forbidden p50/p95 are descriptively
higher than v5.1 but are not gates; forbidden p50/p95 are lower. The candidate
recorded zero relevance-unresolved and zero graph-arena exhaustion in both
rules. Machine-readable summaries and their four raw inputs use the
`finalpaired3_*` names in `evidence/fixed_position_hard_5s/`.

Summary SHA-256:

- no-forbidden: `fdd683091c6a5f626b7544d2206f5e67e4f8fc7eb3ce6e0db23cfb354c29c3e4`
- forbidden: `9819d96f619ffd48078bed6f6c1230afcebb4c22dff73cab3f82356ddcf17e06`
