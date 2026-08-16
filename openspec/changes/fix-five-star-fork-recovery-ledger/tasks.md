## 1. Decision contracts and telemetry

- [x] 1.1 Extend `FCDecisionLedger`, `FCProofSession`, `FCAnalysisResult`, and `FCProofDiagnostics` with session-owned memory release state, live/peak/released counters, fork annotations, and fixture/worker attribution fields.
- [x] 1.2 Add the research-only worker-count override and validate that benchmark cells accept exactly 1, 4, or 8 workers without changing frozen profile defaults.
- [x] 1.3 Define stable fork-risk, coverage, handoff-failure, and fallback reason values and preserve `UNKNOWN_OR_DEADLINE` versus explicit no-legal-move semantics in the serialized diagnostics.

## 2. Reversible proof-session memory ledger

- [x] 2.1 Replace cumulative-only memory reservation checks with an atomic live-reservation operation that enforces the configured budget and updates a peak high-water mark.
- [x] 2.2 Implement an idempotent ledger memory-release operation with underflow protection and released-byte accounting.
- [x] 2.3 Store the owning ledger, reservation size, and held flag in each proof session; release reservations on normal, exhausted, cancelled, worker, and allocation-failure paths.
- [x] 2.4 Update decision-result projection and report serialization to expose live, peak, released, cumulative-used, and exhaustion-stage memory metrics.
- [x] 2.5 Add sequential, allocation-failure, repeated-teardown, and concurrent eight-worker ledger tests asserting zero unreconciled live bytes at decision end.

## 3. Fork-first candidate recovery

- [x] 3.1 Implement a post-placement opponent immediate-win probe that uses the established legality oracle/cache, stops at two replies for risk classification, and distinguishes complete results from budget-unknown results.
- [x] 3.2 Build the fork-probe candidate pool from the four-star move, generated tactical candidates, relevance-zone legal moves, and a canonical full-board extension when no safe candidate is found.
- [x] 3.3 Add stable candidate ranking by own win, mandatory defense, fork safety, opponent reply count, existing score, and coordinate tie-break, including coverage metadata.
- [x] 3.4 Integrate the probe before own-win/opponent-proof gates so a fork-safe alternative can be selected when deeper proof is unknown, while verified wins and mandatory defenses retain priority.
- [x] 3.5 Change invalid or incomplete four-star handoff recovery to retain the generated candidate set instead of constructing a single first-legal candidate.
- [x] 3.6 Ensure unknown or incomplete search selects a legal least-risk candidate, never infers verified loss from a truncated list, and reports explicit no-legal-move only after a complete legality scan.
- [x] 3.7 Add synthetic fork, forbidden-black reply, invalid-handoff, unknown-proof, and deterministic tie-break tests.

## 4. Deterministic worker evaluation

- [x] 4.1 Keep worker proof graphs and mutable boards private while routing all workers through the same decision ledger and absolute deadline.
- [x] 4.2 Dispatch candidate jobs in canonical order, store results by candidate index, and merge them deterministically without using thread completion order as a move-selection tie-break.
- [x] 4.3 Preserve verified early-stop behavior while joining launched workers and retaining enough canonical results for deterministic fallback selection.
- [x] 4.4 Add worker-cell telemetry for requested/actual workers, concurrent peak, proof sessions, completed proofs, useful nodes, early stops, and resource exhaustion.

## 5. Fixture replay and reporting

- [x] 5.1 Extract the 97 audited fallback positions into a versioned fixture manifest containing board, side, rule mode, legacy hint/fallback metadata, and source-log hash.
- [x] 5.2 Extend the replay harness to run the manifest under 1/4/8 workers with identical seeds, profile parameters, and deadlines, validating every selected move with the engine legality oracle.
- [x] 5.3 Extend Markdown/JSON report generation with fork avoidance, candidate coverage, fallback reason, live/peak/released memory, unknown rate, and useful parallel-work metrics.
- [x] 5.4 Add report validation that rejects selected-move/legality drift, unreconciled ledger bytes, false verified loss, or invalid worker-cell configuration.

## 6. Verification and strength gate

- [x] 6.1 Run the existing C/unit test target and the new ledger/fork fixture tests in both forbidden and no-forbidden modes.
- [x] 6.2 Run the 97-position replay and require legal output, fork avoidance whenever complete coverage finds a safe alternative, and zero live memory reservation at each decision end.
- [x] 6.3 Run deterministic 1/4/8 worker cells and confirm selected moves, decision statuses, and fork classifications are identical while reporting actual parallel advantage separately.
- [x] 6.4 Only after fixed gates pass, rerun the existing color/rule-separated 100-game comparison against frozen four-star and publish the paired Markdown/JSON evidence.
- [x] 6.5 Record the final profile manifest, fixture provenance, tool versions, and rollback target to keep the new result distinguishable from frozen 5.7 history.
