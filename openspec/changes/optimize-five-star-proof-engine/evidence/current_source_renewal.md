# Current-source correctness and platform renewal

Date: 2026-08-14  
Candidate: `5.4.1-transactional-deadline-root-parallel-5s`

The transactional-deadline source was rebuilt and tested before revision-7 freeze. Quiet initiative, loss-aware refinement, and quiet defense now commit atomically or roll back partial move overrides whenever the shared deadline fires.

## Strict and sanitizer suites

The complete `tools/five_chess_ai_tests.c` suite was compiled from current `FiveChessAI.c` with strict C11 warnings, ASan/UBSan, and TSAN:

```sh
clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic "ice five chess/FiveChessAI.c" tools/five_chess_ai_tests.c -lm -o /private/tmp/five_chess_ai_tests
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined "ice five chess/FiveChessAI.c" tools/five_chess_ai_tests.c -lm -o /private/tmp/five-chess-sanitizers-current/five_chess_ai_tests_asan
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=thread "ice five chess/FiveChessAI.c" tools/five_chess_ai_tests.c -lm -o /private/tmp/five-chess-sanitizers-current/five_chess_ai_tests_tsan
```

All runs printed `FiveChessAI tests passed`. ASan/UBSan reported no memory or undefined-behavior finding; TSAN reported no data race. The suite includes randomized long make/unmake in both rules, reference equivalence, worker isolation, parallel merge, transactional deadline/no-progress unwind, certificate replay, and lower-level golden checks.

## iOS Simulator

```sh
xcodebuild -quiet -project "ice five chess.xcodeproj" -scheme "ice five chess" -configuration Debug -sdk iphonesimulator -destination "generic/platform=iOS Simulator" -derivedDataPath /private/tmp/five-chess-current-simulator CODE_SIGNING_ALLOWED=NO build
```

The build exited zero. Existing duplicate asset-name, legacy Objective-C precision/retention, and deployment-target warnings remain; no new build error was introduced.

## Resource bounds and fixed-position high water

- Exact private session arena: 10,747,904 bytes per worker.
- Frozen maximum workers: 8.
- Exact concurrent private-arena cap: 85,983,232 bytes (about 82.0 MiB).
- Largest measured paired-process peak RSS: 45,989,888 bytes (about 43.9 MiB).
- Absolute internal decision deadline: 4,500ms; player-visible hard ceiling: 5,000ms.
- No-forbidden raw maximum: 4,549.994ms; raw maximum CPU time: 10,970.306ms.
- Forbidden raw maximum: 2,729.200ms; raw maximum CPU time: 4,186.000ms.

The CPU/wall difference demonstrates decision-local concurrency. Both rules returned legal moves below 5,000ms and had deterministic selected moves plus completed metadata across four repetitions; raw counters for unfinished wall-limited work varied honestly.

## Deadline determinism renewal

Five revision-6 held-out prefixes were run twice under each rule with both candidate colors and play continued through ply 16. Across 40 short games, each rule compared 47 candidate steps per repetition. Complete move lists, winners, terminations, selected moves, and completed proof/corpus/rollback metadata were identical.

Raw unfinished node counters varied on 15 no-forbidden and 13 forbidden decisions, which is allowed only because completed results remained identical and exhaustion was reported. Maximum candidate wall times were 3,891.626ms and 4,500.369ms respectively. Evidence is under `deadline_determinism_prefix/`.

## Current core hashes

- `ice five chess/FiveChessAI.c`: `9e2a1a5bf2e8652e57e2f21bdb0ee9c47e93fee7117f259516ae3460ce256759`
- `ice five chess/FiveChessAI.h`: `da0c423123a4a938fbb66262b31ba527a1f64d0d9015599969a9a2b0c63b8fc2`
- `tools/five_chess_ai_tests.c`: `4b1ef4ed2101b92b2cce4209dd63f6f37f701bfa966844cda8921f767c83dab7`
- `tools/five_chess_benchmark.m`: `adb1e030c113cf5fceda949f0927615042670ddccbadc6dfb2979dd562fd0e36`
- `tools/summarize_five_chess_paired.py`: `3dd51c07db0d360b52a8f6ac21878138dad873dee72665d1352707d3fc43e2ac`
- `tools/replay_five_chess_formal.py`: `5944ad4494e0db87394ad9737b0b95eb097f2cfd2766a45596ca2f0388dab35f`
