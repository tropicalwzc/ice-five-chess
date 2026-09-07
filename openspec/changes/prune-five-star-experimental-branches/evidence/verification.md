# Production-only cleanup verification

Verified on 2026-09-07. This change supersedes historical requirements to keep the exact 5.4.1 rollback executable; it does not rewrite those historical records.

## Retained production identity

`fc_profile_five_star_early_micro_vcf_candidate()` remains the only five-star factory, with version `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`. App binding and the default five-star C route both use it. The factory is flattened onto the unchanged lower-level base instead of inheriting through retired five-star factories.

Removed 14 obsolete factories, 5 dedicated analysis routes, 28 inactive profile fields and their exclusive experimental branches. Removed 22 obsolete tool files, including old variant runners and schedules requiring retired controls. The former multi-version benchmark is now a small current app-path smoke tool. Historical specs, recorded datasets and offline report readers remain available; executing retired variants requires their Git revision.

The engine C diff removes 3,160 lines and adds 258. Shared proof, legality, ledger, corpus and lower-difficulty algorithms remain. Earlier Objective-C page/asset cleanup and unrelated worktree edits are outside this change.

## Validation results

- `python3 tools/verify_five_star_production.py`: passed. All live fields of three supported profiles exactly match `production-before.txt`; all 28 removed fields were zero in those profiles. All 12 deterministic tactical fixtures match, with input boards unchanged.
- `python3 tools/verify_five_star_ui_binding.py`: passed. Shared SwiftUI app selects only 5.8.1 for five stars; four-star binding remains proof-guided without the book.
- `sh tools/run_five_chess_ai_tests.sh`: passed, including retained shared/current proof and guard tests.
- `sh tools/run_legacy_three_star_golden.sh`: passed for one/two/three-star fixed outputs.
- Strict C syntax check with `-std=c11 -DFC_ENABLE_TEST_API=1 -Wall -Wextra -Werror -pedantic`: passed. All eight retained `tools/*.c` files also pass syntax checks with the test API enabled.
- Current benchmark build: passed. Center-opening smoke runs with forbidden moves both off and on, two games per rule with candidate colors swapped, eight plies per game: passed, zero five-star deadline violations. These are short integration checks, not completed-match strength measurements. Retired selector `five-star-v57` is rejected with exit status 2.
- Clean simulator Debug build/test: passed on iPhone SE (3rd generation), iOS 18.3.1, and iPad Pro 11-inch (M5), iOS 26.5. The result bundle reports 10 tests on each device, 20 runs, zero failures/skips.
- Generic iOS Release build with signing disabled: passed. Existing legacy Objective-C operator-parentheses and test deployment-target warnings remain; no claim of a warning-free Xcode build is made.
- `openspec validate prune-five-star-experimental-branches --strict` and `git diff --check`: passed.

## Local validation artifacts

- `/private/tmp/five-star-production-final-c-tests.log`
- `/private/tmp/five-star-production-final-legacy.log`
- `/private/tmp/five-star-production-only-tests.log`
- `/private/tmp/five-star-production-only-tests.xcresult`
- `/private/tmp/five-star-production-only-release.log`
- `/private/tmp/five-star-production-center-smoke.jsonl`
- `/private/tmp/five-star-production-center-forbidden-smoke.jsonl`

The checked-in pre-cleanup snapshot and verifier are durable regression evidence. Temporary build/log artifacts may be removed by the OS. Parameter and fixture equivalence plus regression tests do not establish exhaustive equivalence across all timed searches or a new win-rate claim.
