## 1. Profile and recovery data model

- [x] 1.1 Add isolated recovery-profile fields for immediate-block probing, two-step fork bounds, VCF-unknown VCT escalation, and their decision-ledger budgets in `FCAIProfile`.
- [x] 1.2 Add a distinct research profile/version derived from the soft-40 candidate and serialize all new fields without changing 5.8.1, frozen four-star, lower profiles, or the playable binding.
- [x] 1.3 Extend `FCAnalysisResult` and benchmark step serialization with selected-candidate recovery source, immediate-win count, fork classification, fork completion, VCT escalation, and final-candidate consistency telemetry.

## 2. Baseline-preserving guard recovery

- [x] 2.1 Snapshot the pre-double-three baseline, advisory/default coordinate, and four-star/handoff candidate set before structural preemption changes the provisional move.
- [x] 2.2 Build and deduplicate a guard recovery portfolio that explicitly retains the snapshot candidates alongside structural, tactical, certificate, and ordinary candidates.
- [x] 2.3 Add an immediate-opponent-win pre-pass that ranks candidates removing all current opponent winning moves ahead of candidates leaving an immediate loss, while preserving own immediate-win and verified-own-VCF priority.
- [x] 2.4 Make the guard selection and optional corpus handoff publish proof, loss, recovery, opponent-after-selected, and structural telemetry from one final selected-candidate snapshot.

## 3. VCF-unknown fork and VCT recovery

- [x] 3.1 Implement a deterministic bounded two-step opponent-fork probe that simulates black, enumerates legal white replies, counts resulting immediate white wins, and restores the board on every path.
- [x] 3.2 Integrate fork-risk/no-fork/unknown classifications into black recovery ordering without treating scoped no-fork results as global proof.
- [x] 3.3 Add the bounded VCT follow-up for relevant black recovery candidates whose opponent VCF is unknown, including certificate verification, scoped-disproof replay, ledger reservations, and fail-closed deadline handling.
- [x] 3.4 Add targeted diagnostics for skipped, incomplete, verified-loss, scoped-disproof, no-fork, fork-risk, and unknown recovery outcomes.

## 4. Regression and safety tests

- [x] 4.1 Add the opening-9 final-position fixture proving that a legal immediate blocker is not replaced by a structurally safe move that leaves an immediate white win.
- [x] 4.2 Add representative quiet-white-fork predecessor fixtures from the remaining opening-0, opening-3, and opening-10 black losses, including a case with a completed no-fork recovery alternative.
- [x] 4.3 Assert candidate isolation, deterministic repeated selection, coordinate-bound proof/loss metadata, scoped no-fork semantics, incomplete-probe unknown behavior, and board hash/stone-count restoration.
- [ ] 4.4 Run the normal C test suite plus ASan/UBSan builds with the new fixtures and confirm no existing control-profile tests change.

## 5. Benchmark and replay integration

- [ ] 5.1 Extend the benchmark profile selector and JSONL writer for the new recovery profile and fork/VCT telemetry while preserving the existing schedule format and exact 5.8.1 selector.
- [ ] 5.2 Extend replay validation to compare recovery telemetry, selected/provisional coordinates, proof certificates, fork classifications, and board restoration for every recorded move.
- [ ] 5.3 Run the paired 24-game candidate-black/candidate-white cells against frozen four-star and exact 5.8.1 using the existing 12-opening deterministic-best protocol.
- [ ] 5.4 Summarize black/white/overall W-D-L, immediate-block outcomes, fork/VCT recovery outcomes, latency percentiles, deadline/incomplete/rollback anomalies, and replay integrity failures.

## 6. Evidence and acceptance

- [ ] 6.1 Rebuild the benchmark and profile snapshot artifacts, record source/build hashes, and verify the new profile identity is distinct from the soft-40 and control profiles.
- [ ] 6.2 Run replay and certificate-integrity gates on both raw JSONL cells and exclude any invalid games from clean strength totals.
- [ ] 6.3 Run `openspec validate improve-five-star-black-defense-recovery --strict`, `git diff --check`, and the complete targeted test commands.
- [ ] 6.4 Publish the change-local evidence summary and document whether the candidate is ready for a later promotion review; do not update the playable binding in this change.
