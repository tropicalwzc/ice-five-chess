## 1. Freeze the baseline and isolate the candidate profile

- [x] 1.1 Capture the current 5.8.1 and frozen four-star profile snapshots, source hashes, existing C-test result, and the unchanged iPhone/iPad five-star binding under the change evidence directory.
- [x] 1.2 Add a named 5.8.2 `FCAIProfile` factory in `ice five chess/FiveChessAI.c` and its declaration in `ice five chess/FiveChessAI.h`, inheriting the exact 5.8.1 configuration while keeping 5.8.1 and all lower-difficulty factories unchanged.
- [x] 1.3 Add explicit 5.8.2 and exact-5.8.1 selector identities to `tools/five_chess_benchmark.m` and `tools/five_chess_profile_snapshot.c`, including serialized version validation that cannot resolve the direct comparison to 5.4.1.

## 2. Implement the direction-based double-three scan

- [x] 2.1 Define internal scan-result and gain-record structures in `FiveChessAI.c` for direction masks, coordinates, completion status, bounded capacity, and scan timing without reusing continuation-point counts as direction counts.
- [x] 2.2 Implement the pure direction predicate for the four board directions, four-cell three-stone-plus-empty windows, in-board empty exteriors, immediate-five exclusion, and active rule-aware legality.
- [x] 2.3 Enumerate legal white replies from the bounded local frontier, deduplicate recorded gains deterministically, and return incomplete/unknown status on deadline, storage, or legality-frontier exhaustion.
- [x] 2.4 Add deterministic targeted black blocker generation from recorded gains, retaining existing ordinary, dependency, and certificate candidates and filtering every injected point through black legality.

## 3. Integrate bounded black preemption and telemetry

- [x] 3.1 Add the black-only structural stage to the isolated 5.8.2 decision path before opponent-guard skip paths, with immediate-win and independently verified own-VCF bypasses and no bypass for unresolved opponent VCF or unverified own VCT.
- [x] 3.2 Simulate and restore each provisional and targeted black candidate, re-scan all retained white gains, and order candidates by zero residual gains, existing proof class/scope, distance, and deterministic heuristic tie-breakers.
- [x] 3.3 Enforce profile caps and the existing absolute decision deadline without creating a second time window; mark no-defense and incomplete scans unresolved rather than safe.
- [x] 3.4 Extend the decision ledger and benchmark serialization with scan completion, white gain count, provisional/selected residual counts, candidates examined/eliminated, bypasses, structural overrides, restoration failures, and deadline anomalies.
- [x] 3.5 Add board-hash and move-count assertions around every temporary simulation so a restoration mismatch invalidates the structural override and is reported as unresolved.

## 4. Add focused correctness and regression coverage

- [x] 4.1 Add C fixtures in `tools/five_chess_ai_tests.c` for a crossing double-three, all eight symmetries, a single-open-three negative control, edge and blocked lines, multiple white gains, and both active rule modes.
- [x] 4.2 Add integration fixtures covering no own VCF, verified own immediate win/VCF bypass, unresolved opponent VCF with a complete scan, targeted blocker selection, and the no-single-defense least-residual unresolved path.
- [x] 4.3 Add repeated-evaluation determinism, board restoration, scan overflow/deadline, telemetry integrity, and selected-move legality assertions; run sanitizer or equivalent memory checks where the supported toolchain permits.

## 5. Build the two paired small-match cells

- [x] 5.1 Extend the benchmark schedule and runner to freeze 12 natural-freestyle opening identities, both candidate color assignments, identical per-game seeds, no-forbidden mode, deterministic-best play, and a 120-move cap.
- [x] 5.2 Run 24 games of 5.8.2 versus frozen four-star and 24 games of 5.8.2 versus the exact 5.8.1 profile, preserving the same opening/color/seed schedule across cells and recording the exact profile headers.
- [x] 5.3 Update `tools/replay_early_vcf_quick_match.py`, `tools/summarize_five_chess_paired.py`, or the corresponding benchmark replay path to validate profile identities, replay every move, verify terminal results/certificates, and surface double-three telemetry by candidate color.
- [x] 5.4 Publish raw JSONL, replay/integrity results, source provenance, and Markdown summaries under `openspec/changes/add-five-star-black-double-three-defense/evidence/`, including candidate-black, candidate-white, and overall W/D/L views for both cells.

## 6. Verify performance and promotion gates

- [x] 6.1 Run `tools/run_five_chess_ai_tests.sh` plus the focused fixtures and confirm deterministic legality, symmetry, rule-mode, restoration, and fail-closed assertions.
- [x] 6.2 Measure p50, p95, and maximum decision latency for ordinary black moves and double-three positions, and list every five-second deadline, incomplete-scan, rollback, or replay anomaly.
- [x] 6.3 Review both cells as directional evidence only, verify that 5.8.1/four-star controls and the playable iPhone/iPad binding remain unchanged, and record whether a separate promotion change is warranted without performing that promotion here.
