## Context

The completed `optimize-three-star-ai-search` change established reproducible engine boundaries, isolated randomness, candidate search, a transposition table, and a 200-game legacy benchmark. It did not establish a strength gain: the final production path scored 50%, every one of its 1,534 moves retained the legacy suggestion, and pure alpha-beta variants were weaker or much slower. Increasing the legacy 8/8/10 tactical depths to 10/10/12 also fell to 45% in training.

The legacy Objective-C engine already contains `harsh_super_fast_attack`, `harsh_four_hide_attack`, and `harsh_doublethree_hide_attack`, but these routines mix threat recognition, random traversal, local continuation scans, and Boolean early exits. Some helpers return only the first key point, so they cannot serve as complete proof certificates. The current C forcing search recognizes mostly immediate winning replies; deeper uniform alpha-beta therefore spends budget without obtaining a reliable quiet-position value.

The application supports 15×15 play with optional black forbidden moves and must remain responsive on mobile devices. The new core direction must not require training data, neural weights, online services, or a third-party runtime engine. Opening research may use the network during implementation, but the shipped library is local and immutable.

Two source families were verified while preparing this design:

- The official Renju International Federation site, RenjuNet (`https://www.renju.net/`), exposes federation opening material and a searchable database currently advertising more than 158,000 Renju/Gomoku games, including rule and tournament metadata. The site states “All rights reserved,” so it is a discovery and validation source unless redistribution permission is confirmed.
- The official Gomocup site (`https://gomocup.org/results/`) publishes annual AI tournament results. Its 2025 page states that five named Gomoku/Renju experts selected 12 openings for every rule, separates freestyle 15×15, standard, Renju, Caro, and freestyle 20×20, and links an official results/openings archive at `/static/tournaments/2025/results/gomocup2025results.zip`. These expert-selected, rule-labeled sets are the preferred initial candidates, subject to license review.

## Goals / Non-Goals

**Goals:**

- Add sound, deterministic VCF and budgeted DFPN/VCT proof search that explores forcing lines rather than increasing uniform alpha-beta depth.
- Improve both attack and defense by proving a new move wins or proving the default move permits an opponent tactical win before overriding the frozen legacy advisor.
- Build a small, diverse, rule-compatible opening library from traceable professional sources, with explicit source and rights metadata.
- Preserve user-game variety only among tactically equivalent choices.
- Keep the frozen legacy advisor at three stars and expose the demonstrated no-book proof profile as a distinct four-star difficulty on both iPhone and iPad.
- Validate with proof problems and two held-out 200-game legacy matches—book-on and book-off—whose primary reported color metric is the new engine's white score, plus a direct comparison report.

**Non-Goals:**

- Solving unrestricted 15×15 Gomoku, proving full-game draws, or treating a scoped VCT disproof as a game-theoretic result.
- Adding MCTS, neural networks, NNUE, learned pattern weights, self-play training, or an online move service in this change.
- Importing complete third-party databases without source and redistribution review.
- Artificially balancing black and white outcomes or hiding Gomoku's natural black advantage.
- Replacing one-star, two-star, or frozen legacy three-star behavior.

## Decisions

### 1. Build a new pure-C threat model instead of extending the `harsh_*` recursion

The solver will use a rule-aware board API shared with `FiveChessAI.c` and represent a threat as `(gain, cost[], rest[], severity, side, rule)`. Detection scans all four lines through a candidate, deduplicates defense points, and emits immediate five, open/simple/broken four, four-three and eligible three threats. VCF initially admits only continuous-four forcing sequences; VCT adds three-based threats after the VCF layer is correct.

The existing `harsh_*` functions remain frozen as the legacy advisor. Reusing them as the proof engine was rejected because their first-match returns, random defense ordering, local continuation windows, coordinate defects, and mixed board mutation make proof completeness difficult to establish.

### 2. Use AND/OR semantics with VCF first and DFPN for VCT

At attacker OR nodes, one verified continuation suffices. At defender AND nodes, every legal reply capable of answering the threat or winning/counter-threatening must be refuted. DFPN focuses work using proof/disproof numbers rather than expanding ordinary moves uniformly. It uses node and wall-clock ceilings, depth/sequence limits, a Zobrist transposition table keyed by rule and search class, and deterministic severity/distance ordering.

The result domain is deliberately three-valued:

```text
PROVEN_WIN
NO_FORCED_WIN_IN_SCOPE   // only the configured VCF/VCT class
UNKNOWN                  // budget or unsupported shape prevented completion
```

This was chosen over alpha-beta because the branching structure is highly asymmetric and terminal proof matters more than a noisy static score. PUCT/MCTS was deferred because its useful form would require a stronger trained or hand-tuned policy/value function, which conflicts with the data-free scope.

### 3. Verify proof certificates independently before they affect production moves

The searcher records a compact proof DAG containing board hashes, attacker choices, all defender children, terminal reasons and distance. A separate verifier starts from the input board and replays make/unmake operations. At each AND node it regenerates legal defensive replies, including immediate wins and relevant counter-threats, and rejects an omitted branch. A certificate failure converts the result to `UNKNOWN` and is a test/benchmark anomaly.

Search and verification sharing low-level legality helpers is acceptable; sharing traversal state or trusting the searcher's child list without regeneration is not. This extra work is justified because an unsound “proven” override is worse than retaining the legacy move.

### 4. Wrap a default advisor with proof evidence instead of mixing scores

The decision flow is:

```text
immediate win / complete immediate defense
                    ↓
      compatible opening suggestion, else legacy suggestion
                    ↓
        prove own forcing candidates if tactically eligible
                    ↓
       test whether default permits opponent VCF/VCT
                    ↓
 verified win → override
 verified default loss + verified escape → override
 unknown/no applicable proof → retain safe default
```

Proof classes dominate heuristic scores. Within verified wins, shorter distance dominates; within proven-loss moves, longer survival dominates. Only equal proof/tactical classes may enter the existing isolated seeded random selector. This evidence-driven portfolio was chosen over majority voting because the previous three-advisor voting experiment did not improve strength.

The player-facing app exposes this portfolio as four stars and keeps the direct `harsh_analysisboard` path at three stars. The four-star binding always selects the demonstrated book-off profile; the weaker book-on result remains available for diagnostics only.

### 5. Treat the opening library as a versioned advisor, not training data

An offline import tool will accept explicit source packages, normalize coordinates, validate alternating moves and rule legality, canonicalize all eight symmetries, and emit a compact local asset plus a provenance manifest and checksum. It does not learn weights or evaluate positions from outcomes.

Initial source priority is:

1. Official Gomocup expert-selected opening packages, starting with rule-compatible freestyle 15×15 for no-forbidden play and Renju sets only for matching forbidden rules.
2. RIF/RenjuNet opening definitions and searchable tournament records for naming, frequency and expert-game validation. Because the site declares all rights reserved, raw records will not be bundled unless permission is documented.
3. Other federation/tournament sources only after equivalent provenance and redistribution review.

The product stores several common alternatives and uses a per-game seed to avoid identical games. Every book move still passes legality, immediate safety, and proof override checks. Full professional games will not be replayed as a script; only bounded opening prefixes are used so the searcher controls the middle game.

### 6. Preserve black advantage and make white performance a primary benchmark dimension

The formal suite retains 100 held-out opening identities and exchanges engine colors for exactly 200 games. It does not force equal black/white win counts. Results are ordered in the report as:

1. new engine as white: W/D/L, score and Wilson interval;
2. frozen legacy as white in the directly corresponding new-engine-as-black games, and the paired white delta across the same held-out opening identities;
3. new engine as black;
4. overall paired result;
5. tactical coverage and performance.

The separate legacy-versus-legacy control establishes schedule reproducibility and the natural color baseline only. Its designated white result is supplemental and SHALL NOT be substituted for the direct-match legacy-white result when calculating the white delta or strength classification.

A small fixed pre-final diagnostic may compare book-on and book-off before freezing the candidate. After freezing, both configurations run full formal schedules of 100 new-black and 100 new-white games against the same frozen legacy engine. They share the same held-out opening identities and paired seed derivation so their difference estimates the opening library's effect. The formal schedule, seeds and source opening IDs are held out from algorithm/profile selection. Strength wording uses three levels: demonstrated, directional, or not demonstrated. Overall statistical significance alone is insufficient if white performance regresses.

### 7. Separate product opening benefit from search benefit

Benchmark telemetry labels every move as book default, legacy default, proof attack override, proof defense override, immediate tactic, or random equivalent choice. The report aggregates results by source. The frozen book-on profile and otherwise identical book-off profile each receive an independent formal Markdown/raw-data report based on the same 100 held-out opening identities. A third comparison report presents book-on minus book-off overall and by color, coverage, move-source, latency and uncertainty; only improvements present in book-off are attributed to proof search.

## Risks / Trade-offs

- [Threat generator omits a legal defense and produces a false proof] → Regenerate defenses in an independent certificate verifier, add adversarial multi-defense/refutation positions, and fail closed to `UNKNOWN`.
- [DFPN still exceeds mobile latency in broad VCT positions] → Run VCF first, use strict node/time ceilings and persistent TT, return the safe default on `UNKNOWN`, and measure p50/p95/max on release hardware.
- [A scoped disproof is mistaken for full safety] → Encode the search class in the result and telemetry; expose `NO_FORCED_WIN_IN_SCOPE`, never a generic “safe/win” claim.
- [Opening sources use incompatible rules] → Partition assets by rule and board size and validate every prefix using the same runtime legality implementation.
- [Third-party records cannot be redistributed] → Store no raw record until a rights review passes; retain URL/checksum metadata and prefer official downloadable opening packages.
- [Opening repetition reduces user variety] → Keep multiple canonical alternatives, choose via isolated per-game seeds, and stop book play at a bounded prefix.
- [Natural black advantage masks changes] → Preserve raw paired results but elevate new-white score and the direct-match legacy-white comparison to primary metrics; keep legacy self-play as a supplemental color baseline.
- [Only the opening library improves results] → Require book-off ablation and source-labeled move/result aggregation before attributing gain to proof search.
- [Formal 200 games lack power for small effects] → Report Wilson/paired uncertainty, avoid binary claims for inconclusive effects, and retain raw schedules so more held-out openings can be appended without reusing tuning data.

## Migration Plan

1. Freeze and hash the current legacy advisor, current production profile, rule behavior and previous final-v2 report as comparison inputs.
2. Add the pure threat representation, VCF solver, proof certificates and verifier behind a disabled profile flag; build tactical/symmetry regression coverage.
3. Add DFPN/VCT and defensive counter-proof search; establish release-device budgets while `UNKNOWN` continues to fall back to legacy.
4. Audit and import approved opening packages, generate the local library/manifest, and validate book randomness and rule separation.
5. Integrate the proof-guided decision flow behind a selectable profile; run tactical gates and book-on/book-off diagnostic schedules.
6. Freeze code, profiles, library checksum, held-out opening schedule and seeds; run book-on versus legacy for 100 black plus 100 white games, then book-off versus legacy for 100 black plus 100 white games, and generate two standalone reports plus a direct comparison report.
7. Keep the frozen legacy advisor at three stars and expose the demonstrated no-book proof-guided profile as four stars on both iPhone and iPad. Persist the new four-star value separately while mapping the existing saved 0/1/2 values back to three/two/one stars unchanged.

## Open Questions

- RenjuNet's visible “All rights reserved” notice requires permission or a documented legal determination before any raw game prefix is redistributed; until then it remains a validation source only.
- The exact bounded prefix length and minimum number of distinct openings should be selected after inspecting the approved Gomocup packages and measuring repetition, but they must be frozen before the held-out schedule is generated.
- Device-specific proof budgets require measurement on the oldest supported iPhone/iPad; node budget remains the reproducibility authority and wall time remains a safety ceiling.
