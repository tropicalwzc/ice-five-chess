# Early micro-VCF change evidence index

Date: 2026-08-20 (Asia/Shanghai)

## Outcome

The selected research candidate is
`5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`: strict opponent VCF at adaptive
depth 5/7, 16,000 nodes, 80 ms, and at most two early alternatives. Only an
independently replayed `PROVEN_WIN` rejects a candidate. Shallow disproof and
`unknown` remain scoped/unresolved, and the existing depth-9 VCF/conditional
depth-10 VCT final guard remains authoritative.

Targeted A/B/C evidence found adaptive equal to fixed depth 7 on in-scope
recall (4/4 distance fixtures and 1/3 reconstructed late corrections), with
zero false rejection and a slightly lower measured p95. The missed diagnostic
positions are one depth-9 VCF beyond the 80 ms slice and one VCT-only loss, so
the early policy was not broadened.

The fresh directional quick match against exact UI-bound 5.4.1 was:

- candidate black: 6/0/0;
- candidate white: 0/0/6;
- overall: 6/0/6.

Black won both color assignments on all six openings, so the result is
opening/color driven and does not establish a strength gain. The sentinel did
avoid two verified-losing early candidates (both candidate-white games, both
still lost). The final guard caught 22 further provisional losses. There were
no anomalies, no decisions above 5,000 ms, no rollback, and no evidence
mismatch. The UI remains on 5.4.1; promotion is outside this change.

## Evidence

- `baseline.md`: pre-change source/profile hashes and UI binding.
- `late-correction-fixtures.json`: three diagnostic-only reconstructed boards.
- `targeted-policy-probe.jsonl` and `targeted-policy-probe-repeat.jsonl`: raw
  A/B/C and full-guard probe runs.
- `targeted-policy-selection.md`: correctness gates, deterministic comparison,
  policy metrics, limitations, and frozen selection.
- `verification.md`: repeated strict suite plus ASan/UBSan build and result.
- `pre-match-profiles.json` and `pre-match-freeze.md`: exact candidate/control
  profiles, source hashes, parameters, and settings frozen before schedule
  generation.
- `quick-match-openings.json`: six opening identities 66-71 and hashes.
- `quick-match-candidate-vs-v541.jsonl`: raw 12-game result.
- `quick-match-replay.json`: 384-move legality/winner/integrity replay, 35 cold
  certificate reproofs with serialized roots, and deadline/anomaly audit.
- `quick-match-report.md`: black-first directional outcome, defensive events,
  latency, reuse, and small-sample interpretation.
- `quick-match-provenance.sha256`: seed plus source, rule, corpus, runner,
  profile, schedule, raw, replay, and report checksums.

## Requirement/scenario coverage

- Isolated profile, disabled defaults, parent/control identity, UI isolation:
  profile snapshot plus `test_early_micro_vcf_profile_and_semantics` and
  `test_opponent_guard_profile_and_reserved_ledger`.
- Early placement, immediate-win skip, mandatory defense, bounded ledger, and
  downstream continuation: implementation-stage assertions plus full analysis
  interaction tests in `tools/five_chess_ai_tests.c`.
- Certificate-safe rejection, malformed certificate, scoped disproof,
  `unknown`, overflow, illegal placement, deadline, rollback, and restoration:
  strict C suite and the two targeted raw probe runs.
- Fixed d5, fixed d7, adaptive signal, both rules, and eight symmetries:
  `test_opponent_guard_distance_matrix_symmetry_and_rules` plus targeted policy
  evidence.
- Certificate-directed replacement, deterministic ordering, cache key scope,
  independent cache replay, shallow-disproof retirement, and authoritative
  final guard/corpus interaction: strict C suite and quick-match telemetry.
- Fresh paired evaluation, exact 5.4.1, black-first results, runtime health,
  raw replay, and provenance: quick-match schedule/raw/replay/report/checksums.

## Standard 100-game follow-up

The requested larger Freestyle follow-up uses 50 natural openings with colors
exchanged against each opponent (100 games per cell, 200 total). See
[`standard-tests/report.md`](standard-tests/report.md) for the black/white W/D/L,
paired outcomes, latency, defensive telemetry, strict deadline finding, full
replay, checksums, and reproduction commands. Raw and replayable evidence is in
[`standard-tests/`](standard-tests/).

## Reproduction commands

Correctness and sanitizer:

```sh
tools/run_five_chess_ai_tests.sh
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  'ice five chess/FiveChessAI.c' tools/five_chess_ai_tests.c -lm \
  -o /private/tmp/five_chess_ai_tests_early_vcf_sanitized
env ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  /private/tmp/five_chess_ai_tests_early_vcf_sanitized
```

Targeted probe:

```sh
clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  'ice five chess/FiveChessAI.c' tools/five_star_early_vcf_probe.c -lm \
  -o /private/tmp/five_star_early_vcf_probe
/private/tmp/five_star_early_vcf_probe targeted-policy-probe.jsonl
```

Quick match:

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
"$BENCHMARK_PATH" \
  --output quick-match-candidate-vs-v541.jsonl \
  --profile five-star-early-vcf --suite opponent-guard-smoke \
  --random-mode best --strategy hybrid \
  --seed 0x4755415244534d4b \
  --opening-start 66 --opening-count 6 --max-moves 120 \
  --forbidden-black 0 --opponent five-star-control --paired-phase 0
```

Replay and validation:

```sh
clang -dynamiclib -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  'ice five chess/FiveChessAI.c' tools/opponent_guard_replay_probe.c -lm \
  -o /private/tmp/libfive_chess_early_vcf_replay.dylib
python3 tools/replay_early_vcf_quick_match.py \
  --library /private/tmp/libfive_chess_early_vcf_replay.dylib \
  --schedule quick-match-openings.json \
  --jsonl quick-match-candidate-vs-v541.jsonl \
  --output quick-match-replay.json
openspec validate add-five-star-early-micro-vcf-sentinel --strict
```

## Remaining limitations

- A 12-game, single-rule run cannot establish statistical superiority or a
  general black advantage.
- The 80 ms VCF sentinel intentionally misses deeper VCF and VCT-only losses;
  the final guard is still required.
- JSONL stores IDs and verification flags for general analysis certificates,
  but not every certificate node and historical provisional root. Exact cold
  reproof is therefore limited to the 35 early/final certificates whose roots
  are serialized; all 74 general certificates were checked for the runtime
  independent-verification flag and nonzero ID.
- Candidate latency remains close to the existing hard ceiling in a small
  tail: observed maximum 4,622.268 ms, with zero decisions above 5,000 ms.
