## ADDED Requirements

### Requirement: Five-star-only engine isolation and rollback
The system SHALL expose the incremental proof engine only through a candidate five-star profile until promotion, SHALL retain production five-star v5.1 as rollback, and SHALL preserve the profile identity, decision behavior, persistence, randomness contract, and UI mapping of one-star through frozen four-star.

#### Scenario: Lower difficulty is selected
- **WHEN** a user or benchmark requests one-star, two-star, frozen legacy three-star, or frozen four-star
- **THEN** the system SHALL use the frozen level path and SHALL produce its existing golden decision behavior without invoking five-star-only proof-session overrides

#### Scenario: Candidate has not passed promotion
- **WHEN** any correctness, performance, strength, or report gate remains incomplete or fails
- **THEN** the player-facing five-star entry SHALL remain on production v5.1 and the candidate SHALL remain accessible only through explicit test/profile selection

#### Scenario: Four-star remains available
- **WHEN** the new five-star candidate is built or later promoted
- **THEN** the UI and benchmark tooling SHALL continue to expose the prior frozen four-star as a playable difficulty and formal opponent

### Requirement: Incremental position state is reference-equivalent
The five-star proof engine SHALL update board hash, stone count, occupied/frontier masks, affected line facts, threat masks, and rule-aware legality facts through reversible make/unmake deltas, and every externally visible result SHALL be equivalent to the full-scan reference implementation for the same board and rule.

#### Scenario: Move and unmove under no-forbidden rules
- **WHEN** an arbitrary legal move sequence is applied and fully undone in no-forbidden mode
- **THEN** the board bytes, hash, legal moves, immediate wins, forcing threats, creators, and generated proof replies SHALL match the reference at every checked step and the final state SHALL equal the initial state

#### Scenario: Move and unmove under forbidden rules
- **WHEN** an arbitrary sequence containing black legality dependencies is applied and undone in forbidden mode
- **THEN** incremental legality and threat results SHALL match the forbidden-rule reference and every dirtied crossing line SHALL be restored exactly

#### Scenario: Incremental fact is incomplete
- **WHEN** the engine cannot establish that a cached legality or threat fact is complete for the active rule
- **THEN** it SHALL use the complete reference fallback or return unknown and SHALL NOT certify a proof from the incomplete fact

### Requirement: One bounded proof session serves the complete decision
The system SHALL create one bounded proof session for a candidate five-star decision and SHALL reuse its arena and transposition graph across own-win, opponent-after-baseline, escape, and corpus-candidate queries while enforcing one deterministic node budget and one global 5,000 ms player-visible wall-clock safety ceiling.

#### Scenario: Multiple alternatives transpose
- **WHEN** baseline, escape, or corpus alternatives reach an identical state with identical attacker, side-to-move, rule, search class, scope, algorithm version, and completeness mode
- **THEN** the later query SHALL reuse the compatible proof entry and telemetry SHALL record the session hit rather than allocate and clear an independent table

#### Scenario: Stored scope is narrower
- **WHEN** a transposition entry was produced with a narrower depth, search class, rule, or relevance completeness scope than the current query
- **THEN** the entry SHALL NOT satisfy the broader query and the result SHALL remain in progress or unknown until the broader scope completes

#### Scenario: Global decision budget is exhausted
- **WHEN** the session reaches its global node cap, wall ceiling, arena cap, or certificate capacity
- **THEN** every unfinished query SHALL terminate as unknown, no nested query SHALL receive a fresh independent allowance, and the best legal move in the highest completed proof/evaluation class available before exhaustion SHALL be returned within 5,000 ms

### Requirement: Thresholded deterministic DFPN drives proof search
The proof engine SHALL represent attacker choices as OR nodes and complete defender reply sets as AND nodes, SHALL maintain saturated proof/disproof values and thresholds, and SHALL repeatedly expand the deterministic most-proving child until the root is proved, disproved within its declared scope, or exhausted.

#### Scenario: OR node is unresolved
- **WHEN** an unresolved attacker OR node has multiple expanded or frontier children
- **THEN** the solver SHALL select the deterministic child with the controlling proof value, derive its threshold from the parent and sibling values, expand it, and incrementally update affected ancestors

#### Scenario: AND node is unresolved
- **WHEN** an unresolved defender AND node has multiple required legal replies
- **THEN** the solver SHALL select the deterministic child with the controlling disproof value and SHALL require every non-irrelevant reply to be proved before declaring the attacker win

#### Scenario: Scoped disproof completes
- **WHEN** all attacker options and all required replies in the declared VCF or VCT scope have been enumerated and their disproof conditions complete
- **THEN** the solver SHALL return `no-forced-win-in-scope` with zero disproof number and SHALL record the exact completed class and scope

#### Scenario: Enumeration is incomplete
- **WHEN** threat overflow, reply overflow, relevance uncertainty, transposition-scope mismatch, or resource exhaustion prevents complete enumeration
- **THEN** the solver SHALL return unknown with nonterminal proof/disproof values and SHALL NOT label the position a scoped disproof

### Requirement: Root-parallel DFPN preserves proof semantics and deterministic budgets
The candidate five-star SHALL be able to distribute ordered independent forcing roots across a frozen number of worker threads. Every worker SHALL own its mutable position, DFPN graph, transposition table, certificate arena, and diagnostics; workers SHALL share only immutable root inputs, one absolute decision deadline, and a pre-partitioned aggregate node/memory budget.

#### Scenario: Multiple forcing roots are available
- **WHEN** a candidate proof stage has at least two unique ordered forcing gains and the profile allows multiple workers
- **THEN** the engine SHALL search distinct root gains concurrently up to the frozen worker count and SHALL record workers launched, jobs completed, aggregate nodes, and peak worker memory

#### Scenario: Parallel results complete in a different order
- **WHEN** worker scheduling causes later ordered roots to finish before earlier roots
- **THEN** the merger SHALL retain results by frozen root identity and SHALL choose the earliest verified winning root under the existing deterministic threat order rather than completion order

#### Scenario: Parallel root disproof is incomplete
- **WHEN** any enumerated root is unknown, overflows, misses its declared scope, or reaches the shared absolute deadline
- **THEN** the aggregate root result SHALL remain unknown and SHALL NOT be reported as a scoped disproof

#### Scenario: A worker reaches the global deadline
- **WHEN** any worker observes the shared absolute deadline
- **THEN** it SHALL unwind unfinished DFPN work as unknown, restore its private board, terminate without extending the caller deadline, and allow the caller to return the best previously completed legal move

#### Scenario: Worker count changes
- **WHEN** evaluation is run with a worker count different from the frozen candidate profile
- **THEN** the run SHALL be a separate resource ablation and SHALL NOT be mixed with formal evidence for the frozen profile

### Requirement: Relevance-zone reply reduction is sound
The engine SHALL account for every legal defender reply either by expanding it or by verifying that it is irrelevant to every retained winning certificate under the active rule. Relevance SHALL include gain/cost/rest dependencies, affected five-windows, qualifying global counter-threats, immediate wins, and forbidden-rule legality dependencies.

#### Scenario: Remote defender counter-threat exists
- **WHEN** a legal move outside the local threat radius wins immediately or creates a counter-threat fast enough for the current obligation
- **THEN** the relevance generator SHALL include that move in the AND reply set and SHALL NOT omit it based on distance

#### Scenario: Outside-zone move is omitted
- **WHEN** the generator proposes omitting an outside-zone defender move
- **THEN** a conservative check SHALL prove that the move cannot win, alter a gain/cost/rest or line dependency, create a qualifying counter-threat, or change planned-move legality before the omission counts as complete

#### Scenario: Relevance cannot be proven
- **WHEN** an outside-zone move fails the conservative irrelevance check or a forbidden legality dependency is unresolved
- **THEN** the engine SHALL include the move, fall back to all legal replies, or return unknown

#### Scenario: Iterated related zones shrink replies
- **WHEN** multiple independently verified winning continuations cover a defender move
- **THEN** the engine SHALL remove that move only when at least one still-valid certificate proves a win after it and SHALL retain coverage telemetry for replay

### Requirement: Dependency threat search is a verified candidate generator
The engine SHALL combine gain/cost/rest threat operators only when their dependencies and interference conditions are compatible, and SHALL submit every resulting VCT candidate to exact DFPN and certificate verification before it can affect a production decision.

#### Scenario: Independent threats combine
- **WHEN** one threat establishes the rest condition of another and their cost/rest sets do not invalidate each other
- **THEN** dependency search SHALL be able to propose the combined continuation without enumerating every unrelated move order

#### Scenario: Proposed dependency chain is unsound
- **WHEN** exact defender enumeration finds a counterplay omitted by the dependency candidate
- **THEN** the candidate SHALL be rejected or remain unknown and SHALL NOT be recorded as a verified forced win

### Requirement: Forcing stages precede bounded quiet initiative
The candidate five-star SHALL run immediate tactics, mandatory defense, VCF, and VCT/dependency stages before a bounded quiet/implicit-threat stage, and the quiet stage SHALL operate inside the same global proof session only when neither side has an immediate tactical obligation.

#### Scenario: Immediate obligation exists
- **WHEN** the side to move has an immediate win or must answer an immediate opponent win
- **THEN** the system SHALL resolve that obligation and SHALL NOT spend budget on quiet roots

#### Scenario: Quiet root creates independent initiative
- **WHEN** no forcing proof is available and a quiet root meets the frozen structural requirement for multiple independent future forcing dependencies
- **THEN** the engine SHALL counter-prove the root with the same rule-aware DFPN scope before considering an override

#### Scenario: Quiet root has only heuristic promise
- **WHEN** a quiet root has a favorable heuristic or dependency score but lacks a completed verified own win or completed comparable defense
- **THEN** it SHALL NOT replace the frozen four-star baseline in production decision ordering

#### Scenario: Quiet search is enabled
- **WHEN** the candidate profile declares quiet initiative enabled and eligible natural diagnostic positions occur
- **THEN** telemetry SHALL distinguish eligible decisions, roots examined, completed proofs/disproofs, unknowns, selected quiet moves, and budget skipped cases rather than reporting an unused configuration flag

### Requirement: Completed proof class controls five-star composition
The five-star decision layer SHALL order completed evidence as own verified win, comparable completed opponent disproof, unknown safe alternative, and opponent verified win; SHALL order verified losses by longest survival; and SHALL protect completed forced defenses from corpus or heuristic replacement without equal-or-broader completed evidence.

#### Scenario: Default move is verified losing
- **WHEN** the baseline permits a verified opponent win and a safe alternative remains unknown within budget
- **THEN** the unknown alternative SHALL rank above the verified-losing default without being labeled proven safe

#### Scenario: Corpus conflicts with verified defense
- **WHEN** the baseline is a completed forced defense and corpus advice differs
- **THEN** the corpus move SHALL replace it only if it is a faster own verified win or completes an equal-or-broader opponent disproof

#### Scenario: Every completed alternative loses
- **WHEN** all examined alternatives are verified losses and no unknown or completed defense remains
- **THEN** the engine SHALL select the longest verified survival and SHALL report whether all legal alternatives were actually completed or the global budget ended early

### Requirement: Certificates are independently replayable and fail closed
Every accepted forced-win or completed-defense override SHALL carry a deterministic certificate or completed proof-graph reference that an independent verifier can replay under the exact board, rule, class, and scope without relying on relevance omissions as assumptions.

#### Scenario: Certificate replays successfully
- **WHEN** an accepted certificate is replayed from its original position
- **THEN** every move SHALL be legal, every AND reply obligation SHALL be covered, every terminal claim SHALL be valid, and the mutable board SHALL be restored exactly

#### Scenario: Certificate or relevance replay fails
- **WHEN** a node hash, legal move, reply set, relevance coverage, terminal condition, or final board restoration differs
- **THEN** the result SHALL become unknown and SHALL NOT override the frozen baseline

### Requirement: Evaluation is deterministic and user variation is proof-equivalent
For identical board, side, rule, candidate profile, structural limits, and node budget, evaluation mode SHALL produce identical moves and completed proof metadata independent of opening ID, decision seed, timing jitter, and unrelated random calls. Fixed node-budget runs that do not reach the wall-clock safety deadline SHALL also produce identical deterministic node counts. In safety-wall runs, raw counters for unfinished parallel work MAY vary with worker scheduling, but completed result metadata and the selected move SHALL remain deterministic and the unfinished work SHALL be marked budget-exhausted. User-game randomness SHALL be restricted to candidates equal in completed proof class, scope, distance, tactical obligation, and corpus support.

#### Scenario: Evaluation is repeated
- **WHEN** the same position is analyzed repeatedly in evaluation mode with different benchmark seeds under a fixed node budget and no run reaches the wall-clock safety deadline
- **THEN** the chosen move, proof status, completed scope, certificate identifier, relevance counts, and deterministic node count SHALL match

#### Scenario: Parallel evaluation reaches the safety wall
- **WHEN** repeated evaluation runs reach the shared wall-clock safety deadline with unfinished parallel root work
- **THEN** the chosen legal move, completed proof status, completed scope, certificate identifier, and completed relevance metadata SHALL match, while any varying raw unfinished-work node counters SHALL be reported separately and marked budget-exhausted

#### Scenario: User-game alternatives are not proof-equivalent
- **WHEN** two legal candidates differ in completed proof class, completed scope, distance, tactical obligation, or corpus support
- **THEN** random selection SHALL NOT choose the lower-ranked candidate
