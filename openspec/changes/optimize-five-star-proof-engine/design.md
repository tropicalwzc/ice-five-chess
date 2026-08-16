## Context

Production UI still uses five-star v5.1. The v5.2.1 loss-aware candidate widened from six heuristic escapes to certificate-first progressive widening, but its independent A/B matched v5.1 strength and raised same-seed p95 latency from 1160 ms to 1425 ms in no-forbidden play and from 2107 ms to 2468 ms in forbidden play. Its formal 400 games examined 34,898 escape alternatives yet completed no scoped disproof, and the candidate was correctly left outside the player entry point.

The current proof path recomputes a 225-cell board hash, scans the board for immediate wins, creators, threats, and refutations at nested levels, and copies the board during forbidden-black legality checks. Each `fc_prove_forced_win` call creates and clears a separate direct-mapped table, while five-star composition first runs the frozen four-star analysis and can then repeat proof and score searches for the baseline, loss-aware alternatives, and corpus alternatives. Proof/disproof values are accumulated after an ordinary ordered depth-first traversal; there is no thresholded most-proving-child loop. The quiet-threat configuration and telemetry exist, but the proactive stage is not implemented.

The completed evidence, honest loss labels, certificate-directed candidate extraction, corpus defense protection, deterministic evaluation rules, and frozen opponents from `improve-five-star-black-forcing` remain valid inputs. This change owns the unfinished proof-engine work so the rejected candidate is not accidentally promoted or implemented twice.

Constraints are mobile C/Objective-C integration, deterministic evaluation, bounded memory and response time, exact separation of no-forbidden and forbidden rules, no training or live network dependency, independent proof replay, and immutable one-through-four-star player behavior. The existing four-star must remain both a playable level and the primary strength opponent; the frozen legacy three-star remains the overfitting/generalization control.

## Goals / Non-Goals

**Goals:**

- Reduce repeated work and enforce a five-second player-visible decision ceiling before increasing any search scope.
- Turn proof/disproof numbers into the driver of a real deterministic DFPN search with reusable transpositions.
- Reduce attacker and defender branching with sound Gomoku-specific relevance and dependency reasoning while preserving fail-closed proof semantics.
- Add proactive initiative through a bounded quiet/implicit-threat stage after verified forcing stages, not through uniform alpha-beta depth increases.
- Preserve current four-star, legacy three-star, one/two-star behavior and expose the new engine only through a candidate five-star profile until promotion gates pass.
- Demonstrate correctness, faster fixed-position performance, and color/rule-separated strength on fresh evidence.

**Non-Goals:**

- Claim a complete solution of unrestricted 15x15 Freestyle or forbidden/Renju.
- Add neural inference, self-play training, downloaded runtime data, or opponent-specific response tables.
- Encode prior loss opening IDs, formal opponent continuations, or outcome-selected coordinates into production.
- Change the curated corpus contents or allow corpus trust to bypass completed tactical evidence.
- Add cross-game global proof persistence. Parallelism is decision-local only and does not share mutable proof state across games or turns.
- Promote v5.2.1 or replace production v5.1 before all gates pass.

## Decisions

### 1. Isolate the new engine behind a candidate five-star path

The frozen four-star public analysis path, profile identity, deterministic golden fixtures, UI mapping, and benchmark-control provenance remain unchanged. The new structures and algorithms are called only by a new five-star candidate factory. Production `fc_profile_five_star()` continues to return v5.1 until promotion. Completed loss-aware ranking and corpus-protection behavior is reused, but unfinished tasks 3.1–3.5 and 5.1–5.2 in `improve-five-star-black-forcing` are implemented and verified here rather than independently in both changes.

The benchmark records hashes of the frozen control source/profile assets and checks golden decisions before using four-star or legacy three-star as opponents. Shared public rule helpers may receive correctness-preserving fixes only if golden behavior and an independently compiled frozen control remain available.

Alternative rejected: mutate the four-star solver in place and rely only on profile values. That makes it difficult to distinguish a stronger five-star from an accidentally changed opponent.

### 2. Introduce an incremental, reversible five-star position state

`FCProofPosition` will own the 15x15 board plus:

- an incrementally updated Zobrist key, side/rule/search metadata, and stone count;
- 225-bit occupied and candidate-frontier masks;
- identifiers/codes for the four lines crossing each intersection;
- per-side immediate-win, forcing-threat, creator, and relevant-counter masks;
- rule-aware legality facts and dirty markers for cells whose four crossing lines changed;
- a make/unmake delta stack restoring every modified cache exactly.

A move dirties only its crossing row, column, and two diagonals. Incremental generators update those lines and affected masks; they do not rescan unrelated intersections. Forbidden-black legality uses a reversible internal evaluator and line facts, with a conservative reference fallback when a cached dependency is unavailable. Release builds remain fail-closed, while tests and diagnostic builds can recompute the existing full-board reference result after arbitrary make/unmake sequences and assert byte-for-byte board plus semantic equivalence.

Alternative rejected: optimize only the loops in the existing stateless functions. That leaves O(225) hashing, repeated copies, and nested rescans at every proof node.

### 3. Use one bounded proof session for the complete five-star decision

`FCProofSession` is created once after the frozen four-star baseline move is obtained. It contains the incremental position, a bounded arena, generation-stamped transposition storage, proof graph nodes, certificate storage, global deterministic node counters, stage quotas, and one wall-clock safety deadline. It is reused for own-win proof, opponent-after-baseline proof, certificate escapes, ordinary escapes, and corpus alternatives. The four-star result itself is not recomputed or changed.

Transposition keys include the exact board key, attacker, side to move, rule mode, search class, remaining completed scope, profile/algorithm version, and any relevance-zone completeness mode. Entries store status, proof/disproof values, expanded/completed flags, best/most-proving edge, scope, and verified relevance/certificate metadata. A narrower or incomplete result cannot satisfy a broader query. Generation tags clear a logical session without repeatedly zeroing megabytes; the arena and table have fixed caps and return unknown on exhaustion.

Node budgets are the reproducibility authority. The wall deadline is a global decision safety ceiling, not a fresh allowance for each nested proof. A stage cannot start when its reserved minimum cannot fit in the remaining global budget.

Alternative rejected: one table per candidate. It discards transpositions caused by different move orders and repeats allocation, hashing, and proof work precisely where v5.2.1 widened the most.

### 3a. Split deterministic forcing roots across isolated worker sessions

After the single-session engine and deadline behavior were validated, the target M4 Pro hardware made decision-local root parallelism practical. The candidate may enumerate the ordered unique forcing gains for a proof stage once, then assign those immutable root identities to up to eight workers. Each worker owns a private incremental position, DFPN graph, TT, certificate arena, counters, and allocation; workers never mutate another worker's graph or the caller's board. A thread-local root filter restricts a worker to its assigned gain, while the existing independent certificate verifier remains the acceptance authority.

All workers receive the same absolute decision deadline. The aggregate deterministic node budget is divided by the number of declared root jobs before launch, so scheduling cannot create extra node allowance. Results are stored by frozen root order and merged only after joining: the earliest verified winning root wins; an aggregate scoped disproof is usable only when root enumeration was complete and every root completed the declared scope; otherwise the result is unknown. At a wall timeout the engine falls back to the highest-ranked legal result completed before the deadline. Worker diagnostics are thread-local and explicitly reduced after join, avoiding data races in telemetry.

The candidate profile freezes `proofWorkerCount = 8` for the declared M4 Pro benchmark. Formal games run one game process at a time so nested process-level sharding does not oversubscribe the machine. Both colors and every opponent cell receive the same otherwise-idle host, process priority, wall-clock rules, and scheduling policy; the frozen four-star and legacy three-star remain byte-identical and are not retrofitted with candidate-only proof code. CPU time, peak resident memory, workers launched, jobs completed, and wall time are reported so the compute difference remains explicit.

Alternative rejected: concurrent mutation of one shared proof graph/TT. It would require fine-grained synchronization around make/unmake, graph allocation, thresholds, and certificates, and a race could turn incomplete work into false proof. Isolated root sessions spend more memory but preserve the already-tested single-thread proof semantics.

### 4. Implement thresholded DFPN over an explicit AND/OR proof graph

Attacker choices are OR nodes. Every complete legal refutation set for a selected threat is represented by an AND node. The solver initializes frontier proof/disproof values using a frozen depth/branching-aware rule, selects the deterministic most-proving child, derives child thresholds from the parent threshold and second-best/sibling sums, recursively expands only that child, and incrementally updates ancestors until proof, disproof, or budget exhaustion.

Zero and saturated infinity are reserved for completed results. Unknown frontier estimates guide ordering but never become a scoped disproof. A no-forced-win-in-scope result is returned only when all required attacker choices and all replies for that declared class/scope are completely enumerated. TT hits may reuse solved nodes or bounded proof progress only when all key fields and completeness conditions match.

The solution graph stores enough selected OR edges and complete AND children to reconstruct a certificate. Certificate verification runs through an independent generator/reference path and restores the original board before an override is accepted.

Alternative rejected: keep depth-first traversal and rank candidates by its post-hoc proof numbers. The current values are not comparable most-proving estimates after early breaks and cannot direct recovered budget reliably.

### 5. Make relevance-zone and dependency reasoning sound and independently checkable

Threat operators retain gain, cost, and rest points and additionally reference the exact line windows and legality dependencies they use. Dependency search combines operators only when the predecessor establishes the successor's rest conditions and their cost/rest interactions are compatible. It proposes long VCT continuations without replaying every unrelated move order; verified DFPN remains the authority for accepting a win or disproof.

For an AND node, the initial relevance zone contains the threat's cost points, gain/rest dependencies, affected five-windows, the attack certificate zone, legal immediate defender wins, and counter-threats fast enough for the current threat category. An outside-zone defender move may be omitted only if a conservative commutativity/relevance check proves that it cannot win, occupy or alter a dependency, create a qualifying counter-threat, or change legality of a planned black move under the active rule. Otherwise the move is included or the node remains unknown.

When independent winning continuations produce verified zones, iterated related-zone intersection may shrink the remaining defender set; each removed move must be covered by at least one still-valid winning certificate. Telemetry distinguishes candidate-zone size, all-legal size, omitted-and-verified count, conservative fallbacks, dependency combinations, and relevance failures.

Alternative rejected: radius/frontier-only defensive pruning. Distance is useful for ordering but is not a proof that a global counter-threat is irrelevant.

### 6. Spend recovered budget through a staged Lambda/TSS portfolio

The five-star portfolio runs in this order under one global session:

1. immediate win and mandatory defense;
2. VCF;
3. ordinary VCT plus dependency combinations;
4. a bounded set of quiet/implicit-threat roots only when neither side has an immediate obligation;
5. existing proof-class escape/corpus composition.

Quiet roots must establish at least two independent future forcing dependencies or another frozen structural implicit-threat criterion. Each root is subjected to the same opponent counter-proof and certificate replay. It may replace the frozen four-star baseline only as an own verified win or as a completed defense in an equal-or-broader scope; heuristic promise alone is not a production override. Evaluation ties are deterministic. User-game randomness remains limited to moves equal in completed proof class, scope, distance, tactical obligation, and corpus support.

The candidate uses one 5,000 ms player-visible hard ceiling for the complete decision. Search stops at an internal 4,500 ms deadline to reserve time for the legacy hint pass, worker join/result reduction, and final legality validation, then returns the best legal move whose evaluation/proof class completed before expiry. Structural limits, worker count, aggregate node/memory budgets, and any reinvested per-stage budget are frozen on non-final diagnostics before formal seeds are generated.

Alternative rejected: add another uniform ply or enlarge every proof budget. Prior experiments and v5.2.1 show that more incomplete repeated work raises latency without demonstrating stronger decisions.

### 7. Gate strength testing behind correctness and performance evidence

Before formal games, a checksummed diagnostic set covers prior proof failures and their eight symmetries, independently known VCF/VCT proofs and scoped disproofs, quiet initiative, transpositions, relevance fallbacks, forbidden/no-forbidden differences, corpus conflicts, and randomized make/unmake sequences. Historical losses select structural cases only and are excluded from formal schedules.

The candidate must pass reference equivalence, sanitizer, certificate replay, deterministic regeneration, frozen-opponent golden, and iOS Simulator gates. On the same fixed positions and reference hardware, incremental primitive throughput must be at least twice the full-scan reference for the declared batch. Candidate and production-v5.1 p50/p95/mean/max remain reported separately by rule, but the latency gate is the candidate's 5,000 ms player-visible hard ceiling rather than sub-millisecond percentile ordering. A timeout must preserve the best completed legal move and mark unfinished proof work unknown; any candidate decision above 5,000 ms blocks formal games unless the design is amended and parameters are re-frozen before final seeds.

After passing, fresh untouched seeds create four 100-game cells: no-forbidden versus four-star, no-forbidden versus legacy three-star, forbidden versus four-star, and forbidden versus legacy three-star. Each cell uses 50 natural opening identities with colors exchanged, producing 50 new-five-star-black and 50 new-five-star-white games. Formal results are not fed back into parameters.

Reports compare same-color paired results, lead with five-star white as the primary difficult-side indicator, retain natural first-player advantage without color weighting, and also report black and overall. Promotion requires positive white point-estimate direction against four-star in both rule cells, no negative black or overall point-estimate direction against four-star, and no white/overall regression against legacy three-star; uncertainty is reported and no pooled-rule result may hide a failing cell. All 400 games and accepted certificates must replay cleanly.

Alternative rejected: start another 400-game run while diagnostic scoped disproof and quiet-search activity remain zero. Match noise cannot validate an inactive algorithm.

## Risks / Trade-offs

- [Incremental caches silently diverge from the board] → Keep full-scan reference oracles, randomized long make/unmake tests, per-line dirty assertions, debug recomputation, symmetry tests, and fail closed on unavailable facts.
- [A relevance zone omits a remote winning counter-threat] → Include category-qualified global counter masks, verify every omission conservatively, replay certificates through an independent all-legal generator, and fall back to all legal replies or unknown.
- [Forbidden-rule dependencies are wider than a cached line fact] → Include rule and legality dependencies in keys/zones, dirty all four affected lines, and use the existing reference legality path when completeness is not established.
- [Shared TT reuses a narrow result in a broader query] → Key attacker, side, rule, class, remaining scope, algorithm version, and completeness mode; require stored scope dominance and generation validity.
- [DFPN repeatedly chases deep low-value threats] → Use depth/branching-aware frontier initialization, deterministic second-best thresholds, stage quotas, and non-final diagnostic ablations.
- [Dependency search finds an unsound combination] → Treat it only as a candidate generator; exact DFPN and independent certificate replay remain acceptance authorities.
- [Quiet or parallel search consumes all recovered time] → Run only after forcing stages, cap roots and aggregate nodes inside the global decision, share one absolute deadline, and require completed evidence for override.
- [Parallel workers race or oversubscribe the host] → Give every worker private mutable state and thread-local diagnostics, merge by frozen root order, run TSAN/concurrency stress tests, and use one formal game process at a time.
- [Shared helper edits alter four-star] → Keep the frozen path/profile and golden corpus, use an independently frozen control in benchmarks, and block on any decision mismatch.
- [Fresh formal outcomes influence tuning] → Freeze code/profile/corpus hashes and final seeds after diagnostics; any post-freeze change invalidates and regenerates the complete formal suite.
- [A 100-game cell is statistically inconclusive] → Report intervals and classify honestly; do not pool rules or colors to manufacture promotion evidence.

## Migration Plan

1. Freeze and checksum production v5.1, frozen four-star, legacy three-star, current corpus, existing diagnostics, and current fixed-position latency/proof telemetry.
2. Add counters and full-scan reference oracles, then implement incremental position state behind a five-star-only compatibility mode; require identical generated threats, legality, hashes, and decisions before continuing.
3. Add the bounded session/arena/generation TT and route baseline/opponent/escape/corpus proof queries through it without enabling new move overrides.
4. Replace the proof traversal with thresholded DFPN and certificate reconstruction; pass proof, disproof, transposition, symmetry, overflow, and sanitizer tests.
5. Add relevance-zone and dependency candidate generation with all-legal fallback, followed by the bounded quiet/implicit-threat stage.
6. Add isolated root-split DFPN, run non-final worker-count/node-partition/memory ablations, select structural limits and one global budget, freeze the candidate, and pass performance plus iOS gates.
7. Generate fresh formal seeds, run/replay the four 100-game cells, publish workspace and Downloads reports with checksums, and apply the predeclared promotion classification.
8. On pass, point only the five-star player entry to the new profile while retaining v5.1 as rollback. On any failure, keep production v5.1 and all lower levels unchanged.

## Open Questions

- The exact proof-graph/table capacity, global node budget, frontier initialization formula, relevance fallback threshold, and quiet-root limit will be selected on non-final diagnostics before formal seed generation. The player-visible wall ceiling is fixed at 5,000 ms with an internal 4,500 ms search deadline and a 500 ms integration/finalization reserve.
- If forbidden-mode relevance completeness requires too many conservative fallbacks, relevance pruning may ship only for no-forbidden while forbidden still benefits from incremental state and shared DFPN; the rule cells remain separate.
- Cross-turn reuse remains deferred. Root-parallel DFPN is enabled only after worker-count, peak-memory, determinism, timeout, sanitizer, and iOS evidence pass; otherwise the candidate falls back to one worker without changing proof semantics.
