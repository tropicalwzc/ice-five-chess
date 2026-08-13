## 1. Baseline and engine boundaries

- [x] 1.1 Record the frozen legacy-three-star source hash, current production profile, rule behavior, previous final-v2 report checksum, and a clean reproducibility command in a new change-local baseline manifest.
- [x] 1.2 Add a disabled proof-search profile and public result structures for three-valued proof status, searched class, distance, budgets, certificate identifier, and decision override reason without changing production behavior.
- [x] 1.3 Extract or expose one rule-aware legality and make/unmake boundary usable by the existing C engine, threat solver, proof verifier, opening validator, and benchmark; add board-integrity assertions around it.
- [x] 1.4 Add deterministic Zobrist keys that include side to move, forbidden-move mode, board size, search class, and profile version, with symmetry transform helpers and round-trip tests.

## 2. Threat representation and enumeration

- [x] 2.1 Define pure-C threat, defense-set, severity, dependency, search-result, proof-node, and statistics data structures with bounded storage and explicit overflow handling.
- [x] 2.2 Implement line-pattern extraction for immediate five, open four, simple/broken four, four-three, and eligible open/broken-three VCT threats across all four directions.
- [x] 2.3 Enumerate and deduplicate every legal cost/defense point and dependency point for each threat instead of retaining a first match.
- [x] 2.4 Include defender immediate wins and equal-or-higher-severity counter-threats in defensive reply generation, and filter all generated moves through the active rule implementation.
- [x] 2.5 Add table-driven tests for straight, broken, crossing, edge, multi-defense, duplicate-defense, counter-threat, and forbidden-black patterns under all eight symmetries.

## 3. Strict VCF and proof verification

- [x] 3.1 Implement the strict continuous-four AND/OR VCF solver with deterministic move ordering, node/depth/time budgets, make/unmake safety, and a transposition table.
- [x] 3.2 Emit a compact proof DAG for every VCF `proven-win`, including board hashes, attacker choice, every defender child, terminal reason, and winning distance.
- [x] 3.3 Implement a separate certificate verifier that regenerates defender replies, replays every branch from the original board, validates terminal wins, and rejects illegal, missing, or corrupted branches.
- [x] 3.4 Add known VCF wins, VCF refutations, false positives, multiple-defense lines, defender counter-wins, edge positions, and deliberate certificate-corruption tests.
- [x] 3.5 Verify fixed-input determinism and that budget exhaustion returns `unknown` without leaking a heuristic result or mutating the board.

## 4. DFPN VCT solver

- [x] 4.1 Implement proof/disproof-number initialization and update rules for attacker OR nodes and defender AND nodes, with saturated arithmetic and terminal-state handling.
- [ ] 4.2 Implement depth-first proof-number thresholds, most-proving-node selection, deterministic severity/distance ordering, and rule/search-class-aware transposition reuse.
- [x] 4.3 Stage strict VCF before broader VCT expansion and return `proven-win`, `no-forced-win-in-scope`, or `unknown` with accurate completed-stage and budget statistics.
- [x] 4.4 Extend proof DAG generation and independent verification to open-three, broken-three, four-three, multi-cost, and counter-threat VCT branches.
- [x] 4.5 Build a labeled tactical fixture set covering VCT wins, no-solution-in-scope positions, fake threes, alternative defenses,反杀,禁手,边界, and all eight symmetries; record expected class, budget, and distance where known.
- [x] 4.6 Compare VCF-only, attack VCF+VCT, and attack-plus-defensive-counter-proof modes on non-final tactical/latency fixtures and freeze a correctness-compliant mobile profile without learning weights from game outcomes.

## 5. Curated opening library

- [x] 5.1 Audit the official Gomocup results/opening packages and RIF/RenjuNet pages, recording exact URLs, retrieval dates, checksums, rule families, package formats, attribution, and redistribution status; exclude any source whose bundling rights remain unclear.
- [x] 5.2 Download only approved official source packages, retain immutable source checksums, and create a provenance manifest that identifies every selected event/year and opening index.
- [x] 5.3 Implement an offline importer for the approved formats that normalizes coordinates, validates alternating colors and legality, rejects terminal/incompatible prefixes, and partitions freestyle 15×15, standard, Renju, Caro, and other rules.
- [x] 5.4 Canonicalize rotations/reflections, merge exact duplicates while preserving provenance, report near-duplicate prefixes, and prevent one repeated family from dominating the library.
- [x] 5.5 Select bounded common opening prefixes and alternative continuations using published expert opening sets and cross-source frequency only; document the selection policy and avoid outcome-trained weights.
- [x] 5.6 Generate a compact versioned offline runtime asset plus manifest/checksum and add lookup tests for rule compatibility, transforms, deterministic evaluation seeds, varied user seeds, and no-network operation.

## 6. Proof-guided three-star integration

- [x] 6.1 Add rule-compatible opening lookup as a default advisor before the frozen legacy advisor, with explicit default-source and book-entry telemetry.
- [x] 6.2 Preserve complete immediate-win and immediate-defense gates, then run verified own VCF/VCT candidates before accepting a non-winning default suggestion.
- [x] 6.3 After a candidate default, run opponent counter-proof search; reject a proven-losing default only when a legal alternative removes the proof in the same completed scope or has a faster verified own win.
- [ ] 6.4 Rank verified wins by shortest distance, proven losses by longest survival, scoped-disproved candidates above unknown only when scopes are comparable, and retain the safe legacy default for unresolved cases.
- [ ] 6.5 Restrict user randomness to candidates with equal completed proof class, tactical obligation, and winning distance; verify unique wins/defenses remain seed-independent and benchmark mode remains deterministic.
- [x] 6.6 Extend analysis telemetry with default/selected moves, override reason, proof class/distance, search class, certificate ID, proof/disproof numbers, nodes, TT hits, budget state, book ID, and random-selection metadata.
- [x] 6.7 Keep the new profile behind a switch, add rollback coverage to legacy-three-star, and verify one-star/two-star behavior and both iPhone/iPad controllers remain unchanged.

## 7. Correctness and performance gates

- [x] 7.1 Run the existing FiveChessAI tests and the expanded threat/proof/opening suite under sanitizers where supported, fixing every illegal move, board mutation, proof-verifier failure, nondeterministic evaluation result, and symmetry mismatch.
- [x] 7.2 Add golden integration positions where proof attack overrides legacy, proof defense rejects legacy, book is retained, book is rejected, search is unknown, and all moves are proven losing.
- [ ] 7.3 Measure VCF/DFPN p50, p95, maximum latency, nodes, TT reuse, proof coverage, and budget exhaustion on representative early/middle/late positions and the oldest available supported iPhone/iPad target.
- [x] 7.4 Freeze the release node/time/depth limits only after tactical correctness passes; document any position excluded by the mobile budget as `unknown` rather than weakening certificate verification.
- [x] 7.5 Run a fixed book-on/book-off diagnostic schedule against legacy and report move-source attribution, new-white score, overall score, tactical coverage, and latency before selecting the final profile and library version.

## 8. Held-out match and report

- [x] 8.1 Generate and freeze a new held-out final seed domain and 100 canonical rule-compatible opening identities only after solver/profile/library selection; verify the final IDs and decision seeds were not used by diagnostics.
- [x] 8.2 Run a same-suite frozen legacy control to establish the natural black/white baseline for the exact held-out opening set and record its raw games and checksums.
- [x] 8.3 Run the opening-library-enabled profile for exactly 100 new-black and 100 new-white games against frozen legacy, exchanging engine colors on each held-out opening and recording every anomaly without omission.
- [x] 8.4 Run the otherwise identical opening-library-disabled profile for exactly 100 new-black and 100 new-white games against frozen legacy on the same held-out openings and paired seed derivation.
- [x] 8.5 Validate both raw schedules, replay every recorded move for legality and terminal result, verify all accepted proof certificates, and invalidate an affected formal run if required games, seeds, or proof data are inconsistent.
- [x] 8.6 Extend report generation to lead with new-white W/D/L, score, Wilson interval, and paired change from the direct-match legacy-white games (the new-black half), followed by the supplemental legacy self-play color baseline, new-black and overall results, proof/book/legacy move attribution, tactical results, performance, anomalies, environment, versions, commands, and checksums.
- [x] 8.7 Generate separate standalone Markdown/machine-readable reports for the 200-game book-on and 200-game book-off matches plus a third Markdown comparison report containing book-on minus book-off overall and by color, coverage, move source, latency, proof activity, and uncertainty.
- [x] 8.8 Classify each formal result as `demonstrated stronger`, `directional improvement`, or `not demonstrated`, explicitly attributing gains present only in book-on to the opening library and gains present in book-off to proof-guided search.
- [x] 8.9 Reproduce all three reports from documented commands under `reports/five_chess`, then copy and checksum the complete bundle into `/Users/wangzicheng/Downloads/logs_five_chess` after obtaining filesystem authorization.
- [x] 8.10 Keep the frozen legacy advisor at the application's three-star entry and expose only the demonstrated opening-book-disabled proof-guided profile as four stars; retain the book-enabled profile for diagnostics only.
- [x] 8.11 Add four-star selection to both iPhone and iPad difficulty controls, route each of the four levels explicitly, and preserve existing saved difficulty values 0/1/2 as three/two/one stars while storing four stars as value 3.
