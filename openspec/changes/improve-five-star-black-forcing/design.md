## Context

The frozen five-star rule-partitioned suite contains 10 five-star-black losses in no-forbidden play and 7 in forbidden play against four-star; the same opening identities account for all but one corresponding legacy loss. Every one of those 17 primary losses contains a verified opponent forced-win event for the selected default with no recorded escape, while no five-star-black win contains that event. The decision layer examines at most six heuristic alternatives and accepts only a completed scoped disproof; if none qualifies it retains the already proven-losing default.

Seven losses contain a corpus move different from four-star. Same-seed counterfactual replays with the corpus disabled still lost all seven, establishing that corpus tuning is not the primary fix, although the corpus can change loss distance. The current VCT routine calculates proof/disproof numbers but traverses threats in ordinary depth-first order; full-board threat/refutation scans consume much of the wall budget, especially under forbidden-move legality. The current corpus also cannot advise the two-stone position, and evaluation mode still inherits a seed-dependent frozen-legacy hint.

The existing five-star corpus, frozen four-star, frozen legacy three-star, UI mappings, rule partitions, and previous formal report remain comparison baselines. Prior final games may be used for diagnosis and regression only, never to select a production coordinate or count as new strength evidence.

## Goals / Non-Goals

**Goals:**

- Remove the known-losing-default retention failure with certificate-directed, progressively widened defense search and explicit completed proof-class ordering.
- Make proof work materially more efficient through most-proving-node selection, shared transpositions, and localized/incremental threat generation.
- Add a bounded proactive stage that can find one quiet setup move before a forcing VCF/VCT sequence.
- Keep corpus advice subordinate to verified forced defenses and make evaluation output independent of legacy random seeds.
- Demonstrate generalization on fresh natural schedules under both rule modes against both frozen baselines.

**Non-Goals:**

- Claiming a complete solution of unrestricted 15x15 Freestyle or forbidden/Renju.
- Encoding the 17 failed opening IDs, their continuations, or opponent-specific response tables into production.
- Changing one-star through four-star behavior, weakening proof-certificate verification, or pooling evidence across rules.
- Using formal outcomes to tune weights or schedules after the profile is frozen.

## Decisions

### 1. Treat prior losses as proof regressions, not opening advice

For each loss, tooling identifies the first verified opponent-win decision with no escape plus earlier corpus divergence where present. The pre-decision board and all eight symmetries become diagnostic fixtures. A fixture asserts legality, board integrity, certificate validity, proof-class ordering and deterministic choice; it does not assert that one historical coordinate is globally winning unless independently proven.

Alternative rejected: store successful counter-lines from the paired games. That would overfit the released schedule and make the prior benchmark unusable as evidence.

### 2. Search certificate-directed escape candidates before heuristic widening

Once the default is verified losing, candidate priority is:

1. own immediate or verified win;
2. legal moves appearing in the opponent proof DAG as gain, cost, rest or counter-threat interruption points;
3. legal moves that invalidate the root certificate board/threat dependency;
4. remaining tactical candidates;
5. progressively widened ordinary candidates, ultimately all legal moves while the emergency budget remains.

Each alternative is evaluated against the opponent under a shared proof session. The rank is `own proven win`, `opponent no-forced-win-in-comparable-scope`, `unknown`, then `opponent proven win`; verified losses are ordered by longest distance. Unknown is not called safe, but it ranks above a verified loss after immediate safety checks.

Alternative rejected: simply increase the fixed six-candidate limit. It wastes budget on heuristic moves and still misses certificate-specific defenses.

### 3. Implement most-proving-node DFPN with shared work

The proof engine will maintain per-node proof/disproof thresholds, expand the most-proving child, update saturated AND/OR values, and key reusable transpositions by board, side, rule, search class, depth and profile version. Alternative checks within one decision reuse a bounded session table. Threat enumeration uses occupied-line/frontier indexing and cached rule-aware legality rather than rescanning every intersection for every nested creator query.

Correctness remains fail-closed: overflow, incomplete enumeration or certificate verification failure returns unknown. Node budget is the reproducibility authority; wall time remains a safety ceiling.

### 4. Add one bounded quiet-threat setup ply

When neither side has an immediate obligation and ordinary VCT has no proof, five-star may examine a small deterministic set of quiet moves that create at least two independent future threat dependencies or materially reduce the opponent's proof distance. Each quiet root is followed by the normal verified VCF/VCT solver. A quiet move may override four-star only with a completed verified proof or a frozen diagnostic rule that is independent of formal outcomes and passes opponent counter-proof.

Alternative rejected: uniform alpha-beta depth increases, which previous experiments showed were slower and weaker.

### 5. Protect verified defenses from corpus replacement

`FC_TACTICAL_FORCED_DEFENSE` becomes a corpus obligation. Corpus advice may replace it only when the candidate is an own faster proven win or disproves the same opponent proof under an equal-or-broader completed scope. Trusted support and search score alone are insufficient.

### 6. Separate evaluation determinism from user variety

Evaluation mode derives the baseline hint deterministically from the board rather than the benchmark opening/decision seed. User mode may retain a seeded legacy hint, but five-star randomness remains limited to candidates tied in proof class, scope, distance, tactical obligation and support. This prevents the same board from receiving different evaluation moves solely because it appeared under another opening ID.

### 7. Freeze diagnostics before fresh four-cell evaluation

The old 17 losses and a new non-final natural seed domain select only structural variants and mobile budgets. After code/profile hashes are frozen, tooling generates new untouched no-forbidden and forbidden master seeds. Each rule/opponent cell contains 100 games from 50 natural opening identities with colors exchanged. Reports lead with five-star black for this change, then white and overall results; white and overall must not regress for promotion.

## Risks / Trade-offs

- [Unknown alternatives replace a verified loss but still lose faster] → Require immediate safety, order unknowns by proof/disproof progress and defensive coverage, retain telemetry, and compare loss distance diagnostically without calling them safe.
- [All-legal widening exceeds mobile latency] → Use certificate points first, progressive stages, shared TT, node ceilings and localized threat caches; return the best completed class at the ceiling.
- [Quiet setup search introduces unsound heuristic overrides] → Allow production override only on verified proof or a separately frozen conservative gate with completed opponent counter-proof.
- [Loss fixtures overfit the released schedule] → Use them only for correctness/ablation, exclude their IDs/seeds from new formal schedules, and prohibit runtime tables derived from them.
- [Freestyle assumptions leak into forbidden play] → Keep rule in every proof/cache key, reuse runtime legality, and report each rule separately.
- [Strength gain comes from black while white regresses] → Require non-negative white and overall point estimates in every formal cell and report both colors without weighting away first-player advantage.

## Migration Plan

1. Freeze current five-star/four-star/legacy hashes and extract loss diagnostics with checksums.
2. Implement proof-class ranking, certificate escape generation, progressive widening and telemetry behind a new five-star profile version.
3. Implement DFPN/session reuse and localized threat generation; pass proof/symmetry/sanitizer/mobile latency gates.
4. Add quiet-threat search and corpus forced-defense protection, then select/freeze structural parameters on non-final diagnostics.
5. Generate fresh formal seeds, run the four 100-game cells, replay every move/proof and publish checksummed reports.
6. Promote the new profile only if correctness, black-directional improvement, white non-regression and overall non-regression gates pass; rollback retains the current five-star profile unchanged.

## Open Questions

- The exact emergency widening stages and quiet-root limit will be selected on non-final diagnostics before formal seeds are generated.
- If localized threat generation cannot meet the oldest-device budget, quiet search remains disabled in production while the escape-ranking correction can still ship independently.
