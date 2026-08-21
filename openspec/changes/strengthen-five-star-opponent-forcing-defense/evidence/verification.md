# Opponent-guard verification record

Date: 2026-08-20 (Asia/Shanghai)

## Correctness and isolation

- `tools/run_five_chess_ai_tests.sh`: passed after the guard acceptance-path,
  symmetry/rule, restoration, deterministic-repeat, certificate-corruption,
  overflow/deadline, corpus-conflict, mandatory-continuation, and lower-profile
  isolation assertions were added.
- `tools/run_legacy_three_star_golden.sh`: `legacy one/two/three-star golden
  tests passed`.
- The complete C suite includes randomized incremental make/unmake/reference
  equivalence and fixed-seed deterministic regeneration checks.
- ASan+UBSan build and run:
  `clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic
  -fsanitize=address,undefined -fno-omit-frame-pointer ...` followed by
  `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1
  /private/tmp/five_chess_ai_tests_sanitized`: passed.
- ThreadSanitizer full-suite audit of the same guard implementation passed
  on the same parallel guard code. The final profile reduces the worker cap
  from eight to four without changing scheduler or merge code.

## Simulator, deadline, and memory

- `xcodebuild -quiet -project 'ice five chess.xcodeproj' -scheme
  'ice five chess' -configuration Debug -sdk iphonesimulator -destination
  'generic/platform=iOS Simulator' -derivedDataPath
  /private/tmp/ice-five-chess-opponent-guard CODE_SIGNING_ALLOWED=NO build`:
  passed. The only output was the pre-existing iOS 11 deployment-target
  warning.
- Guard override smoke: 2 games, 10 candidate decisions, no benchmark
  anomalies, maximum candidate decision 1578.919 ms, no decision at or above
  5000 ms, and peak resident size 11,894,784 bytes.
- After freezing four workers, the strict suite, ASan+UBSan suite, benchmark
  build, and iOS Simulator build were rerun and passed.
