## ADDED Requirements

### Requirement: The audited fallback positions SHALL be a versioned replay fixture

The evaluation tooling SHALL provide a manifest for the 97 audited fallback
positions.  Each entry SHALL include the board, side to move, forbidden-rule
mode, legacy hint/fallback metadata, and the source log hash or equivalent
immutable provenance.  The replay SHALL validate every selected move with the
same legality rules used by the engine.

#### Scenario: A fallback fixture is replayed under its recorded rule mode

- **WHEN** a fixture is loaded with its board and forbidden-rule flag
- **THEN** the engine analyzes exactly that position and the selected move is legal for the recorded side
- **AND** the report preserves the fixture identifier and source provenance

#### Scenario: A fixture exposes an avoidable fork

- **WHEN** complete candidate coverage finds a fork-safe legal alternative in an audited position
- **THEN** the replay gate requires the selected move not to leave two legal opponent immediate wins
- **AND** the report counts the position as fork avoided rather than merely as a legal fallback

### Requirement: Worker-count comparisons SHALL use identical inputs and deterministic merging

The research profile SHALL support worker cells of exactly 1, 4, and 8
workers.  The cells SHALL use the same fixture order, profile parameters,
seeds, rule modes, and deadline.  Worker proof state SHALL remain private, and
the coordinator SHALL merge results by canonical candidate index rather than
by thread completion order.

#### Scenario: One, four, and eight workers replay the same position

- **WHEN** a fixture is analyzed in each supported worker cell
- **THEN** legality, selected move, decision status, and fork classification are identical
- **AND** worker utilization and completion order are reported separately

#### Scenario: A worker count outside the supported matrix is requested

- **WHEN** a benchmark requests a worker count below 1, above 8, or not in the 1/4/8 comparison matrix
- **THEN** the harness rejects the cell or records an explicit configuration failure
- **AND** it does not silently present the result as a valid parallel comparison

### Requirement: Evaluation reports SHALL measure useful work and resource effects

Each worker cell SHALL report wall time, CPU time, actual concurrent workers,
proof sessions, completed proofs, unknowns, fallbacks, fork-safe selections,
live/peak memory, ledger exhaustions, and useful proof work.  “Parallel
advantage” SHALL be derived from these measures and fixed-position outcomes;
worker count alone SHALL not qualify as an improvement.

#### Scenario: Additional workers provide no useful gain

- **WHEN** the 4- or 8-worker cell launches more workers but completes no more useful proof work and improves no fixed-position outcome
- **THEN** the report labels the cell as no measured parallel advantage
- **AND** it preserves the result as evidence rather than claiming a strength improvement

#### Scenario: Parallel execution causes a correctness or ledger drift

- **WHEN** a worker cell changes a selected move/legality result or ends with unreconciled live reservation bytes
- **THEN** the fixed-position gate fails the cell
- **AND** the 100-game strength comparison is not authorized by that report

### Requirement: Strength suites SHALL be gated by fixed-position correctness

The evaluation tooling MUST refuse to interpret a new forbidden/no-forbidden
100-game comparison as a model improvement until the 97-position replay and
the 1/4/8 worker matrix pass legality, determinism, fork-avoidance, and ledger
reconciliation checks.

#### Scenario: Fixed gates pass before a new game suite

- **WHEN** all fixture moves are legal, avoidable forks are avoided, worker cells are deterministic, and live ledger bytes reconcile to zero
- **THEN** the report marks the strength suite eligible and includes color/rule-separated metrics

#### Scenario: A fixed gate fails

- **WHEN** any fixture has illegal output, false verified loss, selected-move drift, or unreconciled memory
- **THEN** the report marks the strength suite ineligible
- **AND** no win-rate delta is promoted as evidence of a better five-star model
