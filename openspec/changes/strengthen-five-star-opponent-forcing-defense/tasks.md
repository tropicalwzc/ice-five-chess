## 1. Freeze controls and reproduce the defensive gap

- [x] 1.1 Record checksums and profile snapshots for the UI-bound five-star 5.4.1 control, frozen four-star, one-through-three-star controls, corpus, rule helpers, proof verifier, benchmark tools, and current playable binding.
- [x] 1.2 Find or construct at least one black-to-move separating fixture where 5.4.1 selects a move with a replayable shallow white VCF and another legal move completes a comparable scoped disproof; keep any direct-play coordinates diagnostic-only.
- [x] 1.3 Build a checksummed diagnostic manifest covering no-corpus, frozen-four-star-unknown, mandatory-defense continuation, corpus conflict, VCF distances 3/5/7/9 where legal, all eight symmetries, and both rule modes.
- [x] 1.4 Add an audit proving diagnostic positions, symmetries, losing opening IDs, and result-selected coordinates are absent from runtime policy and future smoke schedule generation.

## 2. Add the isolated profile, guard state, and telemetry

- [x] 2.1 Add a distinct five-star opponent-guard candidate profile derived from UI-bound 5.4.1 while leaving existing five-star, v5.7 research, and one-through-four-star profiles unchanged and selectable.
- [x] 2.2 Add frozen profile fields for guard enablement, VCF/VCT scope, reserved nodes/time opportunity, structural VCT eligibility, and maximum audited alternatives, and include them in profile snapshots.
- [x] 2.3 Extend analysis and proof diagnostics with guard eligibility/skip reason, provisional/selected moves, per-class status/distance/nodes/time, certificate verification, audited stages/counts, completed disproofs, unknowns, verified losses, avoided loss, and rollback.
- [x] 2.4 Extend benchmark JSONL serialization and reporters for all new guard/profile fields without changing old profile output semantics.

## 3. Reserve defensive resources transactionally

- [x] 3.1 Extend the decision ledger with a five-star-only defensive node reservation that earlier own-attack, quiet, and corpus stages cannot consume.
- [x] 3.2 Make guard reservation, consumption, release, worker unwind, and memory accounting saturating and reversible under success, skip, allocation failure, node exhaustion, and absolute deadline.
- [x] 3.3 Enforce the existing 4.5-second internal reservation and 5-second player-visible hard limit without granting a fresh deadline to VCF, VCT, or alternative batches.
- [x] 3.4 Add ledger tests proving earlier-stage exhaustion preserves the defensive reservation, a verified own win releases it, and an interrupted guard restores the last completed legal result.

## 4. Implement the universal VCF-first opponent guard

- [x] 4.1 Implement a reusable opponent-after-move audit that makes/unmakes the proposed move, runs strict rule-aware VCF from the resulting board, and returns honest proven-win/scoped-disproof/unknown metadata with exact restoration.
- [x] 4.2 Add the qualifying opponent VCT root/dependency predicate and continue from completed VCF to bounded VCT only when eligible resources remain.
- [x] 4.3 Require independently replayable certificates for verified opponent wins and independently replayable completed-scope evidence for accepted disproofs; downgrade every failure or incomplete enumeration to unknown.
- [x] 4.4 Add one common five-star acceptance boundary before ordinary, mandatory-defense, no-corpus, quiet, and corpus final returns; skip it only for a legal immediate win or independently verified own win.
- [x] 4.5 Preserve deterministic evaluation ordering and user randomness only among candidates equal in completed guard class, scope, distance, tactical obligation, and corpus support.

## 5. Strengthen mandatory defense and escape composition

- [x] 5.1 Preserve all generated legal immediate-defense moves and audit their opponent continuations instead of treating removal of the current win as completed safety.
- [x] 5.2 When a provisional defense is verified losing, audit remaining mandatory blocks in deterministic tactical/coordinate order and retain the mandatory-defense obligation on the selected result.
- [x] 5.3 Reuse verified gain/cost/rest/dependency/certificate points to seed defensive alternatives before tactical, ordinary, nearby, and remaining-legal widening.
- [x] 5.4 Apply completed-class ordering `own verified win > comparable opponent disproof > immediately safe unknown > verified loss`, order verified losses by longest survival, and expose incomplete coverage honestly.
- [x] 5.5 Route every corpus replacement through the same guard and reject it when it cannot match or exceed a completed defense's proof scope.
- [x] 5.6 Remove or update the existing test expectation that mandatory-defense decisions perform zero VCF/VCT work, replacing it with continuation-audit assertions.

## 6. Pass targeted correctness and performance gates

- [x] 6.1 Add table-driven tests for the separating fixture, shallow VCF distances, no-corpus early exit, inherited four-star unknown, mandatory-block continuation, corpus conflict, unknown-over-known-loss, and longest survival.
- [x] 6.2 Run every applicable regression under all eight symmetries and both rule modes, checking transformed move/proof equivalence, forbidden legality, certificate/disproof replay, and exact board restoration.
- [x] 6.3 Add tests for verified-own-win guard skip, VCF-before-VCT order, structural VCT skip, deadline/overflow/certificate corruption, deterministic repeat, and lower-profile isolation.
- [x] 6.4 Run the complete C suite, frozen lower-level golden tests, randomized make/unmake/reference checks, ASan/UBSan, concurrency/race audit, and deterministic regeneration with zero new failures.
- [x] 6.5 Build the iOS Simulator target and verify every candidate decision returns the last completed legal move below the 5,000 ms hard ceiling with bounded peak memory.
- [x] 6.6 Measure guard activity, VCF/VCT completion, candidate coverage, p50/p95/max latency, node/memory consumption, and budget exhaustion on diagnostic plus non-final natural positions.

## 7. Select and freeze structural parameters

- [x] 7.1 Ablate reserved node/time splits, VCF/VCT depths, VCT eligibility, worker count, and maximum alternatives using diagnostic and non-final natural positions only.
- [x] 7.2 Select the smallest configuration that removes every completed avoidable shallow-VCF regression while satisfying correctness, lower-profile isolation, memory, and hard-deadline gates; document rejected variants.
- [x] 7.3 Freeze and checksum candidate source/profile, selected parameters, controls, corpus, diagnostics, runner/replay/report tools, and predeclared smoke classification before generating smoke seeds.

## 8. Run and report the fresh 48-game four-star smoke A/B

- [x] 8.1 Generate 12 fresh natural freestyle opening identities after freeze and audit separation from diagnostics, symmetries, corpus-shaped starts, prior loss fixtures, non-final runs, and result-selected schedules.
- [x] 8.2 Run UI-bound 5.4.1 versus frozen four-star for 24 games from the 12 openings with colors exchanged and paired deterministic seeds.
- [x] 8.3 Run the opponent-guard candidate versus the same frozen four-star for 24 games using the identical openings, color exchange, seeds, random contract, resource limits, and host policy.
- [x] 8.4 Independently replay all 48 games for schedule/profile/color identity, every move, rule legality, terminal result, board integrity, accepted certificates/disproofs, guard provenance, and the 5,000 ms ceiling; invalidate the dataset on any anomaly.
- [x] 8.5 Diagnose every lost five-star-black game at the first selected move after which white has a replayable VCF/VCT, classifying whether a completed defensive alternative existed.
- [x] 8.6 Generate black-first Markdown/JSON/raw reports with per-profile black/white/overall W/D/L, score rates and intervals, avoidable forcing incidents, guard activity, latency, exhaustion, commands, provenance, limitations, and checksums.
- [x] 8.7 Apply the predeclared smoke gates without tuning on outcomes; label the 48 games directional and retain 5.4.1 if any correctness, forcing-incidence, black-direction, white/overall smoke, or deadline gate fails.
- [x] 8.8 If every gate passes, bind only the playable five-star entry to the new candidate while keeping 5.4.1 as rollback; otherwise leave the UI binding unchanged and publish the failed evidence.
- [x] 8.9 Record that any formal strength or forbidden-mode promotion claim requires a separately proposed, larger, freshly frozen dual-rule evaluation.
