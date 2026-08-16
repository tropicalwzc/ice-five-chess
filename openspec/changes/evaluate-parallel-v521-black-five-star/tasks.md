## 1. Freeze the hypothesis and baselines

- [x] 1.1 Checksum v5.2.1, 5.4.1, production v5.1, frozen four-star, corpus, rule helpers, benchmark/replay/report tools, UI mappings, and the two predecessor report bundles.
- [x] 1.2 Record the same-schedule one-worker/eight-worker evaluation contract, fresh-identity exclusions, rule-specific gates, five-second limit, and candidate-only status before implementation outcomes.

## 2. Implement isolated parallel v5.2.1 and routing

- [x] 2.1 Add an explicit parallel-root profile capability independent of `proofEngineCandidate`, preserving existing profile defaults and serialized compatibility.
- [x] 2.2 Add one-worker-control and eight-worker v5.2.1-black profiles with identical v5.2.1 policy/deadline inputs except worker and aggregate root budget.
- [x] 2.3 Make parallel root jobs use private mutable state, shared absolute deadline, no nested workers, at most eight simultaneous threads, and completed/certified merge semantics.
- [x] 2.4 Add the new hybrid analysis entry point routing white to 5.4.1 and black to parallel v5.2.1 with legal best-completed fallback below five seconds.
- [x] 2.5 Add benchmark selectors for the one-worker control and eight-worker candidate and serialize exact component, configured worker cap, cumulative workers/jobs, CPU, RSS, deadline, proof, and randomness telemetry.
- [x] 2.6 Preserve production/UI v5.1, frozen four-star single-thread behavior, legacy three-star, and lower levels unchanged.

## 3. Verify correctness, isolation, concurrency, and latency

- [x] 3.1 Add profile/routing fixtures for both rules proving white uses 5.4.1, black uses the requested v5.2.1 worker variant, and four-star launches no candidate workers.
- [x] 3.2 Add one-worker/eight-worker proof fixtures for completed result precedence, certificate soundness, board restoration, private state, worker cap, cumulative telemetry, no nested workers, and timeout fallback.
- [x] 3.3 Run strict C11 warnings-as-errors tests plus one/two/legacy-three-star, frozen four-star, production v5.1, v5.2.1, 5.4.1, and predecessor-hybrid rollback checks.
- [x] 3.4 Run ASan/UBSan and TSAN or an equivalent race audit over repeated eight-worker black and white searches with zero findings.
- [x] 3.5 Run same-position one/eight-worker diagnostics under both rules and require legal results, active eight-worker black batches, zero four-star worker launches, and every candidate decision at most 5,000 ms.
- [x] 3.6 Build the iOS Simulator Debug target and verify the player-facing five-star still maps to production v5.1.

## 4. Freeze and generate fresh paired schedules

- [x] 4.1 Freeze and checksum passing source, profiles, binaries, corpus, tools, component identities, random contract, resource limits, controls, diagnostics, and gates.
- [x] 4.2 Generate fresh Freestyle and forbidden master seeds and 50 natural color-exchanged openings per rule after freeze.
- [x] 4.3 Audit zero identity/position/seed overlap with prior formal, diagnostic, corpus-shaped, and failure data plus zero cross-rule overlap.
- [x] 4.4 Validate every prefix as legal and nonterminal with production C rule helpers under its rule mode.

## 5. Run same-schedule four-star evaluation

- [x] 5.1 Run 100 Freestyle one-worker-control games against frozen four-star with 50 control-black and 50 control-white games.
- [x] 5.2 Run 100 Freestyle eight-worker-candidate games against frozen four-star on the identical schedule with 50 candidate-black and 50 candidate-white games.
- [x] 5.3 Run 100 forbidden one-worker-control games against frozen four-star with 50 control-black and 50 control-white games.
- [x] 5.4 Run 100 forbidden eight-worker-candidate games against frozen four-star on the identical schedule with 50 candidate-black and 50 candidate-white games.

## 6. Replay, classify, and publish

- [x] 6.1 Independently replay all 400 games for schedule/color identity, routing, seeds, every move, rule/forbidden legality, terminal result, certificates, random eligibility, board integrity, latency, and worker-policy conformance.
- [x] 6.2 Generate Markdown/JSON/raw reports with same-color white/black statistics, overall results, eight-minus-one-worker paired effects, Wilson/paired uncertainty, latency/CPU/workers/RSS, commands, provenance, and limitations.
- [x] 6.3 Apply each rule's predeclared strength and threading gates without pooling, write the release decision, and leave production/UI v5.1 unchanged.
- [x] 6.4 Checksum the complete bundle, synchronize it to `/Users/wangzicheng/Downloads/logs_five_chess`, and verify byte-identical manifests.
