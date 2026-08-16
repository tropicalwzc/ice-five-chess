## 1. Freeze controls and hand off unfinished proof work

- [x] 1.1 Create a checksummed baseline manifest for production five-star v5.1, frozen four-star, frozen legacy three-star, one/two-star golden fixtures, corpus, rule semantics, benchmark tools, and current reports.
- [x] 1.2 Record that this change owns unfinished proof-efficiency/initiative tasks 3.1–3.5 and correctness tasks 5.1–5.2 from `improve-five-star-black-forcing`, while preserving its completed loss-aware evidence and keeping v5.2.1 unpromoted.
- [x] 1.3 Add an independently identifiable frozen four-star benchmark control and verify its profile, golden decisions, persistence, UI mapping, and player-level availability before five-star engine edits.
- [x] 1.4 Capture the same-seed production v5.1 fixed-position baseline for no-forbidden and forbidden modes, including p50/p95/max/mean, classifications, deterministic nodes, TT hits, allocations, board copies, and current full-scan counters.
- [x] 1.5 Build and checksum diagnostic sets covering prior loss structures and eight symmetries, known VCF/VCT wins, known scoped disproofs in both rules, transpositions, remote counters, forbidden dependencies, dependency combinations, quiet initiative, corpus conflicts, and natural non-final positions.

## 2. Add reference oracles, telemetry, and performance harnesses

- [x] 2.1 Add five-star diagnostic counters for board-hash cell visits, line/threat/refutation scans, legality calls and board copies, allocations/bytes cleared, proof-session hits, relevance sizes/fallbacks, dependency combinations, quiet activity, and global budget termination.
- [x] 2.2 Preserve full-scan hash, legality, threat, creator, refutation, and proof-reply generators as independent test/reference oracles that cannot consume incremental cache assumptions.
- [x] 2.3 Add table-driven and randomized make/unmake harnesses that compare board bytes, hashes, legal moves, threats, creators, reply sets, and terminal results after every step under both rule modes.
- [x] 2.4 Add a deterministic primitive benchmark that applies identical hash/line/threat/legality batches to the full-scan reference and incremental candidate and verifies semantic equality before reporting throughput.
- [x] 2.5 Extend fixed-position benchmark output with per-position proof status/scope, completed classifications per second, p50/p95/max/mean latency, node counts, session reuse, relevance/dependency/quiet activity, fallbacks, and budget exhaustion.

## 3. Implement reversible incremental position state

- [x] 3.1 Define the five-star-only incremental proof position, deterministic Zobrist tables/key, stone count, occupied and frontier bitsets, line identifiers/codes, per-side tactical masks, rule facts, dirty markers, and bounded delta stack.
- [x] 3.2 Implement initialization from an arbitrary 15x15 board and verify the initial incremental hash, line facts, masks, stone count, and legal set against the reference.
- [x] 3.3 Implement make/unmake updates that touch only the four crossing lines and restore every cached field and board byte exactly.
- [x] 3.4 Implement incremental immediate-win, four/open-four, open-three/creator, forcing-threat, and category-qualified counter masks with reference-equivalence checks.
- [x] 3.5 Implement reversible forbidden-black legality facts and dirty propagation, using the complete reference fallback whenever cached legality completeness is not established.
- [x] 3.6 Route five-star threat and refutation generation through incremental line/mask data while retaining all-legal/reference fallback on overflow or uncertainty.
- [x] 3.7 Add no-forbidden/forbidden, board-edge, dense-board, long-line, eight-symmetry, and randomized long-sequence tests proving incremental/reference equality and exact restoration.

## 4. Add one bounded reusable proof session

- [x] 4.1 Define a five-star proof session with the incremental position, bounded graph/certificate arena, generation-stamped TT, deterministic global node counters, stage quotas, and one wall-clock safety deadline.
- [x] 4.2 Define collision-safe TT validation and exact keys containing board, attacker, side-to-move, rule, search class, remaining/completed scope, algorithm version, and relevance completeness mode.
- [x] 4.3 Store/reuse compatible proof/disproof progress, solved status, controlling edge, completed scope, and verified relevance metadata, and reject every narrower/incompatible entry for broader queries.
- [x] 4.4 Route own-win, opponent-after-frozen-baseline, certificate escape, ordinary escape, and corpus-candidate proof queries through the same session without changing the frozen four-star result.
- [x] 4.5 Remove proof-mode allocation/clearing of unused alpha-beta tables and eliminate repeated per-candidate proof-table allocation, baseline proof work, and completed-score searches where a shared completed result exists.
- [x] 4.6 Enforce a global session node cap, memory cap, certificate cap, and wall ceiling so nested stages cannot acquire fresh independent time, and return unknown honestly on any exhausted unfinished query.
- [x] 4.7 Add tests for generation reuse, move-order transpositions, collisions, scope/rule/profile isolation, arena exhaustion, deadline exhaustion, allocation counts, and identical frozen-baseline decisions.

## 5. Replace depth-first proof accounting with thresholded DFPN

- [x] 5.1 Implement explicit attacker OR nodes, defender AND nodes, frontier/terminal states, saturated proof/disproof arithmetic, parent/child edges, and deterministic tie ordering.
- [x] 5.2 Implement most-proving-child selection, second-best/sibling threshold calculation, recursive thresholded expansion, and incremental ancestor updates until proof, scoped disproof, or global exhaustion.
- [x] 5.3 Add a depth/branching-aware frontier initialization behind frozen structural parameters and ensure zero/infinity are assigned only to completed terminal proof states.
- [x] 5.4 Integrate DFPN progress and solved-node reuse with the session TT without allowing unknown estimates or partial reply enumeration to become scoped disproofs.
- [x] 5.5 Implement completed-scope accounting that proves all required attacker choices and defender replies were enumerated before returning `no-forced-win-in-scope`.
- [x] 5.6 Reconstruct deterministic winning/defensive certificates from the shared proof graph and replay them through the independent reference reply generator while restoring the original board.
- [x] 5.7 Add table-driven OR/AND threshold, transposition, overflow, unknown, VCF/VCT, completed-disproof, certificate corruption, symmetry, and board-integrity tests under both rules.

## 6. Implement sound relevance-zone and dependency threat search

- [x] 6.1 Extend threat operators with exact gain/cost/rest, five-window, line, rule-legality, and certificate-zone dependencies plus bitset telemetry.
- [x] 6.2 Generate initial defender relevance zones containing all cost/dependency points, immediate defender wins, and global counter-threats fast enough for the current obligation category.
- [x] 6.3 Implement the conservative outside-zone irrelevance checker and account for every legal defender move as expanded, independently covered, all-legal fallback, or unresolved unknown.
- [x] 6.4 Include forbidden-black legality dependencies in zones and fall back conservatively whenever a remote move can alter a planned move's legality.
- [x] 6.5 Implement iterated related-zone reduction from independently verified continuations and retain per-omission certificate coverage for replay.
- [x] 6.6 Implement dependency DAG construction and combination for compatible threat operators so long VCT candidates can be proposed without enumerating unrelated move orders.
- [x] 6.7 Submit every relevance/dependency candidate to exact DFPN and independent certificate replay, rejecting or returning unknown on any uncovered counterplay.
- [x] 6.8 Add remote counter-win, remote forcing counter, crossing dependency, incompatible cost/rest, iterated-zone, forbidden dependency, all-legal fallback, symmetry, and adversarial omission tests.

## 7. Add the staged forcing and quiet-initiative portfolio

- [x] 7.1 Implement the single-session stage order: immediate tactics/mandatory defense, VCF, VCT plus dependency combinations, then eligible quiet/implicit-threat roots.
- [x] 7.2 Implement a deterministic bounded quiet-root generator requiring multiple independent future forcing dependencies or another predeclared structural implicit-threat criterion.
- [x] 7.3 Counter-prove each quiet root through the same rule-aware DFPN/relevance engine and allow an override only for a verified own win or equal-or-broader completed defense.
- [x] 7.4 Integrate completed proof-class ordering, unknown-over-known-loss behavior, longest verified survival, honest termination labels, and exact corpus forced-defense protection with the new session results.
- [x] 7.5 Preserve evaluation determinism and restrict user-game randomness to candidates equal in completed proof class, scope, distance, tactical obligation, and corpus support.
- [x] 7.6 Add integration tests proving quiet search is skipped for immediate obligations, active on eligible natural positions, unable to override on heuristic promise alone, bounded by the global session, and honestly represented in telemetry.
- [x] 7.7 Implement deterministic root-split DFPN with up to eight isolated worker sessions, one absolute deadline, pre-partitioned aggregate node/memory budgets, thread-local diagnostics, and frozen-order proof merging.
- [x] 7.8 Route candidate five-star own-win and opponent-after-candidate proof stages through the parallel root dispatcher while preserving a one-worker semantic fallback and immutable one-through-four-star paths.
- [x] 7.9 Add concurrency tests for worker isolation, root-order determinism, complete versus partial aggregate disproof, board restoration, deadline unwind, diagnostics reduction, repeated evaluation, and TSAN/sanitizer behavior.

## 8. Pass correctness, activity, speed, and freeze gates

- [x] 8.1 Run all existing and new C tests, frozen legacy three-star golden, frozen four-star golden/control checks, one/two-star regressions, deterministic regeneration, board-integrity, and complete certificate/relevance replay.
- [x] 8.2 Run ASan/UBSan and randomized long make/unmake/proof sessions under both rule modes with zero sanitizer findings or reference mismatches; run TSAN or an equivalent race audit for the parallel dispatcher.
- [x] 8.3 Build the iOS Simulator target and measure candidate per-worker/aggregate memory high-water marks and global decision ceiling behavior.
- [x] 8.4 Run the primitive benchmark and require identical results plus at least 2.0x aggregate incremental throughput over the full-scan reference.
- [x] 8.5 Run the fixed-position production-v5.1 versus candidate comparison, publish per-rule p50/p95/max/mean, CPU time, workers/jobs, peak memory, and all work counters, and require every candidate decision to return its best completed legal move within the 5,000 ms player-visible hard ceiling.
- [x] 8.6 Verify held-out diagnostics demonstrate most-proving expansion, compatible session reuse, verified relevance omissions, dependency combinations, eligible quiet roots, root-parallel activity, and at least one replayable scoped disproof in each rule mode.
- [x] 8.7 Run non-final structural ablations for TT/arena size, frontier initialization, relevance fallback, dependency limits, quiet-root limit, worker count, root budget partitioning, global node allocation, memory, and wall ceiling; document rejected variants without using final seeds.
- [x] 8.8 Freeze and checksum the passing candidate source/profile, structural parameters including worker count, corpus, tools, controls, diagnostics, performance results, and predeclared promotion criteria before final schedule generation.

## 9. Run fresh four-cell strength evaluation and publish reports

- [x] 9.1 Generate fresh untouched no-forbidden and forbidden formal master seeds after the freeze and audit opening/seed/position separation from diagnostics, corpus-shaped starts, prior failures, non-final runs, and previous formal suites.
- [x] 9.2 Run 100 no-forbidden games against frozen four-star from 50 color-exchanged natural openings, with 50 new-five-star-black and 50 new-five-star-white games.
- [x] 9.3 Run 100 no-forbidden games against frozen legacy three-star from 50 color-exchanged natural openings, with 50 new-five-star-black and 50 new-five-star-white games.
- [x] 9.4 Run 100 forbidden games against frozen four-star from 50 color-exchanged natural openings, with 50 new-five-star-black and 50 new-five-star-white games.
- [x] 9.5 Run 100 forbidden games against frozen legacy three-star from 50 color-exchanged natural openings, with 50 new-five-star-black and 50 new-five-star-white games.
- [ ] 9.6 Merge shards and independently replay all 400 games for schedule/color identity, every move, rule legality, terminal result, decision provenance, accepted certificate, relevance coverage, and board integrity; invalidate any anomalous cell. **Executed and failed:** all eight datasets are internally legal, but exact original/regenerated replay diverged in every cell (11 move-divergent games and 26 candidate metadata-divergent steps before the first move divergence), so all four cells are invalid for promotion.
- [x] 9.7 Generate color-correct Markdown/JSON/raw reports that lead with new-five-star white, compute white and black deltas from each model's same-color 50 games, retain natural first-player advantage, and include Wilson/paired uncertainty, activity, latency, commands, provenance, limitations, and checksums.
- [x] 9.8 Apply the predeclared per-rule promotion gates: positive white direction plus non-negative black/overall against four-star, and non-negative white/overall against legacy three-star; classify overlapping uncertainty honestly without pooling rules or colors.
- [x] 9.9 Promote only the five-star player entry on a complete pass while retaining frozen four-star and v5.1 rollback; otherwise keep production v5.1 and all one-through-four-star behavior unchanged.
- [x] 9.10 Synchronize the complete checksummed report bundle to `/Users/wangzicheng/Downloads/logs_five_chess` and verify byte-identical workspace/Downloads manifests.
