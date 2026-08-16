## 1. Freeze inputs and evaluation contract

- [x] 1.1 Create a checksummed baseline manifest for production v5.1, revision-7 proof-engine candidate, frozen four-star, legacy three-star, corpus, runner, rule helpers, current UI mappings, and prior reports.
- [x] 1.2 Record revision-7 results as diagnostic motivation only, exclude its formal identities and seeds, and document that this change validates seeded stochastic correctness without requiring separately seeded move-for-move regeneration.
- [x] 1.3 Freeze the color-specialization hypothesis and gates before implementation outcomes are available: white proof-engine, black v5.1, production-style constrained randomness, four 100-game cells, and no pooled-rule/color override.

## 2. Implement the candidate hybrid and stochastic telemetry

- [x] 2.1 Add candidate-only hybrid component identities, version constants, result provenance, decision-seed, random selected-rank, and completed equivalence-signature fields without changing existing public level behavior.
- [x] 2.2 Implement the analysis router so white delegates directly to the revision-7 proof-engine candidate and black delegates directly to production v5.1 for both rules and random modes.
- [x] 2.3 Add a `five-star-color-hybrid` benchmark profile/engine selector and serialize the hybrid profile plus exact per-step component provenance in JSONL.
- [x] 2.4 Enforce and expose seeded random eligibility across completed proof class, scope, distance, tactical obligation, corpus support, and safety/loss status; keep budget exhaustion distinct from random selection.
- [x] 2.5 Extend stochastic replay tooling to validate schedule/color identity, seed/provenance, C legality, forbidden rules, terminal results, certificates, random equivalence signatures, and board integrity without demanding identical separately seeded moves.
- [x] 2.6 Extend report tooling with hybrid-white-first same-color statistics, Wilson/paired uncertainty, routing/randomness activity, component latency/CPU/workers/RSS, and point-estimate gate classification.

## 3. Pass component, randomness, correctness, and latency gates

- [x] 3.1 Add no-forbidden and forbidden fixtures proving hybrid black matches direct v5.1 and hybrid white matches the direct proof-engine candidate for identical board, seed, hint, random mode, and resource inputs.
- [x] 3.2 Add multi-seed fixtures that demonstrate intended variation only among equal completed equivalence signatures and reject score-near but proof/tactical-unequal alternatives.
- [x] 3.3 Run strict C tests, one/two-star golden, frozen legacy three-star golden, frozen four-star golden/control checks, production v5.1 rollback checks, and board-integrity/certificate replay.
- [x] 3.4 Run ASan/UBSan and TSAN or an equivalent race audit over hybrid routing, random selection, and eight-worker white searches with zero findings.
- [x] 3.5 Build the iOS Simulator Debug target and verify production/UI five-star remains v5.1 while the hybrid is candidate/test-only.
- [x] 3.6 Run same-position latency/resource diagnostics under both rules and require every hybrid decision to return a legal best-completed move within 5,000 ms while reporting component CPU, workers, and peak memory.

## 4. Freeze and generate fresh stochastic schedules

- [x] 4.1 Freeze and checksum the passing router, component profiles, structural/resource limits, corpus, randomness contract, controls, tools, diagnostics, and evaluation criteria.
- [x] 4.2 Generate fresh no-forbidden and forbidden master seeds only after freeze, build 50 color-exchanged natural openings per rule, and audit separation from every prior formal, diagnostic, corpus-shaped, and failure identity/position/seed.
- [x] 4.3 Validate every scheduled prefix with the production C rule helpers as legal and nonterminal under both rule modes, and verify zero cross-rule identity/position overlap.

## 5. Run four fresh 100-game general-strength cells

- [x] 5.1 Run 100 no-forbidden seeded-user-random games against frozen four-star, with 50 hybrid-black and 50 hybrid-white games.
- [x] 5.2 Run 100 no-forbidden seeded-user-random games against legacy three-star, with 50 hybrid-black and 50 hybrid-white games.
- [x] 5.3 Run 100 forbidden seeded-user-random games against frozen four-star, with 50 hybrid-black and 50 hybrid-white games.
- [x] 5.4 Run 100 forbidden seeded-user-random games against legacy three-star, with 50 hybrid-black and 50 hybrid-white games.

## 6. Replay, classify, and publish

- [x] 6.1 Independently replay all 400 recorded games for schedule/color identity, seeds/provenance, every move, rule legality, terminal result, accepted certificates, random equivalence signatures, and board integrity; invalidate any anomalous cell.
- [x] 6.2 Generate Markdown/JSON/raw reports that lead with hybrid white, compare same-color white and black correctly, retain natural first-player advantage, and include uncertainty, randomness, routing, latency, commands, provenance, limitations, and checksums.
- [x] 6.3 Apply the predeclared rule-specific gates against four-star and legacy three-star without pooling, write `release_decision.json`, and keep production v5.1 unless a later explicit UI promotion change is authorized.
- [x] 6.4 Synchronize the complete checksummed bundle to `/Users/wangzicheng/Downloads/logs_five_chess` and verify byte-identical workspace/Downloads manifests.
