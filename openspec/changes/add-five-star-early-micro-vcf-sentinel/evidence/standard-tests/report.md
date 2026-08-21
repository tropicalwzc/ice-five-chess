# 5.8.1 Standard 100-Game Tests

## Test contract

- Candidate: `five-star-early-micro-vcf-sentinel-candidate@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`.
- Opponents: frozen four-star `3.0.0-vcf-dfpn-no-book` and exact UI-bound `5.4.1-transactional-deadline-root-parallel-5s`.
- Suite: `five-star-natural-final`, Freestyle, deterministic-best, hybrid-deep-verified.
- Schedule: master seed `0x9ee4d91480ac5889`, natural openings 0–49, each opening played twice with colors exchanged.
- Size: 100 games per opponent, exactly 50 candidate-black and 50 candidate-white games; 200 games total.
- Maximum game length: 120 moves. Candidate decision hard limit: 5,000 ms.
- Sentinel: adaptive depth 5/7, 16,000 nodes, 80 ms query budget, at most two early alternatives.

## Results

| Opponent | Candidate black W/D/L | Candidate white W/D/L | Overall W/D/L | Overall score | 95% Wilson interval |
| --- | ---: | ---: | ---: | ---: | ---: |
| Four-star | 35/0/15 | 21/0/29 | 56/0/44 | 56.0% | 46.2%–65.3% |
| Exact 5.4.1 | 33/0/17 | 19/0/31 | 52/0/48 | 52.0% | 42.3%–61.5% |

Color-specific score intervals were 70.0% [56.2%, 80.9%] as black and 42.0% [29.4%, 55.8%] as white against four-star, and 66.0% [52.2%, 77.6%] as black and 38.0% [25.9%, 51.8%] as white against 5.4.1.

The 5.4.1 point estimate is mildly positive, but it does not establish superiority. Of the 50 paired openings, the candidate swept 9, 5.4.1 swept 7, and 34 split one game each. Against four-star, the corresponding counts were 10 candidate sweeps, 4 opponent sweeps, and 36 splits. No pair involved a draw. The color-exchanged splits dominate both cells, so the overall result must not be interpreted as a theoretical color advantage or a statistically demonstrated model improvement.

## Runtime and defensive telemetry

| Opponent | Candidate decisions | Opponent decisions | Candidate ms p50/p95/max | Opponent ms p50/p95/max | Eligible sentinel ms p50/p95/max |
| --- | ---: | ---: | ---: | ---: | ---: |
| Four-star | 1,643 | 1,637 | 630.397 / 4,502.678 / 4,862.800 | 162.871 / 335.924 / 1,055.249 | 4.643 / 81.570 / 90.085 |
| Exact 5.4.1 | 1,694 | 1,692 | 390.509 / 4,501.341 / 4,867.479 | 178.291 / 4,500.247 / 5,074.030 | 3.643 / 81.645 / 90.400 |

| Opponent | Early verified losses avoided | Early move changes | Final-guard-only catches | Final guard avoided losses | Cache-reuse decisions / hits | Rollbacks | Evidence mismatches |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Four-star | 7 | 7 | 176 | 16 | 134 / 3,866 | 0 | 0 |
| Exact 5.4.1 | 13 | 13 | 186 | 23 | 152 / 2,804 | 0 | 0 |

The sentinel audited 401 verified-loss alternatives against four-star and 453 against 5.4.1. It was eligible on all 1,643 candidate decisions against four-star and 1,642 of 1,694 candidate decisions against 5.4.1; the remainder were legitimate skip cases such as immediate wins.

Candidate-side runtime health passed in both cells: zero candidate decisions exceeded 5,000 ms. The strict whole-cell runtime gate did not pass for the 5.4.1 cell because the control itself took 5,074.030 ms at opening 12, candidate white, ply 10, control black move `(5,9)`. A same-parameter diagnostic rerun of the opening-12 pair reproduced the same control move above the hard limit at 5,047.209 ms. The violation is therefore reproducible and belongs to exact 5.4.1 rather than the new candidate. The four-star cell had zero decisions above 5,000 ms. There were zero runner anomalies in both main cells.

## Independent replay

Both raw files passed full replay with the current engine and rule helper:

- Four-star cell: 100 games and 4,014 moves replayed; 1,643 candidate and 1,637 control decisions checked; 139 early-VCF and 156 final-guard VCF certificates independently re-proved (295 total).
- 5.4.1 cell: 100 games and 4,120 moves replayed; 1,694 candidate and 1,692 control decisions checked; 157 early-VCF, 166 final-guard VCF, and 2 final-guard VCT certificates independently re-proved (325 total).
- Opening-12 timeout diagnostic: both color-exchanged games replayed and all 9 replayable certificates re-proved.
- Move legality, side order, shared paired-opening prefixes, terminal winners, caller-board restoration, and board integrity passed in both cells.
- The original 12-game quick-match replay was rerun after parameterizing the replay tool and still passed with all 35 replayable certificates re-proved.

## Conclusion

The larger test supports retaining the early micro-VCF sentinel as a useful research candidate: it avoided 7 and 13 shallow verified losing choices at low median cost, stayed below the candidate hard deadline, and produced positive overall point estimates against both controls. It does not justify a superiority claim or UI promotion. White performance remains weak in both cells, the paired evidence is dominated by color splits, and the 5.4.1 cell is not a clean strict-deadline comparison because the exact control crossed 5 seconds once. The playable UI binding remains unchanged on 5.4.1.

## Reproduction

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)

"$BENCHMARK_PATH" \
  --output v581-vs-four-star-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 0 --opponent four-star --paired-phase 0

"$BENCHMARK_PATH" \
  --output v581-vs-v541-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 0 --opponent five-star-control --paired-phase 0
```

## SHA-256

```text
5a952f94004f893e699b03f818c81ef54d01f1da64bc4d4c4523663de04c7b29  FiveChessAI.c
c37286ff5daef343dff0ab102d4bdd75c859322461297f6c3bbed5259b23d3b4  FiveChessAI.h
f2fdb3df517e64010c8f0cfb642f54614acc889db85a908669f24c5d7062908a  five_chess_benchmark.m
817749faa0eb28549b0cc2d745b85311b43455c89a07bb27527de01d2dee3abe  replay_early_vcf_quick_match.py
2930810065a35b63059d271cd7e9c0e6ff9e03479e8f02f269afa6bef5c0fbd9  v581-vs-four-star-100g.jsonl
e27720fcac9c738acce3084413109f230090fa554b3f10168e978a000aa52f76  v581-vs-v541-100g.jsonl
e5bb483ceffb4ff893f0e76ed45824ca27fb56993caf0b4d1f56199b7991d582  v581-vs-four-star-100g-replay.json
2bfe5419885c703d54657ced8920ab4ac6aff76b0fd22546b7f888ce1c0e627e  v581-vs-v541-100g-replay.json
e9c8db5d71585ed83e3a72fd4bead444859af6955514972a507c2dd7875bb2f2  v581-vs-v541-opening12-timeout-repeat.jsonl
222a82c444434ddc83121ce4384280033dd3c4d45067de81551ef852b876891d  v581-vs-v541-opening12-timeout-repeat-replay.json
```
