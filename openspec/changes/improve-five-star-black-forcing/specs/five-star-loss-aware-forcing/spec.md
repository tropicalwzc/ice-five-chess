## ADDED Requirements

### Requirement: Completed proof-class ordering
Five-star SHALL rank legal candidates as own verified win, opponent forced win disproved in a comparable completed scope, unknown, then opponent verified win. It SHALL rank verified own wins by shortest distance and verified losses by longest survival, and SHALL NOT retain a verified-losing default merely because alternatives are unknown.

#### Scenario: Default is proven losing and an alternative is unknown
- **WHEN** the default permits a verified opponent win and a legal immediately safe alternative remains unknown within budget
- **THEN** five-star SHALL prefer the unknown alternative over the verified-losing default and label it unknown rather than safe

#### Scenario: Every candidate is proven losing
- **WHEN** every completed legal candidate permits a verified opponent win
- **THEN** five-star SHALL select the candidate with the longest verified survival and expose an all-proven-losing decision

### Requirement: Certificate-directed progressive escape search
When a default permits a verified opponent win, five-star SHALL search legal certificate-derived interruption, defense and counter-threat points before progressively widening beyond the first six heuristic candidates. Search work and transpositions SHALL be reusable across alternatives in the same decision.

#### Scenario: Escape is outside the heuristic top six
- **WHEN** a legal move outside the first six heuristic candidates invalidates the opponent proof in a comparable completed scope
- **THEN** five-star SHALL be able to select that move and record the widening stage and certificate relationship

### Requirement: Most-proving-node proof search
The VCT solver SHALL implement proof/disproof thresholds, deterministic most-proving-child expansion and rule/search-aware transposition reuse. Overflow, incomplete enumeration, budget exhaustion or certificate failure SHALL produce unknown.

#### Scenario: Most-proving child changes after expansion
- **WHEN** an expanded child updates an AND/OR node's proof and disproof numbers
- **THEN** the solver SHALL recompute thresholds and continue with the deterministic most-proving child until proof, disproof or budget exhaustion

### Requirement: Bounded proactive quiet-threat search
Five-star MAY examine a frozen bounded set of non-immediate quiet setup moves that create multiple independent future forcing dependencies. A quiet move SHALL override four-star only with verified forcing evidence or a frozen conservative gate plus completed opponent counter-proof.

#### Scenario: Quiet setup has a verified continuation
- **WHEN** a legal quiet move leads to a verified VCF/VCT against every relevant reply within the completed scope
- **THEN** five-star SHALL rank it as a verified attack according to proof distance

### Requirement: Corpus protection for verified defense
Corpus advice SHALL NOT replace a four-star verified forced defense using support counts or search score alone. Replacement SHALL require a faster own verified win or equal-or-broader completed evidence that removes the opponent proof.

#### Scenario: Trusted corpus move conflicts with forced defense
- **WHEN** four-star selected a verified forced defense and a highly supported corpus move lacks comparable completed proof evidence
- **THEN** five-star SHALL retain the four-star defense and record the corpus rejection reason

### Requirement: Board-deterministic evaluation
For identical board, side, rule, profile and budgets, evaluation mode SHALL produce the same move and proof metadata independent of opening ID, decision seed and unrelated random calls. User randomness SHALL remain limited to proof-equivalent candidates.

#### Scenario: Same board appears under different benchmark identities
- **WHEN** evaluation analyzes the same position with different schedule seeds
- **THEN** the selected move, proof class and certificate identity SHALL be identical

### Requirement: Honest loss semantics and observability
Telemetry SHALL distinguish verified all-moves-losing, verified selected-move-losing, unknown due to budget, and no immediately safe generated candidate. It SHALL record escape stage, alternatives examined, proof-class counts, selected loss distance and whether corpus protection applied.

#### Scenario: Candidate generation finds no immediately safe move
- **WHEN** the current bounded generator finds no move passing immediate safety but no complete all-move proof exists
- **THEN** telemetry and reports SHALL NOT label the position a mathematical proven loss
