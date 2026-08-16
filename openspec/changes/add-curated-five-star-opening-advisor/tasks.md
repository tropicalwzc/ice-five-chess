## 1. Freeze baseline and diagnose the failed book

- [x] 1.1 Record hashes and profile snapshots for current four-star, frozen legacy three-star, the v2 opening asset, and the corrected formal reports.
- [x] 1.2 Correlate all prior book-on/off games by opening ID/color and identify the first changed new-engine decision, book ID/ply, proof state, and result.
- [x] 1.3 Publish a root-cause Markdown/JSON report separating tournament-assignment content, uniform selection, insufficient support, and proof-gate limitations.

## 2. Research and admit elite records

- [x] 2.1 Audit official federation and major-tournament sources, recording exact URLs, event level, rules, formats, retrieval dates, checksums/stable IDs, and redistribution status.
- [x] 2.2 Freeze an allowlist for compatible elite Freestyle-15 human titles/events and top engine divisions plus explicit exclusion reasons.
- [x] 2.3 Acquire only approved records and keep research-only raw material outside the shipped asset when redistribution rights are unclear.
- [x] 2.4 Implement strict parsing/replay validation, identity checks, rule partitioning, eight-symmetry canonicalization, and cross-source duplicate detection.

## 3. Build the position corpus

- [x] 3.1 Aggregate bounded opening positions and continuations with independent game/event/source support and side-specific metadata instead of storing scripts.
- [x] 3.2 Exclude assignment-only lines, low-support candidates, incompatible rules, corrupted records, and concentrated single-event evidence.
- [x] 3.3 Emit a compact versioned offline corpus, manifest, exclusion audit, and checksums with deterministic regeneration tests.

## 4. Implement conservative five-star fusion

- [x] 4.1 Add a corpus-blind four-star baseline decision API and a separate five-star profile/candidate telemetry surface.
- [x] 4.2 Implement current-position corpus lookup that immediately returns no advice after an unsupported opponent deviation.
- [x] 4.3 Compare corpus candidates against the exact four-star move using legality, immediate obligations, proof class/distance, completed scope, and frozen conservative margins.
- [x] 4.4 Fail closed to four-star on unknown/incomparable evidence, exhaustion, certificate failure, insufficient support, or unique tactical obligations.
- [x] 4.5 Restrict user randomness to proof-equivalent, equally supported accepted candidates and keep evaluation mode deterministic.

## 5. Tests and diagnostic selection

- [x] 5.1 Add corpus provenance, duplicate, rule, symmetry, support-threshold, off-book fallback, and no-network tests.
- [x] 5.2 Add tactical integration fixtures for accepted superior advice, rejected unsafe advice, unique win/defense, unknown proof, and unchanged four-star decisions.
- [x] 5.3 Run C tests and sanitizers plus iPhone/iPad simulator builds; verify one-to-four-star behavior remains unchanged.
- [x] 5.4 Compare conservative fusion variants on a non-final diagnostic domain, freeze thresholds/profile/corpus without using final outcomes, and document selection.

## 6. Add the five-star player level

- [x] 6.1 Add five-star to both iPhone and iPad difficulty controls in descending order.
- [x] 6.2 Route persisted value 4 to five-star while preserving 0=three, 1=two, 2=one, and 3=four across save/load and both controllers.
- [x] 6.3 Keep the five-star entry disabled unless the final release gate passes; rollback SHALL leave four-star and lower levels unchanged.

## 7. Held-out dual-baseline evaluation

- [x] 7.1 Generate a new held-out schedule/seed domain after freezing corpus and profile, and prove it was unused by source filtering and diagnostics.
- [x] 7.2 Run five-star versus book-blind four-star for exactly 100 five-star-black and 100 five-star-white games.
- [x] 7.3 Run five-star versus frozen legacy three-star for exactly 100 five-star-black and 100 five-star-white games.
- [x] 7.4 Replay every move, validate schedules/results, verify accepted proof certificates, and invalidate any affected formal schedule on anomaly.
- [x] 7.5 Generate standalone Markdown/raw/summary reports for both opponents plus a comparison report with white-first statistics, Wilson intervals, paired uncertainty, corpus gates, proof activity, latency, provenance, commands, and checksums.
- [x] 7.6 Apply the anti-overfitting release classification, enable five-star only on pass, and copy/checksum the complete bundle under `/Users/wangzicheng/Downloads/logs_five_chess`.

## 8. Expand coverage without corpus-shaped schedules

- [x] 8.1 Extend the source audit/importer to official top-division 20x20 Freestyle early openings and compatible 15x15 Renju records; partition rules and record source-board-size/embedding exclusions.
- [x] 8.2 Add tiered trust for cross-event and high-repeat single-event continuations, candidate consensus, signed relative coordinates, and deterministic asset regeneration.
- [x] 8.3 Build candidate-centered local opening signatures and runtime exact-first/local-second lookup with rule, side, boundary, support, and consensus guards.
- [x] 8.4 Relax the five-star acceptance window only for sufficiently trusted advice while retaining legality, immediate win/defense, immediate-loss, verified forced-loss, and comparable-search safety gates; extend telemetry and tests.
- [x] 8.5 Run A/B coverage and acceptance diagnostics exclusively on independently seeded natural schedules for forbidden and no-forbidden modes, then freeze the corpus/profile before formal seeds are generated.
- [x] 8.6 Under each rule mode, run five-star versus four-star and frozen legacy three-star for 100 games per opponent, split 50 five-star-black and 50 five-star-white; replay every move and validate corpus evidence.
- [x] 8.7 Generate rule-separated, color-correct Markdown/JSON reports and a comparison report, then copy and checksum the bundle under `/Users/wangzicheng/Downloads/logs_five_chess`.
