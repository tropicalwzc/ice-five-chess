## ADDED Requirements

### Requirement: Hybrid dispatch is determined only by stone color
The candidate hybrid SHALL dispatch every white decision to frozen proof-engine `5.4.1-transactional-deadline-root-parallel-5s` and every black decision to frozen production `5.1.0-elite-rule-partitioned-local-v2`, independent of opponent, opening identity, prior outcome, and rule mode.

#### Scenario: Hybrid owns white
- **WHEN** the hybrid analyzes a position with white to move
- **THEN** it SHALL invoke the frozen proof-engine candidate and SHALL log the white component identity

#### Scenario: Hybrid owns black
- **WHEN** the hybrid analyzes a position with black to move
- **THEN** it SHALL invoke production v5.1 with the candidate-only outer deadline reserve and SHALL log the black component identity

#### Scenario: Opponent or opening changes
- **WHEN** the same board, side, rule, seed, and random mode are evaluated under a different opponent label or opening identity
- **THEN** the hybrid component routing SHALL remain unchanged

### Requirement: Hybrid component behavior is reference-equivalent
For identical board, side, rule, seed, random mode, hint, and resource inputs, hybrid black SHALL use direct v5.1 search and hybrid white SHALL use the direct frozen proof-engine candidate search. Their result SHALL match except for router provenance and the candidate-only safety veto of a legacy random alternative that lacks completed-equivalence evidence; a veto SHALL return the same component's best completed legal result.

#### Scenario: Black component golden comparison
- **WHEN** hybrid black and direct v5.1 analyze the same no-forbidden or forbidden fixture
- **THEN** their chosen move, random-selection metadata, tactical result, and legality SHALL match

#### Scenario: White component golden comparison
- **WHEN** hybrid white and the direct proof-engine candidate analyze the same no-forbidden or forbidden fixture
- **THEN** their chosen move, completed proof metadata, certificate identity, random-selection metadata, and legality SHALL match subject to the same wall-completion boundary

#### Scenario: Frozen component proposes an unverified random alternative
- **WHEN** a direct component's legacy user-random layer selects a score-near move without completed-equivalence verification
- **THEN** the hybrid SHALL veto that policy choice, return the same component's best completed legal move, and report one eligible rank with no random selection used

#### Scenario: Black v5.1 reaches the hybrid outer deadline
- **WHEN** production v5.1 search or corpus comparison remains active at 4,300ms
- **THEN** the hybrid SHALL stop unfinished work, return v5.1's best completed legal result, mark budget exhaustion, and retain finalization reserve below 5,000ms

### Requirement: User randomness is restricted to equivalent completed decisions
The hybrid SHALL permit seeded user-game randomness only among moves equal in completed proof class, completed scope, proof distance, tactical obligation, corpus support tier, and verified safety/loss status. It SHALL log the decision seed, eligible count, selected rank, equivalence signature, and whether random selection occurred.

#### Scenario: Multiple equivalent alternatives exist
- **WHEN** two or more moves have identical completed equivalence fields
- **THEN** the hybrid MAY select among them using the logged user seed and SHALL report the eligible count and selected rank

#### Scenario: Alternative is only score-near
- **WHEN** an alternative differs in proof class, scope, distance, tactical obligation, corpus support, or safety/loss status
- **THEN** it SHALL NOT be eligible for random selection even if its heuristic score is near the best move

#### Scenario: Different user seeds are used
- **WHEN** the same position has multiple eligible proof-equivalent alternatives and is evaluated with different user seeds
- **THEN** different legal selected moves SHALL be accepted as intended policy variation

### Requirement: Hybrid decisions remain bounded and fail closed
Every hybrid decision SHALL return a legal move within the 5,000 ms player-visible ceiling. Unfinished proof work SHALL remain unknown, and timeout SHALL return the best completed legal result available without randomizing across a weaker equivalence class.

#### Scenario: White proof search reaches its deadline
- **WHEN** the white component reaches the shared internal deadline
- **THEN** it SHALL unwind unfinished work, return the highest completed legal equivalence class within 5,000 ms, and log budget exhaustion separately from random selection

#### Scenario: Component returns an illegal move
- **WHEN** either component proposes a move illegal under the active rule
- **THEN** the hybrid SHALL reject it, fail closed to a verified legal fallback, and mark the diagnostic failure

### Requirement: Existing player levels remain isolated
Production five-star v5.1, frozen four-star, legacy three-star, one/two-star behavior, UI mappings, persistence, and randomness contracts SHALL remain unchanged until a separate explicit production switch.

#### Scenario: Player selects current five-star
- **WHEN** the hybrid has not been explicitly promoted
- **THEN** the player-facing five-star entry SHALL continue to use production v5.1

#### Scenario: Lower difficulty is selected
- **WHEN** one-star through frozen four-star is requested
- **THEN** no hybrid router or proof-engine-only override SHALL execute

### Requirement: Hybrid telemetry identifies component and policy behavior
Every hybrid candidate step SHALL record side, component enum/version, decision seed, random mode, random eligible count, selected rank, random-use flag, completed equivalence signature, budget exhaustion, latency, CPU time, worker activity, and peak resident memory.

#### Scenario: A formal step is serialized
- **WHEN** the hybrid writes a benchmark step
- **THEN** the record SHALL contain enough fields to prove correct side routing and audit whether any random choice stayed within the declared equivalence class
