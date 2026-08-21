# Early micro-VCF verification record

Date: 2026-08-20 (Asia/Shanghai)

## Deterministic correctness

- `tools/run_five_chess_ai_tests.sh`: passed twice before the sanitizer
  audit and once after the stack-layout correction.
- The suite includes frozen one-through-four-star and 5.4.1/5.8.0 profile
  identity checks; sentinel skip/eligibility; positive and corrupt certificate
  replay; scoped disproof and unknown handling; both rules and eight
  symmetries; board restoration; illegal, overflow, deadline, rollback,
  ledger, cache-scope, no-alternative, corpus, and final-guard interactions.
- The final ordinary run uses the frozen 80 ms sentinel budget and prints
  `FiveChessAI tests passed`.

## ASan and UBSan

Build:

```sh
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  'ice five chess/FiveChessAI.c' tools/five_chess_ai_tests.c -lm \
  -o /private/tmp/five_chess_ai_tests_early_vcf_sanitized
```

Run:

```sh
env ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  /private/tmp/five_chess_ai_tests_early_vcf_sanitized
```

Result: passed with `FiveChessAI tests passed`, with no ASan or UBSan report.

The first sanitizer attempt exposed that O1 inlined both full-certificate guard
stages into the already large five-star analysis frame. The implementation now
keeps those explicit stages `noinline`, stores the sentinel rollback snapshot
in the caller result buffer, and allocates the four-entry verified-certificate
cache on the heap under the existing decision-ledger memory reservation. The
cache is released before ledger telemetry is published. These changes affect
storage only, not search policy or evidence semantics.

Sanitizer instrumentation makes the targeted proof matrix substantially
slower. That test-only matrix uses an 800 ms local ceiling when a sanitizer is
detected so it continues to check proof, replay, and integrity semantics. The
ordinary suite and runtime profile continue to enforce the frozen 80 ms
production budget.
