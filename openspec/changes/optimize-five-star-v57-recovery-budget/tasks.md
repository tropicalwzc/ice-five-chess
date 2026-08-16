## 1. Contracts and instrumentation

- [x] 1.1 Define the four-state five-star decision certainty enum and compatibility projection for `FCAnalysisResult`, including unknown/deadline and no-legal-move reasons.
- [x] 1.2 Add a per-decision resource-ledger type for monotonic deadline, nodes, temporary bytes, sessions, and corpus/book queries, with saturating reservation/consumption helpers.
- [x] 1.3 Add telemetry fields for stage budgets, fallback state, root-job coverage, actual concurrency, no-progress exits, cache-oracle mismatches, and ledger exhaustion.
- [x] 1.4 Extend the research profile manifest with independent opening-book/corpus switches, worker cap, hard/internal deadlines, and ledger schema version without changing frozen profiles.

## 2. Sound fallback and recovery orchestration

- [x] 2.1 Implement deterministic full-board legal fallback enumeration using the rule-aware oracle and canonical coordinate ordering.
- [x] 2.2 Change empty/truncated candidate handling in `fc_analyze_internal` so incomplete coverage returns unknown instead of `provenLoss`.
- [x] 2.3 Add explicit terminal no-legal-move detection and ensure the Objective-C caller ends a game only for that state or a verified terminal loss.
- [x] 2.4 Refactor `fc_analyze_five_star_profile_with_hint_internal` to construct a legal baseline and continue when four-star analysis returns false or incomplete.
- [x] 2.5 Split escape candidate construction into certificate, tactical, nearby, and full-board layers with coordinate deduplication and fixed ordering.
- [x] 2.6 Add cheap legality/immediate-loss prefilters and reserve proof budget only for candidates that survive the prefilters.
- [x] 2.7 Require scoped-disproof replay before a recovery override; rank completed unknown/survival candidates deterministically when no disproof completes.
- [x] 2.8 Update `doublethree.m` to place a legal fallback for unknown analysis and to preserve terminal/no-legal-move handling.

## 3. Root and escape parallel proof

- [x] 3.1 Extract one root-threat enumeration pass that returns canonical gain records, dependency metadata, and an overflow flag.
- [x] 3.2 Add single-gain root-job structures carrying immutable position input, gain/dependency data, child thresholds, and ledger reservations.
- [x] 3.3 Change workers to enter proof below their assigned gain rather than calling the all-root `fc_prove_forced_win` path again.
- [x] 3.4 Keep mutable positions, proof graphs, allocators, and default transposition tables private to each worker; version any optional read-only metadata.
- [x] 3.5 Implement canonical job-index result storage, fixed-order merge, certificate replay, and atomic verified-result early stop.
- [x] 3.6 Apply the same completed-result/early-stop contract to parallel escape candidates and retain deterministic unknown/survival ranking.
- [x] 3.7 Add diagnostics that distinguish configured worker cap, launched workers, maximum concurrent workers, duplicate root enumeration, and completed jobs.

## 4. DFPN progress and certificate semantics

- [x] 4.1 Add a postponed-edge queue with proof/disproof thresholds, revisit epoch, and deterministic insertion index.
- [x] 4.2 Replace direct no-progress return with thresholded sibling selection and periodic dovetail reinsertion.
- [x] 4.3 Use saturated proof/disproof arithmetic and commit counters only for completed child states.
- [x] 4.4 Ensure deadline/node exhaustion returns unknown unless a certificate has passed independent replay verification.
- [x] 4.5 Add fixed DFPN fixtures for no-progress, postponed sibling completion, threshold ties, and certificate replay failure.

## 5. Incremental forbidden legality

- [x] 5.1 Complete reversible line/window revision updates in `FCIncrementalPosition` for overline, double-four, and the established double-three rule.
- [x] 5.2 Route threat generation, refutation generation, and immediate-win scans through incremental legality masks before falling back to the oracle.
- [x] 5.3 Implement seeded random and adversarial cache/oracle cross-checks, including make/unmake and board-key restoration.
- [x] 5.4 Disable cache-based pruning for the current decision on any mismatch and emit fixture/move diagnostics.
- [x] 5.5 Measure legality calls, full-board copies, cache hits, mismatch count, and p50/p95 latency for forbidden and no-forbidden fixtures.

## 6. Unified budget and profile attribution

- [x] 6.1 Thread the decision ledger through proof, escape, quiet, corpus, opening, and fallback stages without creating local emergency budgets.
- [x] 6.2 Enforce the 4,500 ms internal target and 5,000 ms hard ceiling with monotonic checks at worker/node boundaries.
- [x] 6.3 Implement fixed stage reservation and handoff rules and record requested/reserved/consumed/abandoned resources.
- [x] 6.4 Ensure corpus/book candidates pass mandatory-defense, legality, proof, and ledger checks before overriding a searched move.
- [x] 6.5 Persist profile, corpus version, rule mode, schedule seed, checksums, and ledger schema in every diagnostic bundle.

## 7. Correctness and performance gates

- [x] 7.1 Add automated fixtures for legal fallback, four-star failure handoff, layered escape ordering, root-job equivalence, and early-stop safety.
- [x] 7.2 Add one-/four-/eight-worker determinism and utilization benchmarks with duplicate-root and certificate-replay assertions.
- [x] 7.3 Add ledger deadline, budget exhaustion, memory reservation, and stage-audit tests under both rule modes.
- [x] 7.4 Add profile A/B fixtures that toggle opening book and elite corpus independently on identical seeds and schedules.
- [x] 7.5 Build the target app/test harness with warnings enabled and run the relevant unit, fixture, and stress tests before match play.

## 8. Strength evaluation and handoff

- [x] 8.1 Freeze the new candidate manifest only after correctness/performance gates pass; keep 5.7, four-star, and three-star identities immutable.
- [x] 8.2 Run separate forbidden and no-forbidden 100-game cells against frozen four-star with equal CPU/thread limits and fixed color/schedule provenance.
- [x] 8.3 Run the corresponding legacy three-star cells as a non-gating generalization check without changing its implementation.
- [x] 8.4 Generate Markdown/JSON/raw logs with black/white WDL, white score rate, paired deltas, Wilson intervals, unknown/no-progress counts, latency, and worker telemetry.
- [x] 8.5 Copy the final report bundle and checksums to `/Users/wangzicheng/Downloads/logs_five_chess` and verify the paths are readable.
- [x] 8.6 Confirm production/UI routing still selects the existing profile and document that promotion requires a separate user-approved change.
