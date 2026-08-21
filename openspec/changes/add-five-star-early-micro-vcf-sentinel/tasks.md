## 1. Freeze Baselines and Reconstruct Diagnostics

- [x] 1.1 Record version strings and SHA-256 hashes for the completed 5.8.0 opponent-guard candidate, exact UI-bound 5.4.1 control, proof engine, rule helpers, corpus, tests, runner, and prior direct-match JSONL.
- [x] 1.2 Confirm with a source-level/UI-binding regression that the playable five-star entry still selects `5.4.1-transactional-deadline-root-parallel-5s`.
- [x] 1.3 Reconstruct the three late-correction boards from the prior direct-match move streams and provisional coordinates without embedding opening IDs or coordinates into runtime policy.
- [x] 1.4 Independently replay the opponent certificates from the reconstructed boards and record their shortest available VCF distance and active rule.
- [x] 1.5 Add or freeze independently replayable VCF distance-3, distance-5, distance-7, and distance-9 targeted fixtures where constructible.

## 2. Add Isolated Profile and Telemetry Contracts

- [x] 2.1 Extend `FCAIProfile` with sentinel enablement, base/max depth, node/time budgets, adaptive mode, and early alternative cap using disabled defaults for existing profiles.
- [x] 2.2 Add a new isolated candidate factory derived from `5.8.0-vcf-first-opponent-guard-4w` and preserve byte-comparable parent and 5.4.1 profile identities.
- [x] 2.3 Extend analysis-result telemetry with sentinel eligibility/skip reason, policy/effective depth, provisional and replacement coordinates, proof status/distance/certificate verification, nodes/time, alternatives, replacement source, and rollback.
- [x] 2.4 Extend aggregate diagnostics with sentinel queries, completed proofs/disproofs/unknowns, early verified losses avoided, adaptive escalations, cache hits, fresh nodes, final-guard-only catches, and deadline exhaustion counters.
- [x] 2.5 Add decision-ledger accounting for the bounded sentinel slice without extending the existing internal or hard deadline.
- [x] 2.6 Expose the isolated candidate and sentinel diagnostic overrides in the benchmark runner without changing the playable UI binding.

## 3. Implement the Micro-VCF Sentinel

- [x] 3.1 Implement a transactional opponent-after-move strict VCF sentinel that skips legal immediate own wins and restores the board on every exit path.
- [x] 3.2 Place the sentinel after the legal four-star/fork provisional selection and before own-proof eligibility, quiet search, loss-aware refinement, corpus work, and the final guard.
- [x] 3.3 Verify every reported opponent `PROVEN_WIN` certificate independently before classifying or rejecting the provisional move.
- [x] 3.4 Convert certificate failure, overflow, illegal placement, deadline, incomplete enumeration, and restoration failure to conservative `unknown` without a safety claim.
- [x] 3.5 Record `NO_FORCED_WIN_IN_SCOPE` with its exact VCF depth/budget while allowing normal downstream work and the final full guard.
- [x] 3.6 Implement fixed depth-5 and fixed depth-7 policy selection using profile-scoped node and time limits.
- [x] 3.7 Implement the adaptive depth-5/7 forcing signal for opponent four, open-four, and VCF dependency conditions, treating the signal only as escalation eligibility.

## 4. Implement Early Replacement and Proof Reuse

- [x] 4.1 Reuse verified certificate gain/cost/rest/dependency data to seed early escape candidates before existing deterministic tactical and generated alternatives.
- [x] 4.2 Audit early alternatives under the same bounded policy and cap, rejecting only replayable verified losses and preserving honest `unknown` labels.
- [x] 4.3 Select a deterministic unresolved alternative over a verified loss when available, and use verified survival ordering when all audited alternatives lose.
- [x] 4.4 Ensure downstream own-proof, quiet, loss-aware, and corpus logic runs from the early replacement coordinate rather than the rejected provisional coordinate.
- [x] 4.5 Add compatible proof-session cache keys covering canonical board, side, rule, VCF class, scope, and proof-engine version.
- [x] 4.6 Reuse independently replayed sentinel win certificates in the final guard and permit shallow disproof work only to seed, never satisfy, broader queries.
- [x] 4.7 Preserve the final depth-9 VCF/conditional depth-10 VCT guard as the authoritative acceptance boundary for final and corpus-selected moves.
- [x] 4.8 Record and resolve any sentinel/final-guard evidence mismatch in favor of completed verified final evidence.

## 5. Add Correctness and Invariant Tests

- [x] 5.1 Add tests that existing one-star through four-star, frozen 5.8.0, and UI-bound 5.4.1 behavior and profile identities remain unchanged when the sentinel is disabled.
- [x] 5.2 Add tests that immediate legal own wins bypass the sentinel while ordinary and mandatory-defense provisional moves remain eligible.
- [x] 5.3 Add positive certificate-replay tests and negative malformed/missing-certificate tests proving that unverified search output never rejects a move.
- [x] 5.4 Add scoped-disproof and `unknown` tests proving neither state is labeled globally safe nor skips the final guard.
- [x] 5.5 Add both-rule and eight-symmetry tests for sentinel classification, adaptive signals, certificate replay, and deterministic replacement.
- [x] 5.6 Add board-restoration, illegal-placement, overflow, deadline, transactional rollback, ledger-limit, and no-legal-alternative tests.
- [x] 5.7 Add cache compatibility tests across board, side, rule, class, depth/scope, and engine version, including verified-win replay on reuse.
- [x] 5.8 Add interaction tests showing deeper VCF/VCT and corpus replacements still reach the full final guard after sentinel disproof or `unknown`.
- [x] 5.9 Run the complete AI test suite repeatedly in deterministic mode and under the available sanitizer/integrity configuration.

## 6. Extend Targeted Benchmarking

- [x] 6.1 Emit all sentinel configuration, per-decision telemetry, cache reuse, downstream budget, final-guard outcome, and provenance fields to benchmark JSONL.
- [x] 6.2 Add targeted-run support for fixed depth 5, fixed depth 7, and adaptive depth 5/7 with the proposed node/time ranges.
- [x] 6.3 Run all three policies on VCF distances 3/5/7/9, reconstructed late-correction boards, both rules, and relevant symmetries.
- [x] 6.4 Independently replay every reported proof, verify zero false rejection, check board integrity, and repeat deterministic runs for equality.
- [x] 6.5 Report per-policy recall by distance, reconstructed-position catches, p50/p95/max time, nodes, cache reuse, downstream budget preserved, final move changes, later full-guard catches, and deadline behavior.
- [x] 6.6 Select and freeze the lowest-p95 eligible policy with nodes as tie-breaker; reject adaptive mode on any in-scope recall loss versus fixed depth 7.
- [x] 6.7 If no bounded policy is useful and correct, publish the negative targeted result, keep the sentinel disabled, and stop before direct games. (Condition evaluated; not triggered because adaptive passed.)

## 7. Run the Fresh Quick Test Against 5.4.1

- [x] 7.1 Freeze candidate parameters and hashes before generating match inputs.
- [x] 7.2 Generate and record 6–8 fresh natural freestyle opening identities excluded from openings 60–65, targeted fixtures, corpus-derived diagnostics, and prior result-selected schedules.
- [x] 7.3 Run deterministic-best paired play with colors exchanged against exact UI-bound 5.4.1 for 12–16 total games under the existing deadlines.
- [x] 7.4 Replay the raw match evidence, verify every certificate, check move legality and board integrity, and investigate every anomaly or decision above 5,000 ms.
- [x] 7.5 Produce a directional report leading with candidate-black W/D/L, followed by white/overall and paired-opening outcomes.
- [x] 7.6 Report early verified-loss catches, final-guard-only catches, final move changes, sentinel and total p50/p95/max latency, cache reuse, anomalies, and hard-deadline violations.
- [x] 7.7 Checksum the raw JSONL, opening schedule, profiles, engine, rule helpers, corpus, runner, seed, and report inputs.

## 8. Final Verification and Handoff

- [x] 8.1 Run OpenSpec validation and confirm every scenario has corresponding implementation or evidence coverage.
- [x] 8.2 Re-run source/UI-binding checks proving the playable five-star entry remains on 5.4.1 regardless of the quick-test outcome.
- [x] 8.3 Summarize the selected policy or negative result, targeted correctness evidence, quick-match outcome, remaining limitations, and reproducible commands in the change evidence index.
- [x] 8.4 Review the final diff for accidental runtime coordinates/opening IDs, unrelated edits, changed lower-level behavior, or promotion logic.
