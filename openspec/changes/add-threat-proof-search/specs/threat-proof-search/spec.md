## ADDED Requirements

### Requirement: Complete rule-aware threat representation
The engine SHALL represent each tactical threat with its attacking gain point, every relevant legal defensive cost point, continuation dependency points, severity, side to move, and rule mode. It SHALL enumerate immediate five, open four, simple/broken four, four-three and three-based VCT threats without using learned parameters.

#### Scenario: Threat has multiple defenses
- **WHEN** one attacking move can be answered at more than one legal point
- **THEN** the threat SHALL contain every such defensive point rather than retaining only the first point found

#### Scenario: Forbidden black continuation
- **WHEN** forbidden-move mode is enabled and a black continuation is an overline, double-four, or double-three forbidden by the application rule implementation
- **THEN** that continuation SHALL NOT be emitted as a legal threat branch

### Requirement: Sound AND/OR proof semantics
The solver SHALL treat attacker positions as OR nodes and defender positions as AND nodes. A forced win SHALL be returned only when at least one attacker continuation wins against every legal defender reply represented by the threat, including immediate defender wins and counter-threats of sufficient severity.

#### Scenario: One defense refutes an attack
- **WHEN** any legal defender reply escapes all continuations in an alleged winning line
- **THEN** the parent attack SHALL NOT be classified as a proven win

#### Scenario: Every defense loses
- **WHEN** the verifier confirms that every legal defensive reply has a proven winning attacker continuation
- **THEN** the parent node SHALL be classified as a proven win and expose its proof children

### Requirement: Staged VCF and DFPN VCT solving
The solver SHALL run a strict continuous-four VCF stage before a budgeted depth-first proof-number search over broader VCT threats. Proof and disproof numbers, node/depth budgets, elapsed time, transposition hits, and the deepest completed proof stage SHALL be observable.

#### Scenario: VCF proves a win
- **WHEN** the strict VCF stage proves a continuous-four win within budget
- **THEN** the solver SHALL return the VCF proof without requiring broader VCT expansion

#### Scenario: Budget ends before proof
- **WHEN** neither proof nor disproof completes before a configured budget is exhausted
- **THEN** the solver SHALL return `unknown` and SHALL NOT convert a heuristic score into a proof result

### Requirement: Three-valued scoped result
The public solver result SHALL distinguish `proven-win`, `no-forced-win-in-scope`, and `unknown`. A negative result SHALL be described only for the configured VCF/VCT search class and SHALL NOT be described as a proof that the full game is drawn or won.

#### Scenario: Search class is disproved
- **WHEN** DFPN disproves every eligible root threat in the configured class
- **THEN** the result SHALL be `no-forced-win-in-scope` and record the searched threat class and budgets

### Requirement: Independently verifiable proof certificate
Every `proven-win` result SHALL include or permit reconstruction of a deterministic proof certificate. A separate verifier SHALL replay it from the original board, enumerate all legal replies at defender nodes, validate terminal wins, and confirm make/unmake board integrity.

#### Scenario: Corrupted proof branch
- **WHEN** a proof certificate omits a legal defense, contains an illegal move, or does not reach a terminal win
- **THEN** the verifier SHALL reject it and the decision layer SHALL treat the result as `unknown`

### Requirement: Deterministic data-free operation
For identical board, rule mode, profile, and node budget, the proof solver SHALL produce the same result and certificate independent of global random state. It SHALL operate without neural-network weights, training data, online services, or third-party runtime engines.

#### Scenario: Repeated proof query
- **WHEN** the same query is run after unrelated random calls or another engine query
- **THEN** its proof state, selected proof move, and node statistics SHALL remain reproducible

